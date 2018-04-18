#define LINE 12 //gamecube controller data line

#define CYCLE ARM_DWT_CYCCNT //cpu cycle counter. increments every cpu cycle

#define setBit0(x) x | (1 << 0)
#define setBit1(x) x | (1 << 1)
#define setBit2(x) x | (1 << 2)
#define setBit3(x) x | (1 << 3)
#define setBit4(x) x | (1 << 4)
#define setBit5(x) x | (1 << 5)
#define setBit6(x) x | (1 << 6)
#define setBit7(x) x | (1 << 7)

//one bit takes 4us to send
const uint32_t QUARTER = F_CPU / 1000000;
const uint32_t HALF = QUARTER * 2;
const uint32_t ONE = HALF * 2;
const uint32_t TWO = ONE * 2;

uint8_t buffer[8] = { 0, 0, 128, 128, 128, 128, 1, 1 };
const uint8_t id[3] = { 0x09,0x00,0x00 };
bool rumb;
uint8_t readMode;

#define BTN_A 4
#define BTN_B 5
#define BTN_X 6
#define BTN_Y 7

#define BTN_S 8
#define BTN_Z 9
#define BTN_R 10
#define BTN_L 11

#define A_U 13
#define A_R 14
#define A_L 15
#define A_D 16

#define RLED 3
#define YLED 2
#define GLED 1
#define BLED 0

class DogeBawx {
public:
	//sets up cycle counter and pinmodes for LEDs, buttons, and LINE
	DogeBawx() {
		initCounter();
		initOut();
		initIn();
		digitalWriteFast(BLED, HIGH);
	}

	//wait until the gap inbetween 
	static inline void init() {
		wait();
		digitalWriteFast(GLED, HIGH);
	}

	//read buttons and pack into buffer
	static inline void read() {
		buffer[0] =
			(1 << 3) |
			(!digitalReadFast(BTN_S) << 3) |
			(!digitalReadFast(BTN_Y) << 4) |
			(!digitalReadFast(BTN_X) << 5) |
			(!digitalReadFast(BTN_B) << 6) |
			(!digitalReadFast(BTN_A) << 7);
		buffer[1] =
			1 |
			(!digitalReadFast(BTN_L) << 1) |
			(!digitalReadFast(BTN_R) << 2) |
			(!digitalReadFast(BTN_Z) << 3);

		if (!digitalReadFast(A_L)) {
			buffer[2] = !digitalReadFast(A_R) ? 128 : 0;
		}
		else { buffer[2] = !digitalReadFast(A_R) ? 255 : 128; }

		if (!digitalReadFast(A_U)) {
			buffer[3] = !digitalReadFast(A_D) ? 128 : 0;
		}
		else { buffer[3] = !digitalReadFast(A_D) ? 255 : 128; }
	}

	//get command and respond appropriately
	static inline void write() {
		uint8_t cmd = getByte();
		if (cmd == 0x40) {
			readMode = getByte();
			rumb = getByte() & 1;
			getBit();//stop bit
			rspRead();
		}
		else if (cmd == 0x41) {
			getBit();//stop bit
			rspOrigin();
		}
		else if (cmd == 0x00) {
			getBit();//stop bit
			rspInit();
		}
	}

private:
	//send 3 byte response to init cmd
	static inline void rspInit() {
		pinMode(LINE, OUTPUT);
		for (uint8_t i = 0; i < 3; i++) {
			sendByte(id[i]);
		}
		sendBit(1);		//stop bit
		pinMode(LINE, INPUT);
	}

	//send 10 byte response to origin cmd
	static inline void rspOrigin() {
		pinMode(LINE, OUTPUT);
		for (uint8_t i = 0; i < 8; i++) {
			sendByte(buffer[i]);
		}
		sendByte(0x02);
		sendByte(0x02);
		sendBit(1);		//stop bit
		pinMode(LINE, INPUT);
	}

	//send 8 byte response to read cmd
	static inline void rspRead() {
		pinMode(LINE, OUTPUT);
		for (uint8_t i = 0; i < 8; i++) {
			sendByte(buffer[i]);
		}
		sendBit(1);		//stop bit
		pinMode(LINE, INPUT);
	}

	//sends one byte to line. 8 bits in 1 byte
	static inline void sendByte(uint8_t byt) {
		for (uint8_t i = 0; i < 8; i++) {
			sendBit(byt & 0x80);
			byt <<= 1;
		}
	}

	//1 bit takes 4us to read / write. if LINE is low for < 2us
	//that bit is a 1, otherwise it is a 0
	static inline void sendBit(bool bt) {
		if (bt) {
			digitalWriteFast(LINE, LOW);
			delayMicroseconds(1);
			digitalWriteFast(LINE, HIGH);
			delayMicroseconds(3);
		}
		else {
			digitalWriteFast(LINE, LOW);
			delayMicroseconds(3);
			digitalWriteFast(LINE, HIGH);
			delayMicroseconds(1);
		}
	}

	//reads LINE for 1 byte and returns it. 8 bits in 1 byte.
	static inline uint8_t getByte() {
		uint8_t byt;
		for (uint8_t i = 0; i < 8; i++) {
			byt = (byt << 1) | getBit();
		}
		return byt;
	}

	//1 bit takes 4us to read / write. if LINE is low for < 2us
	//that bit is a 1, otherwise it is a 0
	static inline bool getBit() {
		uint32_t fallCycle;
		//wait for line to drop, signaling the start of a new bit
		while (digitalReadFast(LINE));
		//record the cpu cycle where the line goes low
		fallCycle = CYCLE;
		//wait for LINE to rise
		while (!digitalReadFast(LINE));
		return(CYCLE - fallCycle) < HALF;
	}

	//waits until LINE is idle high for 8us after reading a bit
	//this ensures we dont start reading in the middle of a cmd
	static inline void wait() {
		uint32_t riseCycle;
		bool isIdle = false;
		while (!isIdle) {
			//we need to start waiting after the last bit in a cmd
			//so that we dont do our timing right before a cmd starts
			getBit();
			//wait for line to go high
			while (!digitalReadFast(LINE));
			riseCycle = CYCLE;
			while (digitalReadFast(LINE)) {
				//gamecube controller data line is idle high
				//one bit takes ~4us to send, so if LINE is high
				//for 8us, we know that it must be idle
				isIdle = (CYCLE - riseCycle) > TWO;
				if (isIdle) { break; }
			}
		}
	}

	//sets up cpu cycle counter register
	void initCounter() {
		ARM_DEMCR |= ARM_DEMCR_TRCENA;
		ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	}

	//sets pinmodes for outputs
	void initOut() {
		for (uint8_t i = 0; i < 4; i++) {
			pinMode(i, OUTPUT);
		}
	}

	//sets pinmodes for inputs
	void initIn() {
		for (uint8_t i = 4; i < 17; i++) {
			pinMode(i, INPUT_PULLUP);
		}
	}
};

DogeBawx bawx;

void setup() {
	bawx.init();
}

void loop() {
	cli();
	while (1) {
		bawx.read();
		digitalWriteFast(YLED, HIGH);
		bawx.write();
		digitalWriteFast(RLED, HIGH);
	}
}
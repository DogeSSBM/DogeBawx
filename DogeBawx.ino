#define LINE 14 //gamecube controller data line pin
#define led 15
#define ledOn digitalWriteFast(led, HIGH)
#define ledOff digitalWriteFast(led, LOW)

//set your button binds to their corrosponding pin here
#define BTN_S  8
#define BTN_Y  9
#define BTN_X  10
#define BTN_B  11
#define BTN_A  12
#define BTN_L  5
#define BTN_R  6
#define BTN_Z  7
#define BTN_DU 0
#define BTN_DD 0
#define BTN_DL 0
#define BTN_DR 0
#define A_U 1
#define A_D 2
#define A_L 3
#define A_R 4
#define C_U 0
#define C_D 0
#define C_L 0
#define C_R 0
#define T_L 0
#define T_R 0

#define CYCLE ARM_DWT_CYCCNT //cpu cycle counter. increments every cpu cycle

uint32_t oldCYCLE;	//temp timing var 
						//(global to reduce memory alocation overhead)

//one bit takes 4us to send
const uint32_t QUARTER = clockCyclesPerMicrosecond();
const uint32_t HALF = QUARTER + QUARTER;
const uint32_t THREEQ = HALF + QUARTER;
const uint32_t ONE = THREEQ + QUARTER;
const uint32_t TWO = ONE + ONE;

//cmd from console. cmd[0] is the actual cmd. (if applicable) cmd[1]
//is the read mode. (if applicable) cmd[2] is rumble info
uint8_t cmd[3] = { 0, 0, 0 };

//id of the device to emulate
const uint8_t id[3] = { 0b10010000u,0b00000000u,0b00000000u };

//buffer of bytes to be sent to the gamecube
//we should not ever need to send more than 10 bytes
uint8_t buffer[10] = {
	(0b00000100u),(0b00000001u),(0b10000000u),(0b10000000u),
	(0b10000000u),(0b10000000u),(0b00000001u),(0b00000001u),
	(0b00000000u),(0b00000000u)
};

//sets buttons pinMode() to INPUT_PULLUP, and LINE to INPUT
static void initPins() {
	//buttons
	for (uint8_t i = 0; i < 13; i++) {
		pinMode(i, INPUT_PULLUP);
	}
	pinMode(led, OUTPUT);
	/*
	//make sure LINE does not end up in INPUT_PULLUP
	digitalWriteFast(LINE, LOW);
	*/
	//set LINE to INPUT by default
	pinMode(LINE, INPUT);
}

//reads digital buttons (assumes pinMode is INPUT_PULLUP)
//and packs them into buffer
static inline void updateBuffer() {
	buffer[0] =
		(0 << 0) |	/*error status bit*/
		(0 << 1) |	/*error latch bit*/
		(0 << 2) |	/*origin bit*/
		((!digitalReadFast(BTN_S)) << 3) |
		((!digitalReadFast(BTN_Y)) << 4) |
		((!digitalReadFast(BTN_X)) << 5) |
		((!digitalReadFast(BTN_B)) << 6) |
		((!digitalReadFast(BTN_A)) << 7);

	buffer[1] =
		(1 << 0) |	/*always high bit*/
		((!digitalReadFast(BTN_L )) << 1) |
		((!digitalReadFast(BTN_R )) << 2) |
		((!digitalReadFast(BTN_Z )) << 3) |
		((!digitalReadFast(BTN_DU)) << 4) |
		((!digitalReadFast(BTN_DD)) << 5) |
		((!digitalReadFast(BTN_DR)) << 6) |
		((!digitalReadFast(BTN_DL)) << 7);

	//analog stick x axis byte
	buffer[2] = 128u;
	if (!digitalReadFast(A_L)) {
		buffer[2] = 0u;
	}
	else if(!digitalReadFast(A_R)) {
		buffer[2] = 255u;
	}

	buffer[3] = 128u;
	//analog stick y axis byte
	if (!digitalReadFast(A_U)) {
		buffer[3] = 0u;
	}
	if (!digitalReadFast(A_D)) {
		buffer[3] = 255u;
	}
	
	//c stick x axis byte
	buffer[4] = 128u;
	if (!digitalReadFast(C_L)) {
		buffer[4] = 0u;
	}
	if (!digitalReadFast(C_R)) {
		buffer[4] = 255u;
	}

	//c stick y axis byte
	buffer[5] = 128u;
	if (!digitalReadFast(C_L)) {
		buffer[5] = 0u;
	}
	if (!digitalReadFast(C_R)) {
		buffer[5] = 255u;
	}

	//left trigger analog press byte
	buffer[6] = (!digitalReadFast(T_L)) ? 255 : 0;

	//right trigger analog press byte
	buffer[7] = (!digitalReadFast(T_R)) ? 255 : 0;

	//required for rspOrigin() for some reason
	//TODO: figure out the purpose of these bytes
	buffer[8] = 0; buffer[9] = 0;
}

//response to send in case of read cmd
static inline void rspRead() {
	//console expects an 8 byte response
	for (uint8_t i = 0; i < 8; i++) {
		sendByte(buffer[i]);
	}
}

//response to send in case of origin cmd
static inline void rspOrigin() {
	//console expects a 10 byte response
	for (uint8_t i = 0; i < 10; i++) {
		sendByte(buffer[i]);
	}
}

//response to send in case of init cmd
static inline void rspInit() {
	//console expects a 3 byte response
	for (uint8_t i = 0; i < 3; i++) {
		sendByte(id[i]);
	}
}

//reads cmd from console.
//responds based on the cmd.
static inline void readWrite() {
	updateBuffer();
	cmd[0] = getByte();
	//most likely
	if (cmd[0] == 0x40) {
		//read cmd has two additional bytes
		cmd[1] = getByte();
		cmd[2] = getByte();
		getStop();		//stop bit
		ledOn;
		//pinMode() needs to be OUTPUT mode to write data to LINE
		pinMode(LINE, OUTPUT);
		rspRead();
		//TODO: figure out what to do in case of no stop bit
	}

	//origin is 1 one byte
	else if (cmd[0] == 0x41) {
		getStop();//stop bit
		//pinMode() needs to be OUTPUT mode to write data to LINE
		pinMode(LINE, OUTPUT);
		rspOrigin();
		//TODO: figure out what to do in case of no stop bit
	}

	//init cmd is 1 byte
	else if (cmd[0] == 0x00 || cmd[0] == 0xff) {
		getStop();//stop bit
		//pinMode() needs to be OUTPUT mode to write data to LINE
		pinMode(LINE, OUTPUT);
		rspInit();
		//TODO: figure out what to do in case of no stop bit
	}
	else {
		//TODO: figure out what to do in case of unknown cmd
		pinMode(LINE, OUTPUT);
		rspRead();
	}
	//send controller stop bit
	sendStop();
	//pinMode() set to INPUT by default to lower cmd reading overhead
	pinMode(LINE, INPUT);
}

//reads until 1 bit is read, then returns the bit
//(assumes pin is already INPUT)
static inline bool getBit() {
	while (digitalReadFast(LINE)) {
		//wait for line to drop, signaling the start of a new bit
	}
	//record the cpu cycle where the line goes low
	oldCYCLE = CYCLE;
	while (!digitalReadFast(LINE)) {
		//the ammount of time LINE spends LOW determines the value of the current bit
	}
	//1 bit takes 4us to read / write. if LINE is low for < 2us the bit is a 1, otherwise it is a 0
	return(CYCLE - oldCYCLE) < HALF;
}

//reads for console stop bit (3us or THREEQ)
//(assumes pin is already INPUT)
static inline bool getStop() {
	while (digitalReadFast(LINE)) {
		//wait for line to drop, signaling the start of a new bit
	}
	//record the cpu cycle where the line goes low
	oldCYCLE = CYCLE;
	while (!digitalReadFast(LINE)) {
		//the ammount of time LINE spends LOW determines the value of the current bit
	}
	//similar to a 1, but total time is THREEQ instead of ONE
	return(CYCLE - oldCYCLE) < HALF;
}

//reads LINE for 1 byte and returns it. 8 bits in 1 byte.
//(assumes pin is already INPUT)
static inline uint8_t getByte() {
	uint8_t byt;
	for (uint8_t i = 0; i < 8; i++) {
		//shifts byt over by 1 bit to make room for the next then
		//bitwise or with the bit read from LINE
		byt = (byt << 1) | getBit();
	}
	return byt;
}

//writes a controller stop bit to LINE (assumes pin is already OUTPUT)
static inline void sendStop() {
	//write LOW for 2us
	digitalWriteFast(LINE, LOW);
	oldCYCLE = CYCLE;
	while ((CYCLE - oldCYCLE) < HALF) {
		/*wait for 1/2 bit*/
	}

	//write HIGH for 2us
	digitalWriteFast(LINE, HIGH);
	oldCYCLE = CYCLE;
	while ((CYCLE - oldCYCLE) < HALF) {
		/*wait for 1/2 bit*/
	}
}

//writes a 1 to LINE (assumes pin is already OUTPUT)
static inline void send1() {
	//write LOW for 1us
	digitalWriteFast(LINE, LOW);
	oldCYCLE = CYCLE;
	while ((CYCLE - oldCYCLE) < QUARTER) {
		/*wait for 1/4 bit*/
	}

	//write HIGH for 3us
	digitalWriteFast(LINE, HIGH);
	oldCYCLE = CYCLE;
	while ((CYCLE - oldCYCLE) < THREEQ) {
		/*wait for 3/4 bit*/
	}
}

//writes a 0 to LINE (assumes pin is already OUTPUT)
static inline void send0() {
	//write LOW for 3us
	digitalWriteFast(LINE, LOW);
	oldCYCLE = CYCLE;
	while ((CYCLE - oldCYCLE) < THREEQ) {
		/*wait for 3/4 bit*/
	}

	//write HIGH for 1us
	digitalWriteFast(LINE, HIGH);
	oldCYCLE = CYCLE;
	while ((CYCLE - oldCYCLE) < QUARTER) {
		/*wait for 1/4 bit*/
	}
}

//writes byt (least significant bit first) to LINE
//(assumes pin is already OUTPUT)
static inline void sendByte(uint8_t byt) {
	for (uint8_t i = 0; i < 8; i++) {
		//sends the least significant bit
		byt & 0b1u ? send1() : send0();
		//shifts the significant bit out
		byt = byt >> 1;
	}
}

//waits until LINE is idle high for 8us after reading a bit
//this ensures we dont start reading in the middle of a cmd
static inline void wait() {
	while (1) {
		while (!getBit()) {
			//we need to start waiting after the last bit in a
			//cmd so that we dont start and finish our timing
			//right before a new cmd starts. cmds always end
			//with a 1. so we wait for that.
		}
		oldCYCLE = CYCLE;
		while (digitalReadFast(LINE)) {
			//gamecube controller data line is idle high
			//one bit takes ~4us to send, so if LINE is high
			//for 8us, we know that it must be idle
			if (CYCLE - oldCYCLE > TWO) { return; }
		}
	}
}

void setup() {
	//disable hardware interupts to prevent isr overhead
	cli();
	//set up cpu cycle counter register
	ARM_DEMCR |= ARM_DEMCR_TRCENA;
	ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	//sets pinMode() for all pins to the correct mode
	initPins();
	//ensures cmd read is aligned
	wait();
}

uint8_t cnt;
void loop() {/*avoid overhead*/while (1) {
	readWrite();
}}
//typedef shortcuts (ignore these)
#define u8 uint8_t
#define u16 uint16_t
#define u32 uint32_t
#define u64 uint64_t

#define LINE 23						// gamecube controller data line pin
const u8 id[3] = { 0x09,0x00,0x00 };		// ID to send in response to
								// init CMD
// buffer to be sent to console
// last 2 bytes only sent in response to origin CMD (must be 0x02 for some reason)
u8 buff[10] = { 0x00,0x80,0x80,0x80,0x80,0x80,0x01,0x01,0x02,0x02 };

// one bit takes 4us to read/write
// quarterBit = number of CPU cycles in 1us
const u32 _quarterBit = F_CPU / 1000000;		// 1/4 bit takes this number of
								// cycles to read/write
const u32 _halfBit = _quarterBit * 2;		// 1/2 bit takes this number of
								// cycles to read/write
const u32 _threeqBit = _halfBit + _quarterBit;	// 3/4 bit takes this number of
								// cycles to read/write
const u32 _oneBit = _halfBit * 2;			// 1 bit takes this number of
								// cycles to read/write
const u32 _twoBit = _oneBit * 2;			// 2 bits take this number of
								// cycles to read/write
u32 tempCycle;						// temp var to store CPU cycle
#define cycReg (ARM_DWT_CYCCNT)			// CPU Cycle count Register
#define markCycle() (tempCycle = cycReg)		// record the current CPU cycle
								// for use later
#define elapsedCycles() (cycReg - tempCycle)	// number of CPU cycles elapsed
								// since markCycle() was recorded

// CMD read from console
struct {
	u8 cmd;					// CMD from console
	u8 readMode;				// appears after read CMD (0x40)
	u8 rumbleInfo;				// appears after readMode, after read CMD
	bool unknownCmd;				// TRUE if CMD matches no known CMDs
							// remains TRUE, until reset by user
}cmd;

// reads and returns one bit from LINE
// (assumes LINE is already INPUT)
inline bool readBit()
{
	while (digitalReadFast(LINE)) {
		// start of bit starts on falling edge
	}
	markCycle();			//record CPU cycle of falling edge
	while (!digitalReadFast(LINE)) {
		// bit value is determined by how long LINE is held LOW by the console
	}
	// 1 if LINE was LOW for < halfBit
	// 0 if LINE was LOW for > halfBit
	return elapsedCycles() < _halfBit;
}

// writes one bit to LINE
// (assumes LINE is already OUTPUT)
inline void writeBit(bool bt /* bit to be written */)
{
	markCycle();	// record CPU cycle of the start of the bit
	if (bt) {
		digitalWriteFast(LINE, LOW);
		while (elapsedCycles() < _quarterBit) {
			// hold LINE LOW for 1/4 (1us) of the bit
		}
		digitalWriteFast(LINE, HIGH);
		while (elapsedCycles() < _oneBit) {
			// hold LINE HIGH for the rest of the bit
		}
		return;
	}
	else {
		digitalWriteFast(LINE, LOW);
		while (elapsedCycles() < _threeqBit) {
			// hold LINE LOW for 3/4 (3us) of the bit
		}
		digitalWriteFast(LINE, HIGH);
		while (elapsedCycles() < _oneBit) {
			// hold LINE HIGH for the rest of the bit
		}
		return;
	}
}

// writes one byte to LINE (MSB order)
// (assumes LINE is already OUTPUT)
inline void writeByte(u8 byt /* byte to be written */)
{
	for (uint8_t i = 0; i < 8; i++) {
		writeBit(byt & 0x80);
		byt <<= 1;
	}
}

// reads and returns one byte from LINE
// (assumes LINE is already INPUT)
inline u8 readByte()
{
	u8 byt;		// byte being read
				// Console sends MSB first
	for (u8 i; i < 8; i++) {
		byt = (byt << 1) | readBit();
	}
	return byt;
}

// waits for almost 2 init probes
inline void align()
{
	u16 buffer = 0xffff;				// temp buffer to store bits
	delay(100);						// delay long enough for console to
								// probe for attached controller
	pinMode(LINE, INPUT);
	// wait for buffer to match nearly 2 probes
	while (buffer != 0b0000001000000001) {
		buffer = (buffer << 1) | readBit();
	}
}

// resds CMD and any extra info (if expected) from LINE
// sets LINE to INPUT
// sets cmd.unknownCmd to true in case of read error or unknown cmd
void getCmd()
{
	cmd.cmd = readByte();

	// read CMD has extra info
	if (cmd.cmd == 0x40) {
		cmd.readMode = readByte();
		cmd.rumbleInfo = readByte();
		readBit();		//stopbit
		return;
	}

	// origin CMD, init CMD, and reset CMD are all one byte
	else if (cmd.cmd == 0x41 || cmd.cmd == 0x00 || cmd.cmd == 0xff) {
		readBit();
		return;
	}
}

// button pins
/*
S  Y  X  B
0  1  2  3

A  L  R  Z
4  5  6  7

Du Dd Dr Dl
8  9  10 11

Al Ar Au Ad
12 22 14 15

Cl Cr Cu Cd
16 17 18 19

La Ra
20 21
*/

// reads physical buttons and stores states in buffer
// (assumes button pin modes are already INPUT)
inline void read()
{
	// buttons first byte
	buff[0] = 0 |
		((!digitalReadFast( 0)) << 4) |	// Start
		((!digitalReadFast( 1)) << 3) |	// Y
		((!digitalReadFast( 2)) << 2) |	// X
		((!digitalReadFast( 3)) << 1) |	// B
		((!digitalReadFast( 4)) << 0) ;	// A
	
	// buttons second byte
	buff[1] = 0 | 
		((!digitalReadFast( 5)) << 6) |	// L
		((!digitalReadFast( 6)) << 5) |	// R
		((!digitalReadFast( 7)) << 4) |	// Z
		((!digitalReadFast( 8)) << 3) |	// Dpad-Up
		((!digitalReadFast( 9)) << 2) |	// Dpad-Down
		((!digitalReadFast(10)) << 1) |	// Dpad-Right
		((!digitalReadFast(11)) << 0) ;	// Dpad-Left

	// analog stick X axis byte
	if (!digitalReadFast(12)) {					// A-Left
		buff[2] = !digitalReadFast(22) ? 128 : 0;		// A-Right
	}
	else {
		buff[2] = !digitalReadFast(22) ? 255 : 128;	// A-Right
	}
	// analog sick Y axis byte
	if (!digitalReadFast(14)) {					// A-Up
		buff[3] = !digitalReadFast(15) ? 128 : 255;	// A-Down
	}
	else {
		buff[3] = !digitalReadFast(15) ? 0 : 128;		// A-Down
	}

	// C stick X axis byte
	if (!digitalReadFast(16)) {					// C-Left
		buff[4] = !digitalReadFast(17) ? 128 : 0;		// C-Right
	}
	else {
		buff[4] = !digitalReadFast(17) ? 255 : 128;	// C-Right
	}
	// C sick Y axis byte
	if (!digitalReadFast(18)) {					// C-Up
		buff[5] = !digitalReadFast(19) ? 128 : 255;	// C-Down
	}
	else {
		buff[5] = !digitalReadFast(19) ? 0 : 128;		// C-Down
	}

	// Left trigger analog byte
	buff[6] = !digitalReadFast(20) ? 170 : 1;

	// Right trigger analog byte
	buff[7] = !digitalReadFast(21) ? 85 : 1;
}

// sends response based on CMD
inline void respond()
{
	// read CMD
	if (cmd.cmd == 0x40) {
		for (uint8_t i = 0; i < 8; i++) {
			writeByte(buff[i]);
		}
	}

	// origin CMD
	else if (cmd.cmd == 0x41) {
		buff[0] &= 0b11111011;				//set origin bit
		for (uint8_t i = 0; i < 10; i++) {
			writeByte(buff[i]);
		}
	}

	// init / reset CMD
	else if (cmd.cmd == 0x00 || cmd.cmd == 0xff) {
		for (uint8_t i = 0; i < 3; i++) {
			writeByte(id[i]);
		}
	}

	writeBit(1);					// stop bit
}

// reads CMD from console and responds
inline void write()
{
	cli();						// disable interrupts
								// (timing is critical for this function)
	pinMode(LINE, INPUT);
	getCmd();						// read CMD
	pinMode(LINE, OUTPUT);
	respond();						// respond to CMD
	sei();						// enable interrupts
								// (end of timeing critical code)
}

// sets up cpu cycle counter register
inline void initCounter()
{
	ARM_DEMCR |= ARM_DEMCR_TRCENA;
	ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
}

// sets all button pin modes to INPUT
// assumes all buttons are on pins lower than LINE
inline void initBtns()
{
	for (uint8_t i = 0; i < LINE; i++) {
		pinMode(i, INPUT_PULLUP);
	}
	pinMode(LINE, INPUT);
}

// gets everything ready				// sets up cpu cycle counter register
inline void init()
{
	initCounter();					// sets up cpu cycle counter register
	initBtns();						// sets all button pin modes to INPUT
								// assumes all buttons are on pins < LINE
	align();						// waits for almost 2 init probes
	write();						// reads CMD from console and responds
}

void setup()
{
	/*Serial.begin(9600);
	Serial.println("Hi number 1");
	delay(1000);
	Serial.println("Hi number 2\n");*/
	init();
}

void loop()
{
	read();						// read physical buttons and 
								// stores states in buffer
	write();						// writes buffer to console
}

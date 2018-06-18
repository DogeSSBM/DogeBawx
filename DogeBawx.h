#pragma once
#include "bawxTypeMacros.h"
#include "bawxBackendMacros.h"
#include "bawxButtonDefines.h"

union Buttons{
	u8 arr[10];
	struct {
		// byte 0
		u8 A : 1;
		u8 B : 1;
		u8 X : 1;
		u8 Y : 1;
		u8 S : 1;
		u8 orig : 1;
		u8 errL : 1;
		u8 errS : 1;
		
		// byte 1
		u8 Dl : 1;
		u8 Dr : 1;
		u8 Dd : 1;
		u8 Du : 1;
		u8 Z : 1;
		u8 R : 1;
		u8 L : 1;
		u8 high : 1;

		//byte 2-7
		u8 Ax : 8;
		u8 Ay : 8;
		u8 Cx : 8;
		u8 Cy : 8;
		u8 La : 8;
		u8 Ra : 8;

		// magic byte 8 & 9 (only used in origin cmd)
		// have something to do with rumble motor status???
		// ignore these, they are magic numbers needed
		// to make a cmd response work
		u8 magic1 : 8;
		u8 magic2 : 8;
	};
}btn;

union ID {
	u8 arr3[3] = { 0x09,0x00,0x00 };
	struct {
		u8 byt1 : 8;
		u8 byt2 : 8;
		u8 byt3 : 8;
	};
}id;

struct CMD {
	u8 command : 8;
	u8 readmode : 8;
	u8 rumbleInfo : 8;
}cmd;

class DogeBawx {
public:
	static inline void init()
	{
		cli();
		initCounter();
		initBtns();
		initLines();
		align();
	}
	
	static inline void readWrite()
	{
		readBtns();
		readCmd();
		write();
	}
private:
	// sets up cpu cycle counter register
	static inline void initCounter()
	{
		ARM_DEMCR |= ARM_DEMCR_TRCENA;
		ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
	}

	static inline void initGroup(u8 group) 
	{
		for (uint8_t i = 0; i < 5; i++) {
			pinMode((i+group), INPUT_PULLUP);
		}
	}

	static inline void initBtns()
	{
		initGroup(GL);
		initGroup(GM);
		initGroup(GR);
		btn.high = 0b1u;
		btn.orig = 0b0u;
		btn.errL = 0b0u;
		btn.errS = 0b0u;
		btn.Ax = 0x80u;
		btn.Ay = 0x80u;
		btn.Cx = 0x80u;
		btn.Cy = 0x80u;
		btn.magic1 = 0x02u;
		btn.magic2 = 0x02u;
		pinMode(HP, INPUT_PULLUP);
	}
	
	// sets pin modes for gamecube controller data line
	// and screen rx/tx
	static inline void initLines()
	{
		pinMode(LINE, INPUT);
		//pinMode(STX, OUTPUT);
		//pinMode(SRX, INPUT);
	}

	// reads and returns one bit from LINE
	// (assumes LINE is already INPUT)
	static inline bool readBit()
	{
		while (digitalReadFast(LINE)) {
			// start of bit starts on falling edge
		}
		markCycle();// record CPU cycle of falling edge
		while (!digitalReadFast(LINE)) {
			// bit value is determined by how long LINE is held LOW by the console
		}
		// 1 if LINE was LOW for < halfBit
		// 0 if LINE was LOW for > halfBit
		return elapsedCycles() < _halfBit;
	}
	
	// writes one bit to LINE
	// (assumes LINE is already OUTPUT)
	static inline void writeBit(bool bt /* bit to be written */)
	{
		markCycle();// record CPU cycle of the start of the bit
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
	
	// writes one controller stop bit to LINE
	// (assumes LINE is already OUTPUT)
	static inline void writeStop() {
		markCycle();
		digitalWriteFast(LINE, LOW);
		while (elapsedCycles() < _halfBit) {
			// hold LINE LOW for 1/2 (2us) of the bit
		}
		digitalWriteFast(LINE, HIGH);
		while (elapsedCycles() < _halfBit) {
			// hold LINE HIGH for 1/2 (2us) of the bit
		}
		return;
	}

	// writes one byte to LINE (MSB order)
	// (assumes LINE is already OUTPUT)
	static inline void writeByte(u8 byt /* byte to be written */)
	{
		for (uint8_t i = 0; i < 8; i++) {
			writeBit(byt & 0x80u);
			byt <<= 1;
		}
	}

	// waits for almost 2 init cmds + stop bits to ensure
	// the next command is aligned with the first command read
	static inline void align() 
	{
		u16 buffer=0xffff;
		delay(100);
		while (buffer!=0b0000001000000001) {
			buffer = (buffer << 1) | readBit();
		}
	}
public:
	// attempts to read a console command from LINE
	// one cmd will either be 8bits or 24bits (not including stop bit)
	// returns the first byte of the command
	// stores entire command in the cmd struct
	// sets LINE to pinmode INPUT
	static inline void readCmd()
	{
		pinMode(LINE, INPUT);
		for (uint8_t i = 0; i < 8; i++) {
			cmd.command = (cmd.command << 1) | readBit();
		}
		if (cmd.command==0x41||cmd.command==0x00||cmd.command == 0xff) {			// stop bit
			readBit();			// stop bit
			return;
		}
		else {
			for (uint8_t i = 0; i < 8; i++) {
				cmd.readmode = (cmd.readmode << 1) | readBit();
			}
			for (uint8_t i = 0; i < 8; i++) {
				cmd.rumbleInfo = (cmd.rumbleInfo << 1) | readBit();
			}
			readBit();			// stop bit
			return;
		}
	}
	
	// writes a response to LINE based on the contents of cmd
	static inline void write()
	{
		pinMode(LINE, OUTPUT);

		// read cmd
		if (cmd.command == 0x40) {
			for (uint8_t i = 0; i < 8; i++) {
				writeByte(btn.arr[i]);
			}
			writeStop();
		}

		// origin cmd
		else if (cmd.command == 0x41) {
			btn.orig = 0b0u;
			for (uint8_t i = 0; i < 10; i++) {
				writeByte(btn.arr[i]);
			}
			writeStop();
		}

		// init / reset cmd
		else if (cmd.command == 0x00 || cmd.command == 0xff) {
			for (uint8_t i = 0; i < 3; i++) {
				writeByte(id.arr3[i]);
			}
			writeStop();
		}
		pinMode(LINE, INPUT);
	}

	// reads button state and stores in btn struct/array
	// assumes all button pinmodes are INPUT_PULLUP
	static inline void readBtns() 
	{
		// digital buttons
		btn.A = readBtn(btn_A);
		btn.B = readBtn(btn_B);
		btn.X = readBtn(btn_X);
		btn.Y = readBtn(btn_Y);
		btn.L = readBtn(btn_L);
		btn.R = readBtn(btn_R);
		btn.Z = readBtn(btn_Z);
		btn.S = readBtn(btn_S);
		btn.Du = readBtn(btn_Du);
		btn.Dd = readBtn(btn_Dd);
		btn.Dl = readBtn(btn_Dl);
		btn.Dr = readBtn(btn_Dr);

		// trigger light press DAC
		btn.La = readBtn(btn_La) ? 0x80u : 0x00u;
		btn.Ra = readBtn(btn_Ra) ? 0x80u : 0x00u;

		// Analog stick X axis
		if (readBtn(btn_Al)) {
			btn.Ax = readBtn(btn_Ar) ? 0x80u : 0x00u;
		}
		else if (readBtn(btn_Ar)) {
			btn.Ax = 0xffu;
		}
		else {
			btn.Ax = 0x80u;
		}
		
		// Analog stick Y axis
		if (readBtn(btn_Ad)) {
			btn.Ay = readBtn(btn_Au) ? 0x80u : 0x00u;
		}
		else if (readBtn(btn_Au)) {
			btn.Ay = 0xffu;
		}
		else {
			btn.Ay = 0x80u;
		}

		// C stick X axis
		if (readBtn(btn_Cl)) {
			btn.Cx = readBtn(btn_Cr) ? 0x80u : 0x00u;
		}
		else if (readBtn(btn_Cr)) {
			btn.Cx = 0xffu;
		}
		else {
			btn.Cx = 0x80u;
		}

		// C stick Y axis
		if (readBtn(btn_Cd)) {
			btn.Cy = readBtn(btn_Cu) ? 0x80u : 0x00u;
		}
		else if (readBtn(btn_Cu)) {
			btn.Cy = 0xffu;
		}
		else {
			btn.Cy = 0x80u;
		}
	

	}
};
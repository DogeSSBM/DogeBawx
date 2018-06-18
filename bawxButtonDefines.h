#pragma once

// data line pin
#define LINE 23

#define readBtn(button) (!digitalReadFast(button))

// screen recive and transmit pins
#define SRX 0
#define STX 1

// 3 groups of 5 buttons
// first pin of each
#define GL 2	// left group
#define GM 7	// middle group
#define GR 14	// right group

// always high pin for unused action binds
#define HP 19

// BIND YOUR BUTTONS HERE
#define btn_A	(GR+3)
#define btn_B	(GR+2)
#define btn_X	(GR+1)
#define btn_Y	(HP)
#define btn_Z	(GR+4)
#define btn_L	(HP)
#define btn_R	(GR)
#define btn_S	(GM+2)
#define btn_Du	(HP)
#define btn_Dd	(HP)
#define btn_Dl	(HP)
#define btn_Dr	(HP)
#define btn_Au	(GL)
#define btn_Ad	(GL+2)
#define btn_Al	(GL+1)
#define btn_Ar	(GL+3)
#define btn_Cu	(GM)
#define btn_Cd	(GM+4)
#define btn_Cl	(GM+1)
#define btn_Cr	(GM+3)
#define btn_La	(HP)
#define btn_Ra	(GL+4)
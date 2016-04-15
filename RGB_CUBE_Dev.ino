////////////////////////////////////////////////////////////////////////////////////
// PREPROCESSOR DIRECTIVES

// Libraries
#include <SPI.h>
#include <DueTimer.h>

// Header files
#include "CoreFunctions.h"
#include "DarrahAnimations.h"

// Define pin constants
#define OE_pin 53 // Output enable pin, turns entire cube off when HIGH.
#define SS_pin 52 // Slave select pin

////////////////////////////////////////////////////////////////////////////////////
// PREAMBLE

int anodelevel = 0;					// Current anode level which we are updating
int BAM_Bit, BAM_Counter = 0;		// Bit Angle Modulation variables

byte anode[8];		// Array of bytes that are sent to the anode shift register.

// Arrays of bytes which are written to the cathode shift registers for each individual colour
byte red0[64], red1[64], red2[64], red3[64] = { 0 };
byte blue0[64], blue1[64], blue2[64], blue3[64] = { 0 };
byte green0[64], green1[64], green2[64], green3[64] = { 0 };

//////////////////////////////////////////////////////////////////////////////////////
// SETUP

void setup()
{
	// Set up output pins 
	pinMode(OE_pin, OUTPUT);			// Shift registers are active low, therefore pull high to disable cube
	digitalWrite(OE_pin, HIGH);			// Stop cube from lighting up at startup
	pinMode(SS_pin, OUTPUT);			// Latch pin / Slave select pin.

	// Assign values to be given to the anode register. 
	anode[0] = B00000001;				// Only level 0 can light up.
	anode[1] = B00000010;				// Level 1, etc.....
	anode[2] = B00000100;
	anode[3] = B00001000;
	anode[4] = B00010000;
	anode[5] = B00100000;
	anode[6] = B01000000;
	anode[7] = B10000000;

	// SPI setup
	SPI.setBitOrder(SS_pin, MSBFIRST);	// Most significant bit of each byte is shifted out first.
	SPI.setDataMode(SS_pin, SPI_MODE0); // Mode 0 -> Rising edge of data, keep clock low
	SPI.setClockDivider(SS_pin, 8);		// 84Mhz / 21 = 4Mhz

	SPI.begin(SS_pin);					// Begin hardware SPI

	// Set up and start timer
	Timer3.attachInterrupt(interruptTransfer);		// Use "DueTimer" library function to attach the function "interruptTransfer" to the interrupt.
	Timer3.start(125);								// 125 uS interrupt frequency, so 8 interrupts(1 for each layer) takes 1mS, so an overall refresh rate of 1000Hz

	digitalWrite(OE_pin, LOW);						// Enables all the outputs.
}

/////////////////////////////////////////////////////////////////////////////////////
// LOOP

void loop()
{
	for (int k = 0; k < 8; k++)
	{
		for (int j = 0; j < 8; j++)
		{
			for (int i = 0; i < 8; i++)
			{
				LED(i, j, k, 15, 15, 15);
			}
		}
	}

	// fixed ////////////////////
	//rainversiontwo();
	//folder(); 
	//wipe_out();
	//sinwavetwo();
	//clean();
	//bouncyvtwo();
	//color_wheeltwo();
}


void LED(int level, int row, int column, byte red, byte green, byte blue)
{
	/*
	This function takes inputs for the desired location and brighness for a LED to turn on within the cube. It first checks that these inputs are actually physically within the cube. Then, it writes to the arrays the hold the current status of every LED in the cube. These arrays are red0,red1,red2,red3,green0,green1 etc.

	This function is capable of changing the holding arrays to whatever pattern is required. It only changes one LED per function call. Multiple calls to this function can change LED's accross the cube and create any manner of pattern.
	*/

	// Constrain inputs to be within the range of the cube //////////////////////////////////////////////////////////////////////

	// location
	if (level<0)  level = 0;
	if (level>7)  level = 7;
	if (row<0)    row = 0;
	if (row>7)	  row = 7;
	if (column<0) column = 0;
	if (column>7) column = 7;

	// brightness
	if (red<0)	  red = 0;
	if (red>15)	  red = 15;
	if (green<0)  green = 0;
	if (green>15) green = 15;
	if (blue<0)   blue = 0;
	if (blue>15)  blue = 15;

	// Write desired LED location and brightness to the arrays ///////////////////////////////////////////////////////////////////

	int whichLED = (level * 64) + (row * 8) + column;			// Calculate the LED number within the whole cube (0 - 511)
	int whichByte = (level * 8) + row;							// Which byte is the desired LED contained in (out of 0-63)
	int whichBit = whichLED - (whichByte * 8);					// Which bit represents it within that byte (out of 0-7)

	// Write to the arrays corresponding to the whichByte and whichBit results
	bitWrite(red0[whichByte], whichBit, bitRead(red, 0));
	bitWrite(red1[whichByte], whichBit, bitRead(red, 1));
	bitWrite(red2[whichByte], whichBit, bitRead(red, 2));
	bitWrite(red3[whichByte], whichBit, bitRead(red, 3));

	bitWrite(green0[whichByte], whichBit, bitRead(green, 0));
	bitWrite(green1[whichByte], whichBit, bitRead(green, 1));
	bitWrite(green2[whichByte], whichBit, bitRead(green, 2));
	bitWrite(green3[whichByte], whichBit, bitRead(green, 3));

	bitWrite(blue0[whichByte], whichBit, bitRead(blue, 0));
	bitWrite(blue1[whichByte], whichBit, bitRead(blue, 1));
	bitWrite(blue2[whichByte], whichBit, bitRead(blue, 2));
	bitWrite(blue3[whichByte], whichBit, bitRead(blue, 3));
}

void interruptTransfer()
{
	// BAM counting system, increments the bit after 8, 16, 32 and 64 interrupts.
	if (BAM_Counter == 8){ BAM_Bit++; }
	if (BAM_Counter == 24){ BAM_Bit++; }
	if (BAM_Counter == 56){ BAM_Bit++; }

	// SPI TRANSFERS /////////////////////////////////////////////////////////////////////////////////////////////////////

	// Transfer anode byte first, as anode register is at the far end of chain
	SPI.transfer(SS_pin, anode[anodelevel], SPI_CONTINUE);

	// Choose which set of arrays to transfer, depending on the BAM_Bit
	switch (BAM_Bit)
	{
		case 0:
		{
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, blue0[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, green0[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 7; shift_byte++)
			{
				SPI.transfer(SS_pin, red0[shift_byte], SPI_CONTINUE);
			}
			SPI.transfer(SS_pin, red0[(anodelevel * 8) + 7]); // no continue
			break;
		}
		case 1:
		{
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, blue1[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, green1[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 7; shift_byte++)
			{
				SPI.transfer(SS_pin, red1[shift_byte], SPI_CONTINUE);
			}
			SPI.transfer(SS_pin, red1[(anodelevel * 8) + 7]); // no continue
			break;
		}
		case 2:
		{
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, blue2[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, green2[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 7; shift_byte++)
			{
				SPI.transfer(SS_pin, red2[shift_byte], SPI_CONTINUE);
			}
			SPI.transfer(SS_pin, red2[(anodelevel * 8) + 7]); // no continue
			break;
		}
		case 3:
		{
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, blue3[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 8; shift_byte++)
			{
				SPI.transfer(SS_pin, green3[shift_byte], SPI_CONTINUE);
			}
			for (int shift_byte = (anodelevel * 8); shift_byte < (anodelevel * 8) + 7; shift_byte++)
			{
				SPI.transfer(SS_pin, red3[shift_byte], SPI_CONTINUE);
			}
			SPI.transfer(SS_pin, red3[(anodelevel * 8) + 7]); // no continue
			break;
		}
	}

	// END OF SPI TRANSFERS /////////////////////////////////////////////////////////////////////////////////////////////////////

	// Counter housekeeping.

	BAM_Counter++;
	anodelevel++;

	if (anodelevel == 8)			// Go back to 0 if anode levels goes above top of cube
	{
		anodelevel = 0;
	}

	if (BAM_Counter == 120)			// 15 cycles of BAM complete, so reset
	{
		BAM_Counter = 0;
		BAM_Bit = 0;
	}
}
/*!***************************************************************************
*!
*! FILE NAME  : FrontPanel.cc
*!
*! DESCRIPTION: Handles the LED:s

Goal: Status and CD LED blink in any pattern, but must be independent of each
other. Possible to force a short flash on the Network LED.

Solution: Use threads, semaphores and timers.

8-bit register available at the address 0x80000000, write only. Bit 3 - Network,
bit 4 - Status, bit 5 - CD. Need a shadow variable to remember state information.
Bit set to 1 means LED off, 0 means LED on. Init is therefore 0b00111000, or
0x38.

Implement the classes FrontPanel, LED, NetworkLEDTimer, StatusLEDTimer and
CDLEDTimer. See frontpanel.hh for full descr. of the classes.
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"

#include "iostream.hh"
#include "frontpanel.hh"

//#define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace if(false) cout
#endif

/************************ LED DEFINTION SECTION *****************************/

//Constructor takes in byte theLedNumber and sets the private variable myLedBit to theLedNumber
//myLedBit is a bitmask containing a '1' in the bit position for this led in the led register.
LED::LED(byte theLedNumber) : myLedBit(theLedNumber) {
}

//Initialize static shadow of the content of the led register. Must be used to manipulate one led without reseting the others.
byte LED::writeOutRegisterShadow = 0x38;

//Turn the LED on.
void LED::on() {
  byte ledBit = 4 << myLedBit; //myLedBits are: 1 - Network, 2 - Status, 3 - CD. Convert to their corresponding bits in the register.
  writeOutRegisterShadow ^= ledBit; //LEDs are off, meaning the bit is set to 1. Bitwise XOR with ledBit to turn on only that LED.
  *(VOLATEILE byte*)0x80000000 = writeOutRegisterShadow; //Write into the correct address
  iAmOn = true;
}

//Turn the LED off.
void LED::off() {
  byte ledBit = 4 << myLedBit;
  writeOutRegisterShadow ^= ledBit; //LEDs are on, meaning the bit is set to 0. Bitwise XOR with ledBit to turn off only that LED.
  *(VOLATEILE byte*)0x80000000 = writeOutRegisterShadow;
  iAmOn = false;
}

void LED::toggle() {
  if (iAmOn) {
    off();
  } else {
    on();
  }
}
/****************** FrontPanel DEFINITION SECTION ***************************/

//----------------------------------------------------------------------------
//
type
Class::method()
{
}

//----------------------------------------------------------------------------
//
type
Class::method()
{
}

/****************** END OF FILE FrontPanel.cc ********************************/

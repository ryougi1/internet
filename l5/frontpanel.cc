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
#include "sp_alloc.h"


//#define D_FP
#ifdef D_FP
#define trace cout
#else
#define trace if(false) cout
#endif

/************************ LED DEFINTION SECTION *****************************/

//Constructor takes in byte theLedNumber and sets the private variable myLedBit to theLedNumber.
//myLedBit is a bitmask containing a '1' in the bit position for this led in the led register.
LED::LED(byte theLedNumber) : myLedBit(theLedNumber) {
}

//Initialize static shadow of the content of the led register. Must be used to manipulate one led without reseting the others.
byte LED::writeOutRegisterShadow = 0x38;

//Turns the LED on.
void LED::on() {
  byte ledBit = 4 << myLedBit; //myLedBits are: 1 - Network, 2 - Status, 3 - CD. Convert to their corresponding bits in the register.
  writeOutRegisterShadow ^= ledBit; //LEDs are off, meaning the bit is set to 1. Bitwise XOR with ledBit to turn on only that LED.
  *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow; //Write into the correct address
  iAmOn = true;
}

//Turns the LED off.
void LED::off() {
  byte ledBit = 4 << myLedBit;
  writeOutRegisterShadow ^= ledBit; //LEDs are on, meaning the bit is set to 0. Bitwise XOR with ledBit to turn off only that LED.
  *(VOLATILE byte*)0x80000000 = writeOutRegisterShadow;
  iAmOn = false;
}

void LED::toggle() {
  if (iAmOn) {
    off();
  } else {
    on();
  }
}

/****************** NetworkLEDTimer DEFINTION SECTION ***********************/

NetworkLEDTimer::NetworkLEDTimer(Duration blinkTime) : myBlinkTime(blinkTime) {
}

void NetworkLEDTimer::start() {
  this->timeOutAfter(myBlinkTime); //Inherited from class Timed, starts timer
}

void NetworkLEDTimer::timeOut() {
  // cout << "NetworkLEDTimer timed out." << endl; //Placeholder // notify FrontPanel that this timer has expired.
  FrontPanel::instance().notifyLedEvent(FrontPanel::networkLedId);
}

/******************** CDLEDTimer DEFINTION SECTION *************************/

CDLEDTimer::CDLEDTimer(Duration blinkPeriod) {
  this->timerInterval(blinkPeriod); //Call superclass to set blinkPeriod
  this->startPeriodicTimer(); //Call superclass to start periodic timer
}

void CDLEDTimer::timerNotify() {
    // cout << "CDLEDTimer timed out." << endl; //Placeholder // notify FrontPanel that this timer has expired.
    // cout << "Core " << ax_coreleft_total() << endl;
    FrontPanel::instance().notifyLedEvent(FrontPanel::cdLedId);
}

/****************** StatusLEDTimer DEFINTION SECTION ***********************/

StatusLEDTimer::StatusLEDTimer(Duration blinkPeriod) {
  this->timerInterval(blinkPeriod); //Call superclass to set blinkPeriod
  this->startPeriodicTimer();//Call superclass to start periodic timer
}

void StatusLEDTimer::timerNotify() {
  // cout << "StatusLEDTimer timed out." << endl; //Placeholder // notify FrontPanel that this timer has expired.
  // cout << "Core " << ax_coreleft_total() << endl;
  FrontPanel::instance().notifyLedEvent(FrontPanel::statusLedId);
}

/****************** FrontPanel DEFINITION SECTION ***************************/

// Constructor: initializes the semaphore the leds and the event flags.
FrontPanel::FrontPanel() :
  Job(),
  mySemaphore(Semaphore::createQueueSemaphore("FP", 0)),
  myNetworkLED(networkLedId),
  myCDLED(cdLedId),
  myStatusLED(statusLedId) {
  Job::schedule(this);
}

// Returns the instance of FrontPanel, used for accessing the FrontPanel
FrontPanel& FrontPanel::instance() {
  static FrontPanel myInstance;
  return myInstance;
}

// Turn Network led on and start network led timer
void FrontPanel::packetReceived() {
  // cout << "Received a packet" << endl;
  myNetworkLED.on();
  myNetworkLEDTimer->start();
}

// Called from the timers to notify that a timer has expired.
// Sets an event flag and signals the semaphore.
void FrontPanel::notifyLedEvent(uword theLedId) {
  switch(theLedId) {
    case networkLedId:
      netLedEvent = true;
      break;
    case cdLedId:
      cdLedEvent = true;
      break;
    case statusLedId:
      statusLedEvent = true;
      break;
  }
  mySemaphore->signal();
}

// Main thread loop of FrontPanel. Initializes the led timers and goes into
// a perptual loop where it awaits the semaphore. When it wakes it checks
// the event flags to see which leds to manipulate and manipulates them.
void FrontPanel::doit() {
  //Initiate all timers, only CD and Status timer are actually started.
  myNetworkLEDTimer = new NetworkLEDTimer(Clock::seconds*1);
  myCDLEDTimer = new CDLEDTimer(Clock::seconds*1);
  myStatusLEDTimer = new StatusLEDTimer(Clock::seconds*3);

  while(true) {
    mySemaphore->wait();
    if (netLedEvent) {
      myNetworkLED.off();
      netLedEvent = false;
    }
    if (cdLedEvent) {
      myCDLED.toggle();
      cdLedEvent = false;
    }
    if (statusLedEvent) {
      //cout << "Core " << ax_coreleft_total() << endl;
      myStatusLED.toggle();
      statusLedEvent = false;
    }
  }

}

/****************** END OF FILE FrontPanel.cc ********************************/

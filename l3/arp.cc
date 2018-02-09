/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}

#include "iostream.hh"
#include "ethernet.hh"
#include "arp.hh"

//#define D_LLC
#ifdef D_ARP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** ARP DEFINITION SECTION *************************/
//Constructor
ARPInPacket::ARPInPacket(byte*           theData,
                         udword          theLength,
						             InPacket*       theFrame):
InPacket(theData, theLength, theFrame) {
}

//----------------------------------------------------------------------------
//
void
ARPInPacket::decode() {
  //Use ARPHeader as declared in arp.hh, same typecasting declaration as in Ethernet.cc
  ARPHeader* arpHeader = (ARPHeader *) myData;
  IPAddress* myIp = new IPAddress(130.235.200.115); //Needed for the if statement
  if (arpHeader->targetIPAddress == myIp) { //Are you looking for me?

    //From lab2 LLC decode method
    /*
    uword hoffs = myFrame->headerOffset(); //Was myFrame->headerOffset ???
    byte* temp = new byte[myLength + hoffs]; //Temp points to beginning of byte vector
    byte* aReply = temp + hoffs; //aReply points to beginning of data in the byte vector
    memcpy(aReply, myData, myLength);
    */

    uword flippedOp = HILO(0x0002);
    arpHeader->op = flippedOp; //Set op to reply (HILO see descr.)

    IPAddress senderIp = arpHeader->senderIPAddress; //Save sender IP
    arpHeader->senderIPAddress = myIp; //Set sender ip to us, not effected by endian (nee)
    arpHeader->targetIPAddress = senderIp; //Set target ip to original sender (nee)

    EthernetAddress senderMAC = arpHeader->senderEthAddress;
    arpHeader->senderEthAddress = Ethernet::instance().myAddress(); //Set sender eth address to us (nee)
    arpHeader->targetEthAddress = senderMAC; //Set target eth address to original sender (nee)

    uword headerOffset = headerOffset();
    memcpy(myData, arpHeader, headerOffset);
    this->answer(myData, myLength);
  }
  delete myIp;
}

//----------------------------------------------------------------------------
//
void
ARPInPacket::answer(byte *theData, udword theLength) {
  cout << "arp answer" << endl;
  myFrame->answer(theData, theLength);
}

uword
ARPInPacket::headerOffset() {
  return myFrame->headerOffset() + 28; //ARP header length is 28 bytes
}

/*!***************************************************************************
*!
*! FILE NAME  : llc.cc
*!
*! DESCRIPTION: LLC dummy
*!
*!***************************************************************************/

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
#include "llc.hh"
#include "arp.hh"
#include "ip.hh"

//#define D_LLC
#ifdef D_LLC
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** LLC DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
LLCInPacket::LLCInPacket(byte*           theData,
                         udword          theLength,
						             InPacket*       theFrame,
                         EthernetAddress theDestinationAddress,
                         EthernetAddress theSourceAddress,
                         uword           theTypeLen):
InPacket(theData, theLength, theFrame),
myDestinationAddress(theDestinationAddress),
mySourceAddress(theSourceAddress),
myTypeLen(theTypeLen)
{
}

//----------------------------------------------------------------------------
//
void
LLCInPacket::decode() {
  //It may be assumed that ethernet encapsulation (RFC894) only should be detected.
  //The type field in the ethernet frame is used in order to distinguish between
  //ARP packets and IP datagrams [Stevens96] p.23.
  //Detect ARP packets and IP datagrams and send them to the appropriate layers for further processing.

  if (myTypeLen == 0x0806) { //TL for ARP-req, broadcasted so must check all
    ARPInPacket* arp = new ARPInPacket(myData, myLength, this);
    //ARPInPacket arp(myData, myLength, this);
    //arp.decode();
    arp->decode();
    delete arp;
  }

  if (myDestinationAddress == Ethernet::instance().myAddress()) { //Only decode IP packets meant for us
    if (myTypeLen == 0x0800) {
      IPInPacket* ip = new IPInPacket(myData, myLength, this);
      //IPInPacket ip(myData, myLength, this);
      //ip.decode();
      ip->decode();
      delete ip;
    }
  }
}


//----------------------------------------------------------------------------
//
void
LLCInPacket::answer(byte *theData, udword theLength)
{
  //cout << "LLC answer method is calling myFrame->answer" << endl;
  myFrame->answer(theData, theLength);
}

uword
LLCInPacket::headerOffset()
{
  return myFrame->headerOffset() + 0;
}

InPacket* LLCInPacket::copyAnswerChain() {
  //cout << "LLCInPacket called copyAnswerChain" << endl;
  LLCInPacket* anAnswerPacket = new LLCInPacket(*this); //Create reference IPInPacket
  anAnswerPacket->setNewFrame(myFrame->copyAnswerChain()); //Set the frame to LLC
  return anAnswerPacket;
}

/****************** END OF FILE Ethernet.cc *************************************/

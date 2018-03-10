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
#include "icmp.hh"

//#define D_LLC
#ifdef D_ARP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** ARP DEFINITION SECTION *************************/
//Constructor
ICMPInPacket::ICMPInPacket(byte*           theData,
                         udword          theLength,
						             InPacket*       theFrame):
InPacket(theData, theLength, theFrame) {
}

//----------------------------------------------------------------------------
//
void
ICMPInPacket::decode() {
  /**
  Detect ICMP echo requests and answer them [Stevens96] p.86.The assembly of
  a reply packet is accomplished by changing the fields type and checksum.
  */
  ICMPHeader* icmpHeader = (ICMPHeader *) myData;

  if (icmpHeader->type == 0x08) {
    icmpHeader->type = 0;
    // Adjust ICMP checksum... (taken from llc class lab2)
    /**
    uword oldSum = icmpHeader->checksum;
	  uword newSum = oldSum + 0x8;
    icmpHeader->checksum = newSum;
    */
    uword oldSum = *(uword*)(myData + 2);
    uword newSum = oldSum + 0x8;
    icmpHeader->checksum = 0;
    icmpHeader->checksum = newSum;

    memcpy(myData, icmpHeader, ICMPInPacket::icmpHeaderLen);
    answer(myData, myLength);
  }


}

//----------------------------------------------------------------------------
//
void
ICMPInPacket::answer(byte *theData, udword theLength) {
  myFrame->answer(theData, theLength);
}

uword
ICMPInPacket::headerOffset() {
  return myFrame->headerOffset() + 4;
}

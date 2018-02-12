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
#include "icmp_hh"

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
  ICMPHeader* arpHeader = (ICMPHeader *) myData;
  /**
  if (Check type) {
    Change type to Echo Reply
    Set checksum
  }
  */

}

//----------------------------------------------------------------------------
//
void
ICMPInPacket::answer(byte *theData, udword theLength) {
  myFrame->answer(theData, theLength);
}

uword
ICMPInPacket::headerOffset() {
  return myFrame->headerOffset() + 28;
}

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
LLCInPacket::LLCInPacket(byte*           theData,
                         udword          theLength,
						             InPacket*       theFrame):
InPacket(theData, theLength, theFrame)
{
}

//----------------------------------------------------------------------------
//
void
ARPInPacket::decode() {
  //Use ARPHeader as declared in arp.hh, same typecasting declaration as in Ethernet.cc
  ARPHeader* arpHeader = (ARPHeader *) myData;
  IPAddress* myIp = new IPAddress(130.235.200.115); //Needed for the if statement
  if (arpHeader->targetIPAddress == myIp) { //Are you looking for me?

    ARPHeader* replyHeader = (ARPHeader *) reply;
    //Set sender ip to us
    //Set target ip to original sender
    //Set sender eth address to us
    //Set target eth address to original sender
    //Set op to reply (HILO see descr.)

    /**
    FROM RFC: A reply has the same length as a request, and several
    of the fields are the same. Does it matter what's in the data of the reply???
    */
    //TODO Create the reply
  }
  delete myIp;
}

//----------------------------------------------------------------------------
//
void
ARPInPacket::answer(byte *theData, udword theLength)
{
  myFrame->answer(theData, theLength);
}

uword
ARPInPacket::headerOffset()
{
  return myFrame->headerOffset() + 28; //ARP header length is 28 bytes
}

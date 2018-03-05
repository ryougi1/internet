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
#include "ip.hh"
#include "icmp.hh"
#include "ipaddr.hh"

//#define D_LLC
#ifdef D_ARP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** IP DEFINITION SECTION *************************/
uword seqNum = 0; //2 bytes identification

IP::IP() {
  myIPAddress = new IPAddress(130,235,200,115);
}

IP& IP::instance() {
  static IP myInstance;
  return myInstance;
}

const IPAddress& IP::myAddress() {
  return *myIPAddress;
}

IPInPacket::IPInPacket(byte*           theData,
                         udword          theLength,
						             InPacket*       theFrame):
InPacket(theData, theLength, theFrame) {
}


void IPInPacket::decode() {
  IPHeader* ipHeader = (IPHeader *) myData;
  IPAddress myIp(130,235,200,115); //Needed for the if statement
  /**
  The only packets to be processed are those addressed directly to the server,
  all IP broadcasts may be ignored. Check if the field destination IP address
  contains the address of your server. The packet may be thrown away if the
  IP address is different from that of your server. This field is the first
  to check if an effective algorithm is desired.

  Check the version field and make sure it contains the value 4. Check the
  header length field. Process the packet if the field contains the value 5.
  Either throw the packet away or calculate the start of the data content if
  the value is larger than 5. (In this case version and hlen together as
  one byte. Note: unaffected by endian since bytesize = 1)

  Make sure the packet is not fragmented by a check that the field
  fragmentFlagsNOffset (defined in the class IPHeader) & 0x3FFF is zero.
  */
  if ((ipHeader->destinationIPAddress == myIp) &&
        (ipHeader->versionNHeaderLength == 0x45) &&
        (HILO(ipHeader->fragmentFlagsNOffset) & 0x3FFF) == 0) {
        /**
        Use the field total length in order to calculate the length of the packet to
        be sent on to the upper layers of the stack. This is important as the packet
        may contain frame padding from the ethernet layer.

        Detect ICMP echo requests and send them to the approriate layer. Local copies
        of fields are only needed for the protocol field which describes what type of
        IP datagram it is, TCP, ICMP, UDP, ..., and the source IP address,
        which defines the host sending the request. Both fields are used in
        generating an ICMP echo reply.

        Extract and save the content of the field protocol. It is needed for replies.
        Extract and save the content of the field source IP address. It is needed for replies.
        */
        uword realTotalLength = HILO(ipHeader->totalLength); //totalLength is 2 bytes
        myProtocol = ipHeader->protocol;
        mySourceIPAddress = ipHeader->sourceIPAddress;
        if (ipHeader->protocol == 1) { //Assigned internet protocol number for ICMP (RFC790)
          ICMPInPacket* icmp = new ICMPInPacket(myData + IP::ipHeaderLength, realTotalLength - IP::ipHeaderLength, this);
          //ICMPInPacket icmp(myData + IP::ipHeaderLength, realTotalLength - IP::ipHeaderLength, this);
          icmp->decode();
          delete icmp;
        }
        if (ipHeader->protocol == 6) { //Assigned internet protocol number for TCP (RFC790)
          TCPInPacket* tcp = new TCPInPacket(myData + IP::ipHeaderLength, realTotalLength - IP::ipHeaderLength, this, mySourceIPAddress);
          //TCPInPacket tcp(myData + IP::ipHeaderLength, realTotalLength - IP::ipHeaderLength, this, &mySourceIPAddress);
          tcp->decode();
          delete tcp;
        }
  }
}

//----------------------------------------------------------------------------
//
void IPInPacket::answer(byte *theData, udword theLength) {
  cout << "IPInPacket::answer was called" << endl;
  IPHeader* replyHeader = new IPHeader();
  /**
  Set the version field to 4.
  Set the header length field to 5.
  */
  replyHeader->versionNHeaderLength = 0x45;
  replyHeader->TypeOfService = 0; //Set the type of service field to 0.
  replyHeader->totalLength = HILO(theLength + IP::ipHeaderLength); //Calculate and set the total length field.
  /**
  Set the identification field to a unique sequential number,
  e.g. the value of a global variable which is incremented each time an IP packet is sent.
  */
  replyHeader->identification = seqNum; //HILO???
  seqNum = (seqNum + 1) % 65536; //Starts at 0 all the way up to 2^16, then back to 0
  replyHeader->fragmentFlagsNOffset = 0; //Set the field fragmentFlagsNOffset to 0.
  replyHeader->timeToLive = 0x40; //Set the time to live field to 64, which is the default in TCP/IP.
  replyHeader->protocol = myProtocol; //Set the field protocol to the value saved in the decoding process.
  replyHeader->headerChecksum = 0; //Set the checksum field to 0 in to prepare for the checksum calculation.
  IPAddress myIp(130,235,200,115);
  replyHeader->sourceIPAddress = myIp; //Set the field source IP address to the IP address of your server.
  replyHeader->destinationIPAddress = mySourceIPAddress; //Set the field destination IP address to the source IP address saved in the decoding process.
  /**
  Calculate the checksum of the IP header with the provided function
  calculateChecksum and store the value in the field checksum.
  The function calculateChecksum returns a result which is big endian,
  according to the network byte order, and should not be altered.
  Checksum is calculated on the header only.
  */
  replyHeader->headerChecksum = calculateChecksum((byte*)replyHeader, IP::ipHeaderLength, 0);

  theData -= IP::ipHeaderLength;
  memcpy(theData, replyHeader, IP::ipHeaderLength);
  theLength += IP::ipHeaderLength;
  myFrame->answer(theData, theLength);
  delete replyHeader;
}

uword IPInPacket::headerOffset() {
  return myFrame->headerOffset() + IP::ipHeaderLength;
}

InPacket* IPInPacket::copyAnswerChain() {
  IPInPacket* anAnswerPacket = new IPInPacket(*this); //Create reference IPInPacket
  anAnswerPacket->setNewFrame(myFrame->copyAnswerChain()); //Set the frame to LLC
  return anAnswerPacket;
}

/*!***************************************************************************
*!
*! FILE NAME  : tcp.cc
*!
*! DESCRIPTION: TCP, Transport control protocol
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
#include "timr.h"
}

#include "iostream.hh"
#include "tcp.hh"
#include "ip.hh"

#define D_TCP
#ifdef D_TCP
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** TCP DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//
TCP::TCP()
{
  trace << "TCP created." << endl;
}

//----------------------------------------------------------------------------
//
TCP& TCP::instance() {
  static TCP myInstance;
  return myInstance;
}

TCPConnection* TCP::getConnection(IPAddress& theSourceAddress,
                   uword      theSourcePort,
                   uword      theDestinationPort) {

  TCPConnection* aConnection = NULL;
  // Find among open connections
  uword queueLength = myConnectionList.Length();
  myConnectionList.ResetIterator();
  bool connectionFound = false;
  while ((queueLength-- > 0) && !connectionFound)
  {
    aConnection = myConnectionList.Next();
    connectionFound = aConnection->tryConnection(theSourceAddress,
                                                 theSourcePort,
                                                 theDestinationPort);
  }
  if (!connectionFound)
  {
    trace << "Connection not found!" << endl;
    aConnection = NULL;
  }
  else
  {
    trace << "Found connection in queue" << endl;
  }
  return aConnection;
}

TCPConnection* TCP::createConnection(IPAddress& theSourceAddress,
                      uword      theSourcePort,
                      uword      theDestinationPort,
                      InPacket*  theCreator) {

  TCPConnection* aConnection =  new TCPConnection(theSourceAddress,
                                                  theSourcePort,
                                                  theDestinationPort,
                                                  theCreator);
  myConnectionList.Append(aConnection);
  return aConnection;
}

void TCP::deleteConnection(TCPConnection* theConnection) {
  myConnectionList.Remove(theConnection);
  delete theConnection;
}

//----------------------------------------------------------------------------
//

TCPConnection::TCPConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort,
                             InPacket*  theCreator):
        hisAddress(theSourceAddress),
        hisPort(theSourcePort),
        myPort(theDestinationPort)
{
  trace << "TCP connection created" << endl;
  myTCPSender = new TCPSender(this, theCreator),
  myState = ListenState::instance();
}

TCPConnection::~TCPConnection()
{
  trace << "TCP connection destroyed" << endl;
  delete myTCPSender;
}

//----------------------------------------------------------------------------
//
bool TCPConnection::tryConnection(IPAddress& theSourceAddress,
                             uword      theSourcePort,
                             uword      theDestinationPort)
{
  return (theSourcePort      == hisPort   ) &&
         (theDestinationPort == myPort    ) &&
         (theSourceAddress   == hisAddress);
}

// TCPConnection cont...
//Handle an incoming SYN segment
void TCPConnection::Synchronize(udword theSynchronizationNumber) {
  myState->Synchronize(this, theSynchronizationNumber);
}

//Handle an incoming FIN segment
void TCPConnection::NetClose() {
  //TODO:
}

//Handle close from application
void TCPConnection::AppClose() {
  //TODO:
}

//Handle an incoming RST segment, can also be called in other error
void TCPConnection::Kill() {
  //TODO:
}

//Handle incoming data
void TCPConnection::Receive(udword theSynchronizationNumber,
                            byte* theData,
                            udword theLength) {
  //TODO:
}

//Handle incoming acknowledgement
void TCPConnection::Acknowledge(udword theAcknowledgementNumber) {
  //TODO:
}

//Send outgoing data
void TCPConnection::Send(byte* theData, udword theLength) {
  myTCPSender->sendData(theData, theLength);
}
//----------------------------------------------------------------------------
// TCPState contains dummies for all the operations, only the interesting ones
// gets overloaded by the various sub classes.

//TODO Add dummies

void TCPState::Kill(TCPConnection* theConnection) {
  trace << "TCPState::Kill" << endl;
  TCP::instance().deleteConnection(theConnection);
}

//----------------------------------------------------------------------------
//

ListenState* ListenState::instance() {
  static ListenState myInstance;
  return &myInstance;
}

// Handle an incoming SYN segment
void ListenState::Synchronize(TCPConnection* theConnection,
                              udword theSynchronizationNumber) {
  //TODO:
  switch (theConnection->myPort) {
   case 7:
     trace << "got SYN on ECHO port" << endl;
     /**
     The next expected sequence number from the other host is
     denoted as receiveNext
     */
     theConnection->receiveNext = theSynchronizationNumber + 1;
     theConnection->receiveWindow = 8*1024;
     theConnection->sendNext = get_time();
     // Next reply to be sent.
     /**The variable sentUnAcked contains the latest sequence number an
     acknowledgement has been received for.
     */
     theConnection->sentUnAcked = theConnection->sendNext;
     // Send a segment with the SYN and ACK flags set.
     theConnection->myTCPSender->sendFlags(0x12);
     // Prepare for the next send operation.
     theConnection->sendNext += 1;
     // Change state
     theConnection->myState = SynRecvdState::instance();
     break;
   default:
     trace << "send RST..." << endl;
     theConnection->sendNext = 0;
     // Send a segment with the RST flag set.
     theConnection->myTCPSender->sendFlags(0x04);
     TCP::instance().deleteConnection(theConnection);
     break;
  }
}

//----------------------------------------------------------------------------
//
SynRecvdState* SynRecvdState::instance() {
  static SynRecvdState myInstance;
  return &myInstance;
}

/**
After LISTEN state, where we receive a (SYN) and sent a (SYN, ACK).
Expect to get (ACK) on our (SYN, ACK) .
*/
void SynRecvdState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
  //TODO:
  //Check ACK nr receiveNext
  //Update sentUnAcked
  //Change connection state to ESTABLISHED instance
}

//----------------------------------------------------------------------------
//
EstablishedState* EstablishedState::instance() {
  static EstablishedState myInstance;
  return &myInstance;
}

//Handle an incoming FIN segment
void EstablishedState::NetClose(TCPConnection* theConnection) {
  trace << "EstablishedState::NetClose" << endl;

  // Update connection variables and send an ACK

  // Go to NetClose wait state, inform application
  theConnection->myState = CloseWaitState::instance();

  // Normally the application would be notified next and nothing
  // happen until the application calls appClose on the connection.
  // Since we don't have an application we simply call appClose here instead.

  // Simulate application Close...
  theConnection->AppClose();
}

//Handle incoming data
void EstablishedState::Receive(TCPConnection* theConnection,
                          udword theSynchronizationNumber,
                          byte*  theData,
                          udword theLength)
{
  trace << "EstablishedState::Receive" << endl;

  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment, then echo the data using Send.
  //TODO:
  //Check that seq nr matches receiveNext
  //update receivenext
  //Call Send to echo.
}

//Handle incoming Acknowledgement
void EstablishedState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
  //TODO:
  /**The variable sentUnAcked contains the latest sequence number an
  acknowledgement has been received for.
  */
  //update sentUnAcked
}

void EstablishedState::Send(TCPConnection* theConnection,
          byte*  theData,
          udword theLength)
{
  //TODO:
  //Call connection send.
  //Update sequence number sendNext
}


//----------------------------------------------------------------------------
//
CloseWaitState* CloseWaitState::instance() {
  static CloseWaitState myInstance;
  return &myInstance;
}

//Handle close from application
void CloseWaitState::AppClose(TCPConnection* theConnection) {
  //TODO:
}

//----------------------------------------------------------------------------
//
LastAckState* LastAckState::instance() {
  static LastAckState myInstance;
  return &myInstance;
}

//Handle incoming Acknowledgement
void LastAckState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
  //TODO:
}


//----------------------------------------------------------------------------
//
TCPSender::TCPSender(TCPConnection* theConnection,
                     InPacket*      theCreator):
        myConnection(theConnection),
        myAnswerChain(theCreator->copyAnswerChain()) // Copies InPacket chain!
{
}

TCPSender::~TCPSender() {
  myAnswerChain->deleteAnswerChain();
}
/**
Checksum is calculated over the pseudoheader + tcpheader + tcpdata and then
stored in the tcpheader. The pseudoheader is placed in front of the tcp segment.
Once the checksum is calculated, the pseudoheader is discarded. Why have it?
"This gives the TCP protection against misrouted segments".
*/

//Send a flag segment without data
void TCPSender::sendFlags(byte theFlags) {
  // Decide on the value of the length totalSegmentLength.
  uword totalSegmentLength = TCP::tcpHeaderLength; //No data
  // Allocate a TCP segment.
  byte* anAnswer = new byte[totalSegmentLength];
  // Calculate the pseudo header checksum
  TCPPseudoHeader* aPseudoHeader =
    new TCPPseudoHeader(myConnection->hisAddress,
                        totalSegmentLength);
  uword pseudosum = aPseudoHeader->checksum();
  delete aPseudoHeader;

  // TODO: Create the TCP segment.
  //TCPHeader replyHeader = (TCPHeader *) anAnswer;
  //Set source port
  //Set dest port
  //Set seq nr
  //Set ack nr
  //Set header length
  //Set flags
  //Set window size
  //Set checksum = 0 to prep
  //Set urgent pointer

  // Calculate the final checksum.
  aTCPHeader->checksum = calculateChecksum(anAnswer,
                                           totalSegmentLength,
                                           pseudosum);
  // Send the TCP segment.
  myAnswerChain->answer(anAnswer,
                        totalSegmentLength);
  // Deallocate the dynamic memory
  delete anAnswer;
}

//Send a data segment. PSH and ACK flags are set
void TCPSender::sendData(byte* theData, udword theLength) {
  //TODO:
  // Total length
  // byte vector as above
  // Calculate the pseudo header checksum

  //Create header and set header fields as above

  // Calculate the final checksum

  //Send the TCP segment

  //Deallocate the dynamic memory

}


//----------------------------------------------------------------------------
//
TCPInPacket::TCPInPacket(byte*           theData,
                         udword          theLength,
                         InPacket*       theFrame,
                         IPAddress&      theSourceAddress):
        InPacket(theData, theLength, theFrame),
        mySourceAddress(theSourceAddress)
{
}

void TCPInPacket::decode() {
  // Extract the parameters from the TCP header which define the
  // connection.
  TCPHeader* tcpHeader = (TCPHeader *) myData;
  mySourcePort = HILO(tcpHeader->sourcePort);
  myDestinationPort = HILO(tcpHeader->destinationPort);
  mySequenceNumber = LHILO(tcpHeader->sequenceNumber);
  myAcknowledgementNumber = LHILO(tcpHeader->acknowledgementNumber);

  TCPConnection* aConnection =
           TCP::instance().getConnection(mySourceAddress,
                                         mySourcePort,
                                         myDestinationPort);
    if (!aConnection) {
      // Establish a new connection.
      aConnection =
           TCP::instance().createConnection(mySourceAddress,
                                            mySourcePort,
                                            myDestinationPort,
                                            this);
      if ((tcpHeader->flags & 0x02) != 0) {
        // State LISTEN. Received a SYN flag.
        aConnection->Synchronize(mySequenceNumber);
      } else {
        // State LISTEN. No SYN flag. Impossible to continue.
        aConnection->Kill();
      }
    } else {
      // TODO: Connection was established. Handle all states.

      //State ESTABLISHED. Received RST flag
      if ((tcpHeader->flags & 0x04) == 0x04) {
        aConnection->Kill();
      }

      //State ESTABLISHED. Received FIN flag.
      if ((tcpHeader->flags & 0x01) == 0x01) {
        aConnection->NetClose();
      }

      //State ESTABLISHED. Received ACK flag.
      if ((tcpHeader->flags & 0x10) == 0x10) {
        aConnection->Acknowledge(myAcknowledgementNumber);
      }

      //State ESTABLISHED. Received PSH and ACK flag.
      //The PSH and ACK flags must always be set in a TCP segment to be transmitted containing data.
      //NOTE: Enters here after proccing regular ACK.
      if ((tcpHeader->flags & 0x18) == 0x18) {
        aConnection->Receive(mySequenceNumber, myData + TCP::tcpHeaderLength, myLength - TCP::tcpHeaderLength);
      }

    }
}

void TCPInPacket::answer(byte* theData, udword theLength) {
  myFrame->answer(theData, theLength);
}

uword TCPInPacket::headerOffset() {
  return myFrame->headerOffset() + TCP::tcpHeaderLength;
}

//----------------------------------------------------------------------------
//
InPacket*
TCPInPacket::copyAnswerChain() {
  return myFrame->copyAnswerChain();
}

//----------------------------------------------------------------------------
//
TCPPseudoHeader::TCPPseudoHeader(IPAddress& theDestination,
                                 uword theLength):
        sourceIPAddress(IP::instance().myAddress()),
        destinationIPAddress(theDestination),
        zero(0),
        protocol(6)
{
  tcpLength = HILO(theLength);
}

//----------------------------------------------------------------------------
//
uword
TCPPseudoHeader::checksum()
{
  return calculateChecksum((byte*)this, 12);
}


/****************** END OF FILE tcp.cc *************************************/

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
  //trace << "TCP created." << endl;
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
    //trace << "Connection not found!" << endl;
    aConnection = NULL;
  }
  else
  {
    //trace << "Found connection in queue" << endl;
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

/**
When TCPConnection receives a SYN in ListenState, this method is invoked.
Is true when a connection is accepted on port portNo. At present, only the
ECHO port, 7, should be supported.
*/
void TCP::acceptConnection(uword portNo) {
  return portNo == 7;
}

/**
When TCPConnection receives ACK in SynRecvdState, this method is invoked.
*/
void TCP::connectionEstablished(TCPConnection* theConnection) {
  if (theConnection->serverPortNumber() == 7) {
    TCPSocket* aSocket = new TCPSocket(theConnection); // Create a new TCPSocket.
    theConnection->registerSocket(aSocket); // Register the socket in the TCPConnection.
    Job::schedule(new SimpleApplication(aSocket)); // Create and start an application for the connection.
  }
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
  //trace << "TCP connection created" << endl;
  currentSeq = 0;
  myTCPSender = new TCPSender(this, theCreator),
  myState = ListenState::instance();
}

TCPConnection::~TCPConnection()
{
  //trace << "TCP connection destroyed" << endl;
  delete myTCPSender;
}

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
  myState->NetClose(this);
}

//Handle close from application
void TCPConnection::AppClose() {
  myState->AppClose(this);
}

//Handle an incoming RST segment, can also be called in other error
void TCPConnection::Kill() {
  myState->Kill(this);
}

//Handle incoming data
void TCPConnection::Receive(udword theSynchronizationNumber,
                            byte* theData,
                            udword theLength) {
  myState->Receive(this, theSynchronizationNumber, theData, theLength);
}

//Handle incoming acknowledgement
void TCPConnection::Acknowledge(udword theAcknowledgementNumber) {
  myState->Acknowledge(this, theAcknowledgementNumber);
}

//Send outgoing data
void TCPConnection::Send(byte* theData, udword theLength) {
  myState->Send(this, theData, theLength);
}


//ADDITIONS FOR LAB5, add in .hh-file as well?
// Return myPort.
uword TCPConnection::ServerPortNumber(){
  return myPort;
}

//set mySocket to theSocket
void TCPConnection::registerSocket(TCPSocket* theSocket){
  mySocket = theSocket;
}


//----------------------------------------------------------------------------
// TCPState contains dummies for all the operations, only the interesting ones
// gets overloaded by the various sub classes.
void TCPState::Synchronize(TCPConnection* theConnection, udword theSynchronizationNumber) {}
void TCPState::NetClose(TCPConnection* theConnection) {}
void TCPState::AppClose(TCPConnection* theConnection) {}
void TCPState::Kill(TCPConnection* theConnection) {
  //trace << "TCPState::Kill" << endl;
  TCP::instance().deleteConnection(theConnection);
}
void TCPState::Receive(TCPConnection* theConnection,
                      udword theSynchronizationNumber,
                      byte* theData,
                      udword theLength)
{}
void TCPState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {}
void TCPState::Send(TCPConnection* theConnection,
                    byte* theData,
                    udword theLength)
{}

//----------------------------------------------------------------------------
//

ListenState* ListenState::instance() {
  static ListenState myInstance;
  return &myInstance;
}

void ListenState::Synchronize(TCPConnection* theConnection,
                              udword theSynchronizationNumber) {
if (TCP::instance().acceptConnection(theConnection->myPort)) {
  switch (theConnection->myPort) {
   case 7:
     // The next expected sequence number from the other host is receiveNext
     theConnection->receiveNext = theSynchronizationNumber + 1; //Increment next expected seq nr
     theConnection->receiveWindow = 8*1024;
     theConnection->sendNext = get_time(); //Next seq nr to be sent

     /**The variable sentUnAcked contains the latest sequence number an
     acknowledgement has been received for.
     */
     theConnection->sentUnAcked = theConnection->sendNext; //Initiate sentUnAcked as initial seq nr - 1 (so it is technically un-acked)
     theConnection->myTCPSender->sendFlags(0x12); // Send a segment with the SYN and ACK flags set.
     theConnection->sendNext += 1; // Prepare for the next send operation.
     theConnection->myState = SynRecvdState::instance();  // Change state
     break;
   default:
     //trace << "send RST..." << endl;
     theConnection->sendNext = 0;
     theConnection->myTCPSender->sendFlags(0x04); // Send a segment with the RST flag set.
     TCP::instance().deleteConnection(theConnection);
     break;
  }
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
    //trace << "SynRecvdState::Acknowledge" << endl;
    if(theAcknowledgementNumber == theConnection->sendNext) {
      // Since sendNext was incremented in ListenState,the ACK should be for our next segment
      theConnection->sentUnAcked = theAcknowledgementNumber; //Update sentUnAcked to last acked segment
      theConnection->myState = EstablishedState::instance(); //Change state to Established
      TCP::instance().connectionEstablished(theConnection); //Call new lab5 method for socket & app functionality
    } else {
      trace << "SynRecvdState::Acknowledge received wrong ACK nr" << endl;
      theConnection->Kill();
    }

}

//----------------------------------------------------------------------------
//
EstablishedState* EstablishedState::instance() {
  static EstablishedState myInstance;
  return &myInstance;
}

//Handle an incoming FIN segment
void EstablishedState::NetClose(TCPConnection* theConnection) {
  /** Update connection variables and send an ACK
  We want to increment ack nr, but keep our seq nr the same.
  Since ack nr remains the same as in the prior packet just +1
  */
  theConnection->receiveNext += 1;
  theConnection->myTCPSender->sendFlags(0x10); //An ACK, to complete half close.
  theConnection->mySocket->socketEof();
  /**
  Passive close from lab4 no longer used. Instead implemented active close. NetClose calls socketEof,
  which will lead to TCPSocket::Close() which invokes EstablishedState::AppClose(). See diagram.
  */
}

void EstablishedState::AppClose(TCPConnection* theConnection) {
  theConnection->myState = FinWait1State::instance();
  theConnection->myTCPSender->sendFlags(0x11); //Send FIN
  theConnection->sendNext = theConnection->sendNext + 1; //https://community.apigee.com/articles/7970/tcp-states-explained.html
}

//Handle incoming data
void EstablishedState::Receive(TCPConnection* theConnection,
                          udword theSynchronizationNumber,
                          byte*  theData,
                          udword theLength)
{
  // Delayed ACK is not implemented, simply acknowledge the data
  // by sending an ACK segment, then echo the data using Send.
  //trace << "EstablishedState::Receive" << endl;
  if (theSynchronizationNumber == theConnection->receiveNext) {
    theConnection->receiveNext += theLength; //Update next expected seq nr
    theConnection->mySocket->socketDataReceived(theData, theLength); //Tell socket to release read semaphore
    Send(theConnection, theData, theLength); //Call EstablishedState::Send
  } else {
    trace << "EstablishedState::Receive unexpected sequence number" << endl;
  }
}

//Handle incoming Acknowledgement
void EstablishedState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
  if (theAcknowledgementNumber > theConnection->sentUnAcked) {
    theConnection->sentUnAcked = theAcknowledgementNumber; // Only update sentUnAcked if received ACK is greater
  }

  if (theAcknowledgementNumber == theConnection->queueLength + theConnection->firstSeq) {
    //All previously sent segments are now acked.
    //Time to release the TCPSocket::Write semaphore
    theConnection->mySocket->socketDataSent();
  } else {
    theConnection->myTCPSender->sendFromQueue();
  }

  // Handle sendNext in sendFromQueue
}

// TCPSocket calls TCPConnection::Send which calls myState->Send which is here
// Send in Lab5 implements a transmission queue
void EstablishedState::Send(TCPConnection* theConnection,
          byte*  theData,
          udword theLength)
{
  //Set variables for transmission queue
  theConnection->transmitQueue = theData;
  theConnection->queueLength = theLength;
  theConnection->firstSeq = theConnection->sendNext;
  theConnection->currentSeq = theConnection->sendNext;


  while (theConnection->theOffset() != theConnection->queueLength) { //There's more
    theConnection->myTCPSender->sendFromQueue();
  }

/*
  theConnection->Send(theData, theLength); //Echo the received data
  theConnection->sendNext += theLength; // Update our own sequence number
  */

}


//----------------------------------------------------------------------------
//
CloseWaitState* CloseWaitState::instance() {
  static CloseWaitState myInstance;
  return &myInstance;
}

//Handle close from application
void CloseWaitState::AppClose(TCPConnection* theConnection) {
  /**
  Close from application is manually called in in EstablishedState::NetClose()
  Here we simply want to send a FIN flag to complete our half of the half close.
  Then we change state to LastAckState.
  */
  //trace << "CloseWaitState::AppClose" << endl;
  theConnection->myTCPSender->sendFlags(0x11);
  theConnection->myState = LastAckState::instance();
}

//----------------------------------------------------------------------------
//
LastAckState* LastAckState::instance() {
  static LastAckState myInstance;
  return &myInstance;
}

//Handle incoming Acknowledgement
void LastAckState::Acknowledge(TCPConnection* theConnection, udword theAcknowledgementNumber) {
  /**
  Final stage of the channel tear-down. We acked their FIN in
  EstablishedState::NetClose(), then we sent our FIN in
  CloseWaitState::AppClose(), and here we received their final ack.
  */
  theConnection->sentUnAcked = theAcknowledgementNumber;
  //Make sure that their ACK is actually for our FIN and not something else
  if (theConnection->sendNext + 1 == theAcknowledgementNumber) {
    //trace << "LastAckState::Acknowledge" << endl;
    theConnection->Kill();
  }
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

  //Create the TCP segment.
  TCPHeader* replyHeader = (TCPHeader *) anAnswer;
  replyHeader->sourcePort = HILO(myConnection->myPort); //Set source port
  replyHeader->destinationPort = HILO(myConnection->hisPort); //Set dest port
  replyHeader->sequenceNumber = LHILO(myConnection->sendNext); //Set seq nr
  replyHeader->acknowledgementNumber = LHILO(myConnection->receiveNext); //Set ack nr
  replyHeader->headerLength = 0x50; //Set header length, since only four bits, left shift.
  replyHeader->flags = theFlags;//Set flags
  replyHeader->windowSize = HILO(myConnection->receiveWindow); //Set window size
  replyHeader->checksum = 0; //Set checksum = 0 to prep
  replyHeader->urgentPointer = 0; //Set urgent pointer

  // Calculate the final checksum.
  replyHeader->checksum = calculateChecksum(anAnswer,
                                           totalSegmentLength,
                                           pseudosum);
  // Send the TCP segment.
  myAnswerChain->answer(anAnswer, totalSegmentLength);
  // Deallocate the dynamic memory
  delete anAnswer;
}

//Send a data segment. PSH and ACK flags are set
void TCPSender::sendData(byte* theData, udword theLength) {
  uword totalSegmentLength = TCP::tcpHeaderLength + theLength;
  byte* anAnswer = new byte[totalSegmentLength];

  TCPPseudoHeader* aPseudoHeader =
    new TCPPseudoHeader(myConnection->hisAddress,
                        totalSegmentLength);
  uword pseudosum = aPseudoHeader->checksum();
  delete aPseudoHeader;

  TCPHeader* replyHeader = (TCPHeader *) anAnswer;
  replyHeader->sourcePort = HILO(myConnection->myPort); //Set source port
  replyHeader->destinationPort = HILO(myConnection->hisPort); //Set dest port
  replyHeader->sequenceNumber = LHILO(myConnection->sendNext); //Set seq nr
  replyHeader->acknowledgementNumber = LHILO(myConnection->receiveNext); //Set ack nr
  replyHeader->headerLength = 0x50 << 4; //Set header length, since only 4 bits, left shift
  replyHeader->flags = 0x18; //Set flags, should be PSH and ACK since sending data
  replyHeader->windowSize = HILO(myConnection->receiveWindow); //Set window size
  replyHeader->checksum = 0; //Set checksum = 0 to prep
  replyHeader->urgentPointer = 0; //Set urgent pointer

  memcpy(anAnswer + TCP::tcpHeaderLength, theData, theLength);

  replyHeader->checksum = calculateChecksum(anAnswer,
                                           totalSegmentLength,
                                           pseudosum);

  myAnswerChain->answer(anAnswer, totalSegmentLength);
  delete anAnswer;
}

void TCPSender::sendFromQueue() {
  udword theWindowSize = theConnection->myWindowSize -
    (theConnection->sendNext - theConnection->sentUnAcked); // From lab 5 descr.

  //Calculated window size is
  if (theWindowSize > myConnection->myWindowSize) {
      theWindowSize = 0;
  }

  udword min = MIN(theWindowSize, myConnection->queueLength);

  if (myConnection->sendNext < myConnection->currentSeq) { // sendNext less than currentSeq

  } else {

  }
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
        // State LISTEN. Received a SYN flag. Will send SYN, ACK and go to SynRecvdState.
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

      //State ESTABLISHED or SynRecvd. Received ACK flag.
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

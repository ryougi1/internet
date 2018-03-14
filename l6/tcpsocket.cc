

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
#include "tcp.hh"
#include "ip.hh"
#include "tcpsocket.hh"
#include "threads.hh"
#include "sp_alloc.h"


#ifdef D_TCPSOCKET
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** TCPSocket *************************/

// Constructor. The socket is created by class TCP when a connection is
// established. create the semaphores
TCPSocket::TCPSocket(TCPConnection* theConnection) :
  myConnection(theConnection),
  myReadSemaphore(Semaphore::createQueueSemaphore("readSemaphore", 0)),
  myWriteSemaphore(Semaphore::createQueueSemaphore("writeSemaphore", 0)),
  eofFound(false),
  RSTFlag(false)
{
}

// Destructor. destroy the semaphores.
TCPSocket::~TCPSocket(){
  delete myReadSemaphore;
  delete myWriteSemaphore;
}


/**
Semaphore blocks read until data is available. The method socketDataReceived is
invoked in EstablishedState::Receive, which releases semaphore.
*/
byte* TCPSocket::Read(udword& theLength) {
  //cout << "Socket waiting for data to read" << endl;
  myReadSemaphore->wait(); // Wait for available data
  theLength = myReadLength;
  byte* aData = myReadData;
  myReadLength = 0;
  myReadData = 0;
  return aData;
}

bool TCPSocket::isEof(){
  return eofFound;
}

// Semaphore blocks write until data is transmitted AND acked.
void TCPSocket::Write(byte* theData, udword theLength) {
  //cout << "Inside TCPSocket::Write" << endl;
  if (myConnection->RSTFlagReceived) {
    cout << "NOT WRITING BECAUSE OF RSTFLAG" << endl;
    return;
  }
  myConnection->Send(theData, theLength);
  myWriteSemaphore->wait(); // Wait until the data is acknowledged
}

void TCPSocket::Close(){
  myConnection->AppClose();
}

// Called by state ESTABLISHED receive
void TCPSocket::socketDataReceived(byte* theData, udword theLength) {
  //cout << "Releasing read semaphore" << endl;
  myReadData = new byte[theLength];
  memcpy(myReadData, theData, theLength);
  myReadLength = theLength;
  myReadSemaphore->signal(); // Data is available
}

// Called by state ESTABLISHED acknowledge
void TCPSocket::socketDataSent() {
  //cout << "Releasing write semaphore" << endl;
  myWriteSemaphore->signal(); // The data has been acknowledged
}

void TCPSocket::socketEof() {
  //cout << "socketEOf was called" << endl;
  eofFound = true;
  myReadSemaphore->signal();
}

/****************** SimpleApplication *************************/

// Constructor. The application is created by class TCP when a connection is
// established.
SimpleApplication::SimpleApplication(TCPSocket* theSocket) :
  mySocket(theSocket)
{
}

// Gets called when the application thread begins execution.
// The SimpleApplication job is scheduled by TCP when a connection is
// established.
void SimpleApplication::doit(){
  //cout << "SimpleApplication started" << endl;
  udword aLength;
  byte* aData;
  bool done = false;
  //cout << "Before WHILE: " << done << " : " << mySocket->isEof() << endl;
  while (!done && !mySocket->isEof()) {
    aData = mySocket->Read(aLength);
    if (aLength > 0) {
      if ((char)*aData == 'q') {
        //cout << "SimpleApplication:: found 'q'" << endl;
        done = true;
      } else if ((char)*aData == 'r') {           // Functionality 'r' for bulk data.
        sendBigData('r');
      } else if ((char)*aData == 's') {          // Functionality 's' for even more bulk data.
        sendBigData('s');
      } else {                                    // Regular
        mySocket->Write(aData, aLength);
      }
    }
    delete aData;
    //cout << "AFTER WHILE: " << done << " : " << mySocket->isEof() << endl;
  }
  //cout << "SimpleApplication::doIt is bailing out holmes" << endl;
  mySocket->Close();
}

void SimpleApplication::sendBigData(char code) {
  byte* theData = new byte[10000];
  for (int i = 0; i < 10000; i++) {
    theData[i] = 'a';
  }
  switch (code) {
    case 'r': mySocket->Write(theData, 10000); break;
    case 's':
      for (int j = 0; j < 10; j++) {
        mySocket->Write(theData, 10000);
      }
      break;
  }
  delete theData;
}



/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
extern "C"
{
#include "system.h"
}

#include "tcpsocket.hh"

#ifdef D_TCPSOCKET
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** TCPSocket *************************/

// Constructor. The socket is created by class TCP when a connection is
// established. create the semaphores
TCPSocket::TCPSocket() {
  mySocket = new TCPSocket(TCPConnection* theConnection);
}

// Destructor. destroy the semaphores.

TCPSocket::~TCPSocket(){
  delete mySocket;
}



byte* TCPSocket::Read(udword& theLength) {
  myReadSemaphore->wait(); // Wait for available data
  theLength = myReadLength;
  byte* aData = myReadData;
  myReadLength = 0;
  myReadData = 0;
  return aData;
}

bool TCPSocket::isEof(){
  return eofFound; //TODO: maybe not correct
}


void TCPSocket::Write(byte* theData, udword theLength) {
  myConnection->Send(theData, theLength);
  myWriteSemaphore->wait(); // Wait until the data is acknowledged 
}

void TCPSocket::Close(){
  //TODO
  //invokes the method EstablishedState::AppClose in
  //the state machine as a response to the command q or a FIN flag in a received segment.
}

void TCPSocket::socketDataReceived(byte* theData, udword theLength) {
  myReadData = new byte[theLength];
  memcpy(myReadData, theData, theLength);
  myReadLength = theLength;
  myReadSemaphore->signal(); // Data is available
}

void TCPSocket::socketDataSent() {
  myWriteSemaphore->signal(); // The data has been acknowledged
}

void TCPSocket::socketEof() {
  eofFound = true;
  myReadSemaphore->signal();
}




/****************** SimpleApplication *************************/

// Constructor. The application is created by class TCP when a connection is
// established.
SimpleApplication(TCPSocket* theSocket) {
  mySimpleApplication = new SimpleApplication(theSocket);
}



// Gets called when the application thread begins execution.
// The SimpleApplication job is scheduled by TCP when a connection is
// established.
void SimpleApplication::doit(){
  udword aLength;
    byte* aData;
    bool done = false;
    while (!done && !mySocket->isEof())
    {
      aData = mySocket->Read(aLength);
      if (aLength > 0)
      {
        mySocket->Write(aData, aLength);
        if ((char)*aData == 'q')
        {
          done = true;
        }
        delete aData;
      }
    }
    mySocket->Close();
}

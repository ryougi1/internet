/*!***************************************************************************
*!
*! FILE NAME  : http.cc
*!
*! DESCRIPTION: HTTP, Hyper text transfer protocol.
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
#include "tcpsocket.hh"
#include "http.hh"
#include "fs.hh"

//#define D_HTTP
#ifdef D_HTTP
#define trace cout
#else
#define trace if(false) cout
#endif

/****************** HTTPServer DEFINITION SECTION ***************************/

HTTPServer::HTTPServer(TCPSocket* theSocket) :
  mySocket(theSocket)
{
}

void HTTPServer::doit() {
  /**
  Every time a page is loaded, starts a new connection and HTTPServer job.
  Our job to close the connection once we have handled the clients request with
  the appropriate reponse.
  */
  udword aLength;
  char* aData;
  bool done = false;
  //cout << "Before WHILE: " << done << " : " << mySocket->isEof() << endl;
  while ((!done)) {
    aData = (char*) mySocket->Read(aLength);
    if (strncmp(aData, "GET ", 4) == 0) {
      //cout << "DETECTED GET REQUEST" << endl;
      handleGetRequest(aData, aLength);
    }
    if (strncmp(aData, "HEAD", 4) == 0) {
      //cout << "DETECTED HEAD REQUEST" << endl;
      handleHeadRequest(aData, aLength);
    }
    if (strncmp(aData, "POST", 4) == 0) {
      //cout << "DETECTED POST REQUEST" << endl;

      /**The header is sent over two segments*/
      udword addLength = 0;
      char* addData = (char*) mySocket->Read(addData);
      char* completeHeader = new char[aLength + addLength];
      memcpy(completeHeader, aData, aLength);
      memcpy(completeHeader + aLength, addData, addLength);

      handlePostRequest(completeHeader, aLength + addLength);
      delete addData;
      delete completeHeader;
    }
    done = true;
  }
  delete aData;
  mySocket->Close();
  //cout << "HTTPServer is quitting" << endl;
}

void HTTPServer::handlePostRequest(char* theData, udword theLength) {
  //POST request should only be possible for ''/private/private.htm'. If that is
  //the case, decode the file content with decodeForm, add a '\0', and write the
  //file to the file system. Lastly, send the appropriate response to the client.

  //cout << "Inside handlePostRequest" << endl;
  char* path = findPathName(theData);
  byte* responseData;
  char* initRespLine;
  char* headerContType;
  if (strncmp(path, (char*)"private", 7) == 0) { //Correct
    udword theContentLength = contentLength(theData, theLength);
    char* bodySoFar = moveToBody(aData);
    cout << "bodySoFar is: " << endl;
    cout << bodySoFar << endl;
    udword totalBodyLength = theLength - ((udword)bodySoFar - (udword)theData); //Amount of body received so far
    cout << "Total Body Length so far: " << totalBodyLength << endl;
    char* allData = new char[theContentLength + 1]; //Create space for all the body data
    memcpy(allData, bodySoFar, totalBodyLength); //Start by copying over the body that we have to allData
    delete bodySoFar;

    /** Now account for the fact that not all the body was sent in the
    received segments so far. Might need to read more segments */
    while (totalBodyLength < theContentLength) {
      char* moreData = (char*) mySocket.Read(theLength);
      memcpy(allData + totalBodyLength, moreData, theLength);
      totalBodyLength += theLength;
      delete moreData;
    }

    allData[theContentLength] = '\0';
    char* decodedBody = decodeForm(allData);
    cout << "decodedBody: " << endl;
    cout << decodedBody << endl;
    delete allData;

    if (FileSystem::instance().writeFile((byte*)decodedBody, strlen(decodedBody))) { //Write to dynamic.htm
      cout << "Writefile returned true" << endl;
      initRespLine = "HTTP/1.0 200 OK\r\n";
      headerContType = "Content-type: text/html\r\n\r\n";
      responseData = (byte*)  "<html><head><title>Accepted</title></head>\r\n"
                              "<body><h1>dynamic.htm successfully updated.</h1></body></html>";

      /**
      If it still doesn't work, try to send:
      mySocket->Write((byte*)decodedBody, strlen(decodedBody));
      return;
      */
    } else {
      cout << "Something went wrong when writing to dynamic.htm" << endl;
      //TODO: Should sent response with info that server ran into an error
    }
  } else { //Here if POST came with wrong path
    cout << "POST request came from wrong path" << endl;
    initRespLine = "HTTP/1.0 405 Method Not Allowed\r\n";
    headerContType = "Content-Type: text/html\r\n\r\n";
    responseData =  (byte*) "<html><head><title>What you playing at?</title></head>\r\n"
                            "<body><h1>405 Method Not Allowed</h1></body></html>";
  }

  cout << "Calling mysocket->Write" << endl;
  mySocket->Write((byte*)initRespLine, strlen(initRespLine));
  mySocket->Write((byte*)headerContType, strlen(headerContType));
  //mySocket->Write(responseData, strlen((char*)responseData));

  delete path;
  delete responseData;
  delete initRespLine;
  delete headerContType;
}

void HTTPServer::handleGetRequest(char* theData, udword theLength) {
  bool shouldSendData = true;

  char* path = findPathName(theData);
  udword fileLength;
  byte* responseData;
  char* initRespLine;
  char* headerContType;
  if (path == NULL || path == 0) { //Path was either "/" or "/index.htm"
    char* fileName = "index.htm"; //Manually set file name since we know what file to read and send

    responseData = FileSystem::instance().readFile(path, fileName, fileLength);
    initRespLine = "HTTP/1.0 200 OK\r\n";
    headerContType = "Content-Type: text/html\r\n\r\n";
    //cout << "Sending: index.htm" << endl;
    delete fileName;

  } else { //A absolute path was found
    //cout << "Path is: " << path << endl;
    //Want to find fileName to get file, and fileType for header
    char* first = strchr(theData, ' '); //Points to blank space right before absolute path
    first++; //Need to move of the blankspace onto the '/' for next line of code to work
    char* last = strchr(first, ' '); //Points to blank space right after absolute path
    char* pathAndFile = extractString((char*)first+1, last-first); // +1 to get off '/' and onto the path
    //cout << "pathAndFile is: " << pathAndFile << endl;
    char* fileName = strrchr(pathAndFile, '/'); //Points to '/' just before file name
    fileName++; //Move onto file name
    //cout << "File name is: " << fileName << endl;

    responseData = FileSystem::instance().readFile(path, fileName, fileLength);
    initRespLine = "HTTP/1.0 200 OK\r\n";
    headerContType = contentTypeFromFileName(fileName);
    //cout << "Header: " << headerContType << endl;

    //cout << "Sending: " << fileName << endl;

    delete first;
    delete last;
    delete pathAndFile;
    delete fileName;
  }

  //Checks go here
  if (responseData == 0) { //check if file was not found
    cout << "File not found, returning 404 error response" << endl;
    initRespLine = "HTTP/1.0 404 Not found\r\n";
    headerContType = "Content-type: text/html\r\n\r\n";
    shouldSendData = false;
    //responseData = (byte*)  "<html><head><title>File not found</title></head>\r\n"
    //                        "<body><h1>404 Not found</h1></body></html>";
  } else {
    if (strncmp(path, "private", 7) == 0) { //check if authentication required i.e. path contains private
      cout << "Inside authentication check" << endl;
      if (!authSuccessful(theData)) { //check if authentication failed
        cout << "Authentication failed - sending 401 error" << endl;
        initRespLine = "HTTP/1.0 401 Unauthorized\r\n";
        headerContType =  "Content-type: text/html\r\n"
                          "WWW-Authenticate: Basic realm=""private""\r\n\r\n";
        shouldSendData = false;
        //responseData = (byte*)  "<html><head><title>401 Unauthorized</title></head>\r\n"
        //                        "<body><h1>401 Unauthorized</h1></body></html>";
      }
    }
  }

  mySocket->Write((byte*)initRespLine, strlen(initRespLine));
  mySocket->Write((byte*)headerContType, strlen(headerContType));
  if (shouldSendData) {
    mySocket->Write(responseData, fileLength);
  }

  delete path;
  delete responseData;
  delete initRespLine;
  delete headerContType;
}


char* HTTPServer::moveToBody(char* theData) {
  char* body = strstr(theData, "\r\n\r\n");
  body += 4;
  return body;
  /**
  bool headerDone = false;
  char* line = strstr((char*)aData, "\r\n");
  line += 2;
  while (!headerDone){
    line = (strstr((char*)line, "\r\n") + 2);
    if(strncmp(line, line - 2, 2) == 0){
      headerDone = true;
    }
  }
  return line + 2;
  */
}

/**
Try to find the header field Authorization: Basic in the request. Header looks
like: Authorization: Basic qWjfhjR124=<CRLF>. Decode qWjfhjR124= using
decodeBase64. The result from the method is a string of the form user:password.
Compare the string with a few invented users with passwords stored in the
class HTTPServer and decide whether the resource should be sent.
*/
bool HTTPServer::authSuccessful(char* theData) {
  //cout << "In authSuccessful method" << endl;
  char* toCompare = "Authorization:";
  char* authHeadLine = strstr(theData, toCompare); //Pointer to first occurence of toCompare
  if (authHeadLine == NULL) { //Request did not include login details
    return false;
  }
  authHeadLine = strchr(authHeadLine, ' '); //Move inbetween Authorization and Basic
  authHeadLine++;
  authHeadLine = strchr(authHeadLine, ' '); //Move inbetween Basic and base64 encoded login details
  authHeadLine++; //Move onto base64 encoded login details
  char* endOfAuthHeadLine = strchr(authHeadLine, '\r\n'); //Pointer to the CRLF after the base64 encoded login details
  char* theAuthDetails = extractString(authHeadLine, (udword)(endOfAuthHeadLine-authHeadLine));

  if (theAuthDetails == NULL) { //Did not enter any auth details
    //cout << "authSuccessful check says you didn't enter any details" << endl;
    return false;
  } else {
    char* decodedAuthDetails = decodeBase64(theAuthDetails);
    //cout << "theAuthDetails: " << theAuthDetails << endl;
    //cout << "decodedAuthDetails: " << decodedAuthDetails << endl;
    if (strncmp("admin:admin", decodedAuthDetails, 11) == 0) {
      delete theAuthDetails;
      delete decodedAuthDetails;
      //cout << "authSuccessful check username:password was correct" << endl;
      return true;
    } else {
      delete theAuthDetails;
      delete decodedAuthDetails;
      //cout << "authSuccessful check username:password was incorrect" << endl;
      return false;
    }
  }
}

/**
Takes as input the requested file name, finds out what type of content the
request is for, and returns the appropriate char sequence with CRLF
*/
char* HTTPServer::contentTypeFromFileName(char* theFileName) {
  char* fullStop = strchr(theFileName, '.');
  if (strncmp(fullStop, ".htm", 4) == 0) {
    return "Content-type: text/html\r\n\r\n";
  } else if (strncmp(fullStop, ".gif", 4) == 0) {
    return "Content-type: image/gif\r\n\r\n";
  } else if (strncmp(fullStop, ".jpg", 4) == 0) {
    return "Content-type: image/jpeg\r\n\r\n";
  }
  return NULL;
}

char* HTTPServer::findPathName(char* str) {
  char* firstPos = strchr(str, ' ');     // First space on line
  firstPos++;                            // Pointer to first /
  char* lastPos = strchr(firstPos, ' '); // Last space on line
  char* thePath = 0;                     // Result path
  if ((lastPos - firstPos) == 1)
  {
    // Is / only
    thePath = 0;                         // Return NULL
  }
  else
  {
    // Is an absolute path. Skip first /.
    thePath = extractString((char*)(firstPos+1),
                            lastPos-firstPos);
    if ((lastPos = strrchr(thePath, '/')) != 0)
    {
      // Found a path. Insert -1 as terminator.
      *lastPos = '\xff';
      *(lastPos+1) = '\0';
      while ((firstPos = strchr(thePath, '/')) != 0)
      {
        // Insert -1 as separator.
        *firstPos = '\xff';
      }
    }
    else
    {
      // Is /index.html
      delete thePath; thePath = 0; // Return NULL
    }
  }
  return thePath;
}

//----------------------------------------------------------------------------
//
// Allocates a new null terminated string containing a copy of the data at
// 'thePosition', 'theLength' characters long. The string must be deleted by
// the caller.
//
char* HTTPServer::extractString(char* thePosition, udword theLength) {
  char* aString = new char[theLength + 1];
  strncpy(aString, thePosition, theLength);
  aString[theLength] = '\0';
  return aString;
}

//----------------------------------------------------------------------------
//
// Will look for the 'Content-Length' field in the request header and convert
// the length to a udword
// theData is a pointer to the request. theLength is the total length of the
// request.
//
udword HTTPServer::contentLength(char* theData, udword theLength) {
  udword index = 0;
  bool   lenFound = false;
  const char* aSearchString = "Content-Length: ";
  while ((index++ < theLength) && !lenFound)
  {
    lenFound = (strncmp(theData + index,
                        aSearchString,
                        strlen(aSearchString)) == 0);
  }
  if (!lenFound)
  {
    return 0;
  }
  trace << "Found Content-Length!" << endl;
  index += strlen(aSearchString) - 1;
  char* lenStart = theData + index;
  char* lenEnd = strchr(theData + index, '\r');
  char* lenString = this->extractString(lenStart, lenEnd - lenStart);
  udword contLen = atoi(lenString);
  trace << "lenString: " << lenString << " is len: " << contLen << endl;
  delete [] lenString;
  return contLen;
}

//----------------------------------------------------------------------------
//
// Decode user and password for basic authentication.
// returns a decoded string that must be deleted by the caller.
//
char* HTTPServer::decodeBase64(char* theEncodedString) {
  static const char* someValidCharacters =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

  int aCharsToDecode;
  int k = 0;
  char  aTmpStorage[4];
  int aValue;
  char* aResult = new char[80];

  // Original code by JH, found on the net years later (!).
  // Modify on your own risk.

  for (unsigned int i = 0; i < strlen(theEncodedString); i += 4)
  {
    aValue = 0;
    aCharsToDecode = 3;
    if (theEncodedString[i+2] == '=')
    {
      aCharsToDecode = 1;
    }
    else if (theEncodedString[i+3] == '=')
    {
      aCharsToDecode = 2;
    }

    for (int j = 0; j <= aCharsToDecode; j++)
    {
      int aDecodedValue;
      aDecodedValue = strchr(someValidCharacters,theEncodedString[i+j])
        - someValidCharacters;
      aDecodedValue <<= ((3-j)*6);
      aValue += aDecodedValue;
    }
    for (int jj = 2; jj >= 0; jj--)
    {
      aTmpStorage[jj] = aValue & 255;
      aValue >>= 8;
    }
    aResult[k++] = aTmpStorage[0];
    aResult[k++] = aTmpStorage[1];
    aResult[k++] = aTmpStorage[2];
  }
  aResult[k] = 0; // zero terminate string

  return aResult;
}

//------------------------------------------------------------------------
//
// Decode the URL encoded data submitted in a POST.
//
char* HTTPServer::decodeForm(char* theEncodedForm) {
  char* anEncodedFile = strchr(theEncodedForm,'=');
  anEncodedFile++;
  char* aForm = new char[strlen(anEncodedFile) * 2];
  // Serious overkill, but what the heck, we've got plenty of memory here!
  udword aSourceIndex = 0;
  udword aDestIndex = 0;

  while (aSourceIndex < strlen(anEncodedFile))
  {
    char aChar = *(anEncodedFile + aSourceIndex++);
    switch (aChar)
    {
     case '&':
       *(aForm + aDestIndex++) = '\r';
       *(aForm + aDestIndex++) = '\n';
       break;
     case '+':
       *(aForm + aDestIndex++) = ' ';
       break;
     case '%':
       char aTemp[5];
       aTemp[0] = '0';
       aTemp[1] = 'x';
       aTemp[2] = *(anEncodedFile + aSourceIndex++);
       aTemp[3] = *(anEncodedFile + aSourceIndex++);
       aTemp[4] = '\0';
       udword anUdword;
       anUdword = strtoul((char*)&aTemp,0,0);
       *(aForm + aDestIndex++) = (char)anUdword;
       break;
     default:
       *(aForm + aDestIndex++) = aChar;
       break;
    }
  }
  *(aForm + aDestIndex++) = '\0';
  return aForm;
}

/**
Exactly like handleGetRequest, but never send the actual resource. Return only
status line and headers.
*/

void HTTPServer::handleHeadRequest(char* theData, udword theLength) {
  char* path = findPathName(theData);
  udword fileLength;
  byte* responseData;
  char* initRespLine;
  char* headerContType;
  if (path == NULL || path == 0) { //Path was either "/" or "/index.htm"
    char* fileName = "index.htm"; //Manually set file name since we know what file to read and send

    responseData = FileSystem::instance().readFile(path, fileName, fileLength);
    initRespLine = "HTTP/1.0 200 OK\r\n";
    headerContType = "Content-Type: text/html\r\n\r\n";
    delete fileName;

  } else { //A absolute path was found
    //cout << "Path is: " << path << endl;
    //Want to find fileName to get file, and fileType for header
    char* first = strchr(theData, ' '); //Points to blank space right before absolute path
    first++; //Need to move of the blankspace onto the '/' for next line of code to work
    char* last = strchr(first, ' '); //Points to blank space right after absolute path
    char* pathAndFile = extractString((char*)first+1, last-first); // +1 to get off '/' and onto the path
    //cout << "pathAndFile is: " << pathAndFile << endl;
    char* fileName = strrchr(pathAndFile, '/'); //Points to '/' just before file name
    fileName++; //Move onto file name
    //cout << "File name is: " << fileName << endl;

    responseData = FileSystem::instance().readFile(path, fileName, fileLength);
    initRespLine = "HTTP/1.0 200 OK\r\n";
    headerContType = contentTypeFromFileName(fileName);
    //cout << "Header: " << headerContType << endl;
    delete first;
    delete last;
    delete pathAndFile;
    delete fileName;
  }

  //Checks go here
  if (responseData == 0) { //check if file was not found
    //cout << "File not found, returning 404 error response" << endl;
    initRespLine = "HTTP/1.0 404 Not found\r\n";
    headerContType = "Content-type: text/html\r\n\r\n";
  } else {
    if (strncmp(path, "private", 7) == 0) { //check if authentication required i.e. path contains private
      //cout << "Inside authentication check" << endl;
      if (!authSuccessful(theData)) { //check if authentication failed
        //cout << "Authentication failed - sending 401 error" << endl;
        initRespLine = "HTTP/1.0 401 Unauthorized\r\n";
        headerContType =  "Content-type: text/html\r\n"
                          "WWW-Authenticate: Basic realm=""private""\r\n\r\n";
      }
    }
  }
  cout << "Calling mysocket->Write" << endl;
  mySocket->Write((byte*)initRespLine, strlen(initRespLine));
  mySocket->Write((byte*)headerContType, strlen(headerContType));

  delete path;
  delete responseData;
  delete initRespLine;
  delete headerContType;
}

/************** END OF FILE http.cc *************************************/

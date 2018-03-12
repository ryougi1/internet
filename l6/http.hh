#ifndef http_hh
#define http_hh

/****************** INCLUDE FILES SECTION ***********************************/

#include "job.hh"

/****************** CLASS DEFINITION SECTION ********************************/
//class TCPSocket;
//class FileSystem;
class HTTPServer : public Job
{
 public:
  HTTPServer(TCPSocket* theSocket);
  //~HTTPServer();

  void doit();
  void handleGetRequest(char* theData, udword theLength);
  void handleHeadRequest(char* theData, udword theLength);
  void handlePostRequest(char* theData, udword theLength);

  bool authSuccessful(char* theData);
  char* findFileName(char* theData);
  char* contentTypeFromFileName(char* theFileName);

  char* findPathName(char* str);
  char* extractString(char* thePosition, udword theLength);
  udword contentLength(char* theData, udword theLength);
  char*  decodeBase64(char* theEncodedString);
  char* decodeForm(char* theEncodedForm);

 private:
   TCPSocket* mySocket;
};

#endif
/****************** END OF FILE *********************************/

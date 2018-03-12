/*!***************************************************************************
*!
*! FILE NAME  : fs.cc
*!
*! DESCRIPTION: FileSystem
*!
*!***************************************************************************/

/****************** INCLUDE FILES SECTION ***********************************/

#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "iostream.hh"
extern "C"
{
#include "system.h"
}

#include "fs.hh"

#define D_fs
#ifdef D_fs
#define trace cout
#else
#define trace if(false) cout
#endif
/****************** FileSystem DEFINITION SECTION *************************/

//----------------------------------------------------------------------------
//

FileSystem::FileSystem() {
  //int slask;
  //cout << "FileSystem created." << endl;
  dynamicPage = NULL;
  lengthOfDynamicPage = 0;
}

//----------------------------------------------------------------------------
//
FileSystem&
FileSystem::instance()
{
  static FileSystem myInstance;
  return myInstance;
}

bool FileSystem::writeFile(char *path,char *name,
			   byte *theData,udword theLength)
{
  /**
  Ignore path and name for now, method is only called for one specific page -
  the dynamic page. NOTE: '\0' is added before method call, so no need to
  account for it here.
  */
  if (dynamicPage != NULL) {
    delete dynamicPage; //Already exists, so delete the old one
    dynamicPage = NULL; //Reinitiate
  }
  dynPageLength = theLength; //Set the length
  /**
  Malloc:
  On success returns a pointer to the memory block allocated by the function.
  The type of this pointer is always void*, which can be cast to the desired
  type of data pointer in order to be dereferenceable.
  If the function failed to allocate the requested block of memory, a null
  pointer is returned.
  */
  dynamicPage = (byte*) malloc(theLength);
  if (dynamicPage == NULL) {
    cout << "Malloc returned null pointer, unable to allocate memory for dynamic page" << endl;
    return false;
  }
  memcpy(dynamicPage, theData, theLength);
  return true;
}

typedef struct lzhead
{
        unsigned char           HeadSiz,HeadChk,HeadID[5]; // 0 1 2
        unsigned char           PacSiz[4];                 // 7
        unsigned char           OrgSiz[4];                 // 11
        unsigned char           Ftime[4];                  // 15
        unsigned char           Attr,Level;                // 17
        unsigned char           name_length;               // 18
        unsigned char           Fname[1];                  // 19
} LzHead;

const byte FileSystem::myFileSystem[]=
{
#include "lhafile.bin"
};

byte *FileSystem::readFile(char *path,char *name,udword& theLength)
{
  //Looking for dynamic page, and it has been set.
  if(strncmp(name, "dynamic.htm", 11) == 0 && dynamicPage != NULL) {
    theLength = lengthOfDynamicPage; //Set the length variable
    return dynamicPage; //Return the dynamic page
  }

  int file_size=sizeof(FileSystem::myFileSystem);
  int curr_size=0;
  int curr_file_size=0;

  while((curr_size<(file_size-16))&&(curr_size>=0))
  {
    LzHead *header=(LzHead *)&FileSystem::myFileSystem[curr_size];
    char *file_path;
    udword the_file_size;

    curr_size+=header->HeadSiz+2;

    curr_file_size=header->PacSiz[0];
    curr_file_size+=header->PacSiz[1]<<8;
    curr_file_size+=header->PacSiz[2]<<16;
    curr_file_size+=header->PacSiz[3]<<24;

    the_file_size=curr_file_size;

    // check if it is the correct file.
    if ((!strncmp(name,header->Fname,header->name_length))&&
	(header->name_length))
      {
	bool endCheck=false;

        byte *file_ptr=&FileSystem::myFileSystem[curr_size];


	//may have found it check path

	if ((header->HeadSiz>27)&&(header->Level==1)&&
	    (header->Fname[header->name_length+17]==2))
	  {
	    byte *file_path=&header->Fname[header->name_length+18];
	    byte *file_path2=file_path;
	    udword path_len;

	    endCheck=true;

	    //hitta slut
	    do
	      while(*(file_path2++)!=0xff);
	    while (*file_path2!=7);

	    the_file_size-=((int)(file_path2-file_ptr))+9;
	    file_ptr=file_path2+9;

	    printf("%p\n",file_ptr);

	    path_len=file_path2-file_path;
	    if (!strncmp(path,file_path,path_len))
	      {
		if (strlen(path)==path_len)
		  {
		    endCheck=false;
		  }
	      }
	  }
	else
	  {
	    file_ptr += 19;
            the_file_size -= 19;
	    if (*path)
	      {
		endCheck=true;
	      }
	  }

	if (!endCheck)
	  {
	    //found it!

	    // calc size and ptr
	    theLength=the_file_size;
	    return file_ptr;
	  }
      }
    curr_size+=curr_file_size;
  }
  return 0;
}


/****************** END OF FILE fs.cc *************************************/

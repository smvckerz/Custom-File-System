/**************************************************************
* Class::  CSC-415-0# Spring 2024
* Name:: Eduardo Enrique Munoz Alvarez
* Student ID:: 922499965
* GitHub-Name:: smvckerz
* Project:: Assignment 5 – Buffered I/O read
*
* File:: b_io.c
*
* Description::
*
**************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdint.h>

#include "b_io.h"
#include "fsLowSmall.h"

#define MAXFCBS 20	//The maximum number of files open at one time
#define B_CHUNK_SIZE 512


// This structure is all the information needed to maintain an open file
// It contains a pointer to a fileInfo strucutre and any other information
// that you need to maintain your open file.
typedef struct b_fcb
	{
	fileInfo * fi;	//holds the low level systems file info
	int currentUse;
    char *buff;
    int buffLength;
    int buffPosition;
    long filePosition;

	// Add any other needed variables here to track the individual open file
	} b_fcb;
	
//static array of file control blocks
b_fcb fcbArray[MAXFCBS];

// Indicates that the file control block array has not been initialized
int startup = 0;	

// Method to initialize our file system / file control blocks
// Anything else that needs one time initialization can go in this routine
void b_init ()
	{
	if (startup)
		return;			//already initialized

	//init fcbArray to all free
	for (int i = 0; i < MAXFCBS; i++)
		{
		fcbArray[i].fi = NULL; //indicates a free fcbArray
		}
		
	startup = 1;
	}

//Method to get a free File Control Block FCB element
b_io_fd b_getFCB ()
	{
	for (int i = 0; i < MAXFCBS; i++)
		{
		if (fcbArray[i].fi == NULL)
			{
			fcbArray[i].fi = (fileInfo *)-2; // used but not assigned
			return i;		//Not thread safe but okay for this project
			}
		}

	return (-1);  //all in use
	}

// b_open is called by the "user application" to open a file.  This routine is 
// similar to the Linux open function.  	
// You will create your own file descriptor which is just an integer index into an
// array of file control blocks (fcbArray) that you maintain for each open file.  
// For this assignment the flags will be read only and can be ignored.

b_io_fd b_open (char * filename, int flags)
	{
	if (startup == 0) b_init();  //Initialize our system

	//*** TODO ***//  Write open function to return your file descriptor
	//				  You may want to allocate the buffer here as well
	//				  But make sure every file has its own buffer

	fileInfo *fi = GetFileInfo(filename);

	if(fi == NULL)
	{
		return -1;
	}

	//Sees if theres an available control block index for the files
	int getFCB = b_getFCB();

	if(getFCB < 0)
	{
		printf("No spaee left");
		return -1;
	}

	//Allocates buffer for the file(s)
	char *buffer = malloc(B_CHUNK_SIZE);

	if(!buffer)
	{
		fcbArray[getFCB].fi = NULL;
		return -1;
	}

	//Fills the FCB fields with the file info and also the details
	//of the buffer
	b_fcb *populate = &fcbArray[getFCB];
 
	//Populating struct variables with thei required information
	populate -> fi = fi;
	populate -> buff = buffer;
	populate -> buffLength = 0;
	populate -> buffPosition = 0;
	populate -> filePosition = 0;

	return getFCB;

	// This is where you are going to want to call GetFileInfo and b_getFCB
	}



// b_read functions just like its Linux counterpart read.  The user passes in
// the file descriptor (index into fcbArray), a buffer where thay want you to 
// place the data, and a count of how many bytes they want from the file.
// The return value is the number of bytes you have copied into their buffer.
// The return value can never be greater then the requested count, but it can
// be less only when you have run out of bytes to read.  i.e. End of File	
int b_read (b_io_fd fd, char * buffer, int count)
	{
	//*** TODO ***//  
	// Write buffered read function to return the data and # bytes read
	// You must use LBAread and you must buffer the data in B_CHUNK_SIZE byte chunks.

	if (startup == 0) b_init();  //Initialize our system

	// check that fd is between 0 and (MAXFCBS-1)
	if ((fd < 0) || (fd >= MAXFCBS))
		{
		return (-1); 					//invalid file descriptor
		}

	// and check that the specified FCB is actually in use	
	if (fcbArray[fd].fi == NULL)		//File not open for this descriptor
		{
		return -1;
		}	

	//Calling struct 
	b_fcb *call = &fcbArray[fd];

	//Ensuring fi is not null, if so it returns -1
	if(call -> fi == NULL)
	{
		return -1;
	}

	// Gets the total size of the file
	long fileSize = call -> fi -> fileSize;

	if(call -> filePosition >= fileSize)
	{
		return 0;
	}

	int bytesCopied = 0;

	//Keeps comparing until it reaches a condition or the end of the file
	while(bytesCopied < count && call -> filePosition < fileSize)
	{
		//If the buffer is empty it will fill it
		if(call -> buffPosition == call -> buffLength)
		{
			//Does the block number calculation to read from the disk
			uint64_t blockNum = call -> fi -> location + (call -> filePosition / B_CHUNK_SIZE);
			uint64_t blocksRead = LBAread(call -> buff, 1, blockNum);

			if(blocksRead == 0)
			{
				break;
			}

			//Resets the buffer
			call -> buffLength = B_CHUNK_SIZE;
			call -> buffPosition = 0;
		
		}

		//Calculates the how much of the data is left and what is needed still
		int available = call -> buffLength - call -> buffPosition;
		int needed = count - bytesCopied;
		int remaining = fileSize - call -> filePosition;
		int chunk;

		if(available < needed)
		{
			chunk = available;
		}
		else
		{
			chunk = needed;
		}
		if(chunk > remaining)
		{
			chunk = remaining;
		}

		//Copies the chunk from the buffer to the user buffer
		memcpy(buffer + bytesCopied, call -> buff + call -> buffPosition, chunk);

		//Updates the positions to the current positions
		call -> buffPosition = call -> buffPosition + chunk;
		call -> filePosition = call -> filePosition + chunk;
		bytesCopied = bytesCopied + chunk;
	} 

	return bytesCopied;	

	// Your Read code here - the only function you call to get data is LBAread.
	// Track which byte in the buffer you are at, and which block in the file	
	}
	


// b_close frees and allocated memory and places the file control block back 
// into the unused pool of file control blocks.
int b_close (b_io_fd fd)
	{
	//*** TODO ***//  Release any resources

	if(fd < 0 || fd >= MAXFCBS || fcbArray[fd].fi == NULL)
	{
		return -1; //If statement checks whether if the FD is invalid
				   //or wasnt open and returns -1 if either condition is met
	}

	//Allocates memory for the buffer
	free(fcbArray[fd].buff);
	//marks the FCB free
	fcbArray[fd].fi = NULL;
	return 0;
	}
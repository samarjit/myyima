/*******************************************************************************

                          Yima Personal Edition v1.0
                               January 10, 2003

                            Streaming Media System
                        http://streamingmedia.usc.edu

                        Department of Computer Science
                        Integrated Media Systems Center
                       University of Southern California
                             Los Angeles, CA 90089

    Developed by:

    Roger Zimmermann
    Cyrus Shahabi
    Kun Fu
    Sahitya Gupta
    Mehrdad Jahangiri
    Farnoush Banaei-Kashani
    Hong Zhu


Copyright 2003, University of Southern California. All Rights Reserved.

"This software is experimental in nature and is provided on an AS-IS basis only.
The University SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED,
INCLUDING WITHOUT LIMITATION ANY WARRANTY AS TO MERCHANTIBILITY OR FITNESS FOR
A PARTICULAR PURPOSE.

This software may be reproduced and used for non-commercial purposes only,
so long as this copyright notice is reproduced with each such copy made."

*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include "interface.h"
#include "../sysinclude/OSDebug.h"

/* module debugging */
#define MEMORY_DEBUG       1  
#define DEEP_DEBUG         1



// These two pointer are providing access to two link lists: "Session" which keeps 
// track of sessions, and "usedBlock" which keeps track of blocks of Movies, and 
// the number of sessions that using those blocks

static struct sessionInfo *sessions = NULL;

static struct usedBlock *usedBlocks = NULL;

/*********************************************************************************
 * Name: localReadBlock
 * Input: movieName, BlockName
 * Output: NumOfPackets, packet lengths of all RTP packets in a block, pointer to a newly created block data.
 *
 * Description: The following function looks at the currently fetchced blocks. If 
 *              there are other sessions that already have fetched the requested 
 *              block it won't be fetched again and a counter shows that how
 *              many sessions are using this block will be incremented. 
 *              Otherwise a new block will be fetched and the counter set to 1.
 *
 * Date: April 2001
 * Developed by: Atousa Golpayegani
 * Modified by: Sahitya Gupta 
 *********************************************************************************/
static char *localReadBlock(const char *movieName, const char *BlockName,
			   int* NumOfPackets,
			   UInt32 *pkt_lens_in_a_block)
{
	char* newBlock;
	struct usedBlock *newUsedBlock; // pointer to hold a newly creted usedBlock structure
	struct usedBlock *prevPtr, *curPtr; // trace the UsedBlocks' linked list
  
	printf("--localReadBlock---%s\n", BlockName);
  
	// this loop is tracing the link list to find if the given blockName has been fetched.
	curPtr = prevPtr = usedBlocks;
  	while (curPtr) {
      		if (!strcmp(curPtr->blockName,BlockName)) {

			//  block found in a usedBlcks list.

			getNumPackets_PackSize(movieName, BlockName,
					       NumOfPackets,
					       (int *)pkt_lens_in_a_block);
			if (*NumOfPackets != curPtr->numberOfPackets) {
				printf("The expected number %d of packets in block %s is different from that %d of packets in used block\n", 
				*NumOfPackets, BlockName, curPtr->numberOfPackets);
				exit(0);
			}
			*NumOfPackets = curPtr->numberOfPackets;

			curPtr->counter++;
			return curPtr->blockPtr;
		}
	
		// FALL THROUGH	
		prevPtr = curPtr;
		curPtr = curPtr->next;
	}

	// curPtr == NULL

	// cached block is not found.
	// let's fetch the block from the disk and attach to the usedBlocks list.
 
	// create a memory block to hold data (prepare more data space)

	newBlock = (char*)malloc(MAX_PKTSIZE * NUMPKTS);
	if(!newBlock) {
		printf("\nlocalReadBlock(): Error in allocating memory for newBlock\n");
		exit(0);
	}
	newUsedBlock = (struct usedBlock *)malloc(sizeof(struct usedBlock));
	if (!newUsedBlock) {
		printf("\nlocalReadBlock() : Error in allocating memory for newUsedBlock\n");
		exit(0);
	}

	// fetch block data from disk and
	// and returns a pointer to this block beside the number and size of its packets
	readBlock(movieName, BlockName,
		  newBlock,
		  NumOfPackets, 
		  (int *)pkt_lens_in_a_block);
 
	// fill out the fields in UsedBlock structure. 
	strncpy(newUsedBlock->blockName,BlockName,BLOCKNAMESIZE);
	newUsedBlock->blockPtr = newBlock;
	newUsedBlock->numberOfPackets = *NumOfPackets;
	newUsedBlock->counter = 1; // this block is possessed by a caller.
	newUsedBlock->next = NULL;
  
 	// attach newly created UsedBlock to the end of the UsedBlocks list. 
	if (prevPtr) {
		prevPtr->next = newUsedBlock;
	}
	else {    
		usedBlocks = newUsedBlock;
	}

	return newBlock;
}


/****************************************************************************************
 * Name: releaseUsedBlock
 * Input: blockPtr to be freed
 * Output: 0 if block not found. 1 if block was removed 
 * Description: The following function removes a block in the cache
 * Date: April 2001
 * Developed by: Atousa Golpayegani
 * Modified by: Sahitya Gupta
 ****************************************************************************************/
static int releaseUsedBlock(char *blockPtr)
{ 
	struct usedBlock *cur, *prev;  

	if (!blockPtr) { 
		// nothing to be done on the NULL pointer.                    
		return 1;
	}

	// find the block to be released in the usedBlock list.
	prev = cur = usedBlocks;
	while (cur) {
		if(cur->blockPtr == blockPtr) {
			break;
		}

		// FALL THROUGH
		prev = cur;  
		cur = cur->next;
	}
  
	if (!cur) {
		printf("block not found\n");
		return 0;
	}

	// cur != NULL

	// block to be released is found

	ASSERT(cur->counter > 0);
	//update the counter
	cur->counter--;

	if(cur->counter == 0) {
		// if no other session uses this block
		// release the block and usedBlock structure.

		// detach the usedBlock from the list
		if (cur == usedBlocks) {
			usedBlocks = cur->next;
		}
		else {
			prev->next = cur->next;
		}

		// free all unnecessary data.
		free( (void *)cur->blockPtr);
		free((void *)cur);
 	}
	return 1;
}


/*********************************************************************************
 * Name: Flib_init 
 * Input: -
 * Output:-
 * Description: This function is for initialization of session pointer to NULL
 *
 * Date: April 2001
 * Developed by: Atousa Golpayegani 
 * Modified by: Sahitya Gupta
 *********************************************************************************/

void Flib_init()
{
	if (InitFileSystem() == -1) {
		printf("Initialization of the file system failed\n");
		exit(0);
	}
}



/*********************************************************************************
 * Name: Flib_play 
 * Input: sessionId, movieName, nptRange(requested range)
 *
 * Description: The following function prefetches two blocks in the buffer
 *
 * Date: April 2001
 * Developed by: Atousa Golpayegani 
 * Modified by: Sahitya Gupta
 *********************************************************************************/

Boolean Flib_play(UInt64 sessionId, const char *movieName, struct NPT_RANGE_T nptRange)
{  
	char blockName[BLOCKNAMESIZE];
	char nextBlockName[BLOCKNAMESIZE];
	struct sessionInfo *session, *temp_session; 
	  
	// trace sessions whether a given session id is in the list.
	session = NULL;
	temp_session = sessions;
	while (temp_session) {
     		if (temp_session->sessionId == sessionId) {
			session = temp_session;
			break;
		}
		temp_session = temp_session->next;
	}
  
	if (session) { 
		printf(">>>> found the session from the list.\n");

		// release the block data in cached block list.
		(void) releaseUsedBlock(session->blockPtr);
		(void) releaseUsedBlock(session->nextBlockPtr);

		// clean up the block pointers
		session->nextBlockPtr = session->blockPtr = NULL; 
	}
	else {
		// there is no session associated.

		// create a new session structure.
		session = (struct sessionInfo *)malloc(sizeof(struct sessionInfo));  
		if (!session) {
			printf("\nflib_play() : Error in allocating memory for newSession\n");
			exit(0);
		}

		// attach newly created session to the head of "sessions" list.
		session->next = sessions;
		sessions = session;
	}

	// fill the fields in the structure.
	session->sessionId = sessionId;
	// number of RTP packets for next block is not configured yet.
	session->nextBlockNumOfPackets =0;

	strncpy(session->movieName, movieName, BLOCKNAMESIZE);

	// get the first block number from a movie name and a given NPT start range
	// and curresponding timestamp value.
	getStartBlockName(movieName,nptRange.startLocation,blockName);  
  
	// read the first block from disk
	// and configure relevant data structures.
	session->blockPtr = localReadBlock(movieName,blockName,
					  &session->numOfPackets,
					 (UInt32 *)session->pkt_lens_in_a_block);

	// configure the start index number of packets in the block.
	session->currentPacketIndex=0;
      
	// fetch next block from the disk in advance.
 	getNextBlockName(movieName, blockName, nextBlockName);
	if (nextBlockName[0]) { // if there exist next block fetched
		strncpy(session->nextBlockName, nextBlockName, BLOCKNAMESIZE);
		session->nextBlockPtr = localReadBlock(movieName,nextBlockName,
						       &session->nextBlockNumOfPackets, 
						       NULL); // we don't have to store packet lengths
							      // of next block file at this moment.
							      // they will be loaded later.
	}
	else {
		printf("________No Next Block_______________________\n");
		session->nextBlockPtr = NULL;
		session->nextBlockName[0] = 0;
	}
   
 	return TRUE;
}


/****************************************************************************************
 * Name: Flib_getNextPacket
 * Input: sessionId
 * Output: nextRtpPacketPointer, nextRtpPacketSeqNum, nextPacketSize, nextRtpPacketTimestamp
 *         lastPacketFlag, lastBlockPointer
 *
 * Return val:  -1: session not found 
 *               0: end of Moive and NO MORE PACKETS TO SEND
 *               1: succeed and there are more packets
 *              
 * Description: This function checks if the sessionId exists or not. If it does't exists
 *              returns an error, else returns a pointer to the ith packet of the block.
 *              Besides, if the packet that is going to be sent is the last packet in 
 *              the block, lastPacketFlag will be set to 1, and lastBlockPointer points
 *              to that block, and the next block will be read and the index  points to
 *              the begining of that block (ready for the next reading)
 *
 * Date: April 2001
 * Developed by: Atousa Golpayegani
 * Modified by: Sahitya Gupta
 ****************************************************************************************/

int Flib_getNextPacket(	UInt64 sId,
			char **nextRtpPacketPointer, 
			UInt32 *nextRtpPacketSeqNum, 
			UInt32 *nextPacketSize, 
			UInt32 *nextRtpPacketTimestamp,
			Boolean *lastPacketFlag, 
			char **lastBlockPointer)
{
	struct rtp_header *nextRtpPacketPtr = NULL;
	struct sessionInfo *current;
	int offset;

	//finding the session with the given sessionId  
	current = sessions;
	while(current) {
		if(current->sessionId == sId) {
			break;
		}
		current= current->next;
	}
	if (!current) {
		printf("Flib_getNextPacket: there is no associated session.\n");
		// do nothing
		return 0;
	}

	// found the corresponding session
      
	if (!current->blockPtr) {
		return 0;   // end of Moive and NO MORE PACKETS TO SEND
	}

	// there exists a block that has been retrieved.

	// find the correct memory offset for RTP packets to read from the data block.
	offset = 0;
	for (int i = 0; i < current->currentPacketIndex; i++) {
		// it's a time consuming routine.
		// you can improve it by caching the intermediate information.
		offset += current->pkt_lens_in_a_block[i];
	}

	// prepare fields to be ready for network transmission immediately.
	// fill out RTP memory location, sequence number, timestamp and its packet length to send.
	nextRtpPacketPtr = (struct rtp_header *)(current->blockPtr + offset);
	*nextRtpPacketPointer = ((char*)nextRtpPacketPtr);
	*nextRtpPacketSeqNum = (UInt32)htons(nextRtpPacketPtr->SeqNum);
	*nextRtpPacketTimestamp= htonl(nextRtpPacketPtr->TimeStamp);
	*nextPacketSize =current->pkt_lens_in_a_block[current->currentPacketIndex];


	// prepare fields that will be used in the next round.
	if( (current->currentPacketIndex%NUMPKTS) < (current->numOfPackets-1)) {
	      
		// the last RTP packet has not been reached yet.

		// you need to set up lastPacketFlag and BlockPointer correctly.
		*lastPacketFlag= FALSE;
		*lastBlockPointer = NULL;
	      	      
		// proceed to the next packet index.
		current->currentPacketIndex++;
	}
	else {
		//last packet of the block is reached 
	      
		// What needs to be done:
		// set the lastPacketFlag
		// release the current block and set the previously next block as the current 
		// prefetch a new block
		// return the last packet
	      
              	      
		//since the last packet has been reached
		*lastPacketFlag = TRUE;
		*lastBlockPointer = current->blockPtr;
	  
		// get ready for the next time	  
		current->blockPtr = current->nextBlockPtr;
		current->nextBlockPtr = NULL;

		// load the number of RTP packets and their length 
		// in a next block to read.
		getNumPackets_PackSize(current->movieName, 
					current->nextBlockName, 
					&current->numOfPackets, 
					current->pkt_lens_in_a_block);

		current->currentPacketIndex = 0;
	}

	return(1);
}


/****************************************************************************************
 * Name: Flib_tear_down
 * Input: sessionId, MovieEndFlg
 * Output: 0 if session not found. 1 if session was 
 * Description: The following function removes a session. If MovieEndFlg = TRUE, it does not free the blocks 
 * Date: April 2001
 * Developed by: Atousa Golpayegani
 * Modified by: Sahitya Gupta
 ****************************************************************************************/

int Flib_tear_down(UInt64 sId, Boolean MovieEndFlg)
{
	struct sessionInfo *prev, *cur;

	cur = prev = sessions;
	if (!cur) {
		// There is no element in the sessions, return error      
		return 0;
	}

	if(cur->sessionId == sId) {
		sessions = (sessions)->next;
	}
	else {
		cur = (sessions)->next;
		//searching all the nodes in the session to find the sessionId
		while (cur) {
			if (cur->sessionId == sId) {
				// delete curPtr node
				// curPtr are pointng to the deleted node
				prev->next = cur->next;
				printf("Deleted session %llu\n",cur->sessionId);
				break;
			}

			// FALL THROUGH
			prev = cur;	      
			cur = cur->next;
		}
	}

	if(!cur) {
		// sessionId not found return error
		printf("<YimaJade 1.1: FLIB Error>: Session already deleted or not created\n");
		return 0;
	}

	// curPtr != NULL

	// matched sessionId found
	// "cur" is pointing at it
	// update usedBlocks counters of the current and the next packet of the deleted session 
	if (MovieEndFlg == FALSE) {
		(void) releaseUsedBlock(cur->blockPtr);
		(void) releaseUsedBlock(cur->nextBlockPtr);
 		cur->blockPtr = cur->nextBlockPtr = NULL;
	}

	if (cur) {
		// free allocated session structure.
		free((void *) cur);
	}

	return 1;
}



/****************************************************************************************
 * Name:  Flib_lastpktofBlock
 * Input: session id, blockPtr
 * Output: 0 if block not found. 1 if block was removed 
 * Description: The following function Updates the usedBlock list of the given blockName 
 * Date: April 2001
 * Developed by: Atousa Golpayegani
 * Modified by: Sahitya Gupta
 ****************************************************************************************/

int Flib_lastpktofBlock(UInt64 sId, char *blockPtr)
{ 
	struct sessionInfo *session;

	session = sessions;
	while (session) {
		if(session->sessionId == sId) {
			break;
		}
		session = session->next;
	}

	if (!session) {
		printf("Flib_lastpktofBlock: No matched session exists\n");
		return 0;
	}

	getNextBlockName(session->movieName, session->nextBlockName, session->nextBlockName); 
	if ( session->nextBlockName[0] == 0) {
		// no more block to fetch
		session->nextBlockPtr = NULL;
		session->nextBlockNumOfPackets = 0;
	}
	else {
		// read next block and the number of RTP packets.
		session->nextBlockPtr = localReadBlock(session->movieName, 
						session->nextBlockName, 
						&(session->nextBlockNumOfPackets),NULL);
	}

	// release previous block data.
	return (releaseUsedBlock(blockPtr));
}


/****************************************************************************************
 * Name:  Flib_get_npt_range
 * Input: movieName
 * Output: nptRange
 * Description: The following function returns the duration of the whole movie
 * Date: April 2001
 * Developed by: Atousa Golpayegani
 * Modified by: Sahitya Gupta 
 ****************************************************************************************/

FLIBerror Flib_get_npt_range(const char *movieName, struct NPT_RANGE_T *nptRange, MOV_INFO_T *movInfo)
{
	MOVIE *m;
	
	// Initializing the start value. It is because the getMovieDetails function
	// gets a double value as input.

	nptRange->startLocation =0;
	m = getMovieDetails(movieName);
	if (!m) {
		printf("Error in retrieving movie details\n");
		exit(0);
	}
	movInfo->duration_in_seconds = m->time; 
	movInfo->movie_size_in_bytes = m->size;
	//samarjit
	movInfo->timestamp_step_perPkt = 1;
	//samaarjit

	nptRange->stopLocation = (double)m->time;  

	if(nptRange->stopLocation != 0) {
		return(FLIB_OK);
	}
	return(FLIB_FAIL);  
}

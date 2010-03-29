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

#ifndef _FLIB_INTERFACE_H
#define _FLIB_INTERFACE_H

#include "../sysinclude/Yima_module_interfaces.h"
#include "FileInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* data structure that caches blocks that have been retrieved. */
struct usedBlock {
	int	counter; // how many sessions use the current block data
	char	blockName[BLOCKNAMESIZE]; // blockfilename
	char	*blockPtr;		  // pointer to the block data region
	int	numberOfPackets;	  // number of RTP packets stored in the block

	struct usedBlock *next;
};

struct sessionInfo {
	UInt64	sessionId;
	char	movieName[BLOCKNAMESIZE];
	char	nextBlockName[BLOCKNAMESIZE]; // temporary storage to store next block file name to read
	char	*blockPtr;     // currently used block data
	int	numOfPackets;  // number of RTP packets stored in the "blockPtr"
	int	pkt_lens_in_a_block[NUMPKTS];
	char	*nextBlockPtr; // pointer to tne next block data to read
	int	nextBlockNumOfPackets; // number of RTP packets stored in the "nextBlockPtr"
	int	currentPacketIndex; // current index to a RTP packet in the block data

	struct sessionInfo *next;
};


/* in retrieve.c */
extern int InitFileSystem();
extern MOVIE *getMovieDetails(const char* movieName);
extern int getStartBlockName(const char* movieName,
			     float time,
			     char* blockName);
extern int getNextBlockName(const char* movieName, 
			    const char* blockName,
			    char* nextblockName);
extern int readBlock(const char* movieName, 
		     const char* blockName,
		     char* buff_addr, 
		     int* numpackets,
		     int* pkt_lens_in_a_block);
extern int getNumPackets_PackSize(const char* movieName, 
				  const char* blockName, 
				  int* numpackets, 
				  int* pkt_lens_in_a_block);
    
/* in interface.c */ 
extern void 	Flib_init();
extern Boolean	Flib_play(UInt64 sessionId, 
		  const char *movieName,
		  struct NPT_RANGE_T nptRange);
extern int 	Flib_tear_down(UInt64 sId, Boolean MovieEndFlg);

extern int 	Flib_getNextPacket(UInt64 sessionId,
	 	      char **nextRtpPacketPointer,
		      UInt32 *nextRtpPacketSeqNum,
		      UInt32 *nextPacketSize,
	 	      UInt32 *nextRtpPacketTimestamp,
		      Boolean *lastPacketFlag,
		      char **lastBlockPointer);
		      

extern int 	Flib_lastpktofBlock(UInt64 sId, char *blockPtr);

extern FLIBerror Flib_get_npt_range(const char *movieName, 
			     struct NPT_RANGE_T *nptRange, 
			     struct MOV_INFO_T *movInfo);



#ifdef __cplusplus
}
#endif 


#endif /* _FLIB_INTERFACE_H */

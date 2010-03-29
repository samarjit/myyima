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

#ifndef __FILEINTERFACE_H
#define __FILEINTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif

#define NUMPKTS 500      // number of RTP packets per block file
#define MAX_PKTSIZE 1450 // max RTP length

#define DIRECTORY "/home/bseo/YimaPE_v1.1/Streams/BLOCKS/"
#define ConfigFile "../Server/config"

#define BLOCKNAMESIZE 256
#define RTP_Packet_HeaderSize 12

/* keep in mind the following fields are in network order. */
struct rtp_header{
	unsigned int V:2 ;
	unsigned int P:1;
	unsigned int X:1;
	unsigned int CC:4;
	unsigned int M:1;
	unsigned int Pt:7;
	unsigned int SeqNum:16;
	unsigned int TimeStamp:32;
	unsigned int SSRC:32;
};
  
struct packet{
	struct rtp_header header;
	char*	data;
};

typedef struct movie {
	char	name[BLOCKNAMESIZE];
	unsigned long long size;
	int	numpkts;
	int	pktsize;
	int	time;

	// TODO:
	// add more fields for internal use

	struct	movie *next;
} MOVIE;

#ifdef __cplusplus
}
#endif

#endif /* __FILEINTERFACE_H */

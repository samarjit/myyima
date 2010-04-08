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

#ifndef _SCHEDULER_SYSMON_H_
#define _SCHEDULER_SYSMON_H_

/* include files */
#include "../sysinclude/Yima_module_interfaces.h"

/* Constant Definitions */
#define MAX_MOVIE_NAME_LEN     256      /* maximum movie length */

// How earlier we send a RTP packet than designated time.
#define MAX_ADVANCED_TIME      100000     /* in 100 ms */

// deltaP is no longer used in version 1.1
#define DEFAULT_DELTAP         372
                                        

// Above lines were commented by Sahitya Gupta <end>

/* server name */
char* g_ServerName = "Yima2";
/* server version */
char* g_ServerVer= "0.0";
/* server building date */
char* g_ServerBuildDate= "03/12/01";

/* type definitions */
typedef enum Active_T {
   INACTIVE,
   ACTIVE
} Active_T;

/*************************************************************************/
/* SessionState type is mapped to RTSP Server State machine              */
/*************************************************************************/
/* RTSPD Server State Machine                                            */
/* The server can assume the following states                            */
/*   Init:                                                               */
/*      The initial state, no valid SETUP has been received yet.         */
/*                                                                       */
/*   Ready:                                                              */
/*	Last SETUP received was successful, reply sent or after          */
/*	playing, last PAUSE received was successful, reply sent.         */
/*                                                                       */
/*   Playing:                                                            */
/*	Last PLAY received was successful, reply sent. Data is being     */
/*      sent.                                                            */
/*                                                                       */
/*  In general, the server changes state on receiving requests.          */
/*      state           message received  next state                     */
/*    	Init            SETUP             Ready                          */
/*                      TEARDOWN          Init                           */
/*      Ready           PLAY              Playing                        */
/*                	SETUP             Ready                          */
/*                      TEARDOWN          Init                           */
/*      Playing         PLAY              Playing                        */
/*        		PAUSE             Ready                          */
/*	                TEARDOWN          Init                           */
/*	                SETUP             Playing                        */
/*************************************************************************/
typedef enum SessionState_T {
   INIT,
   READY,
   PLAYING,
   TEARDOWN,  /* teardown KunFu Debug !!!???*/
   FINISH     /* end of movie !!!???*/
} SessionState_T;

typedef struct Session_T {
   UInt64 session_id;
   SInt32 rtsps_fd;   /* socketfd for corresponding rtsp server */

   char mov_name[MAX_MOVIE_NAME_LEN];
   UInt16 clientRTPPort;
   UInt16 clientRTCPPort;
   UInt32 clientIPAddr; /* 32-bit IPv4 address */
                        /* struct in_addr {
                                  in_addr_t s_addr; 
                           };
                           
                           struct sockaddr_in {
                                  uint8_t sin_len;
                                  sa_family_t sin_family;
                                  in_port_t sin_port;
                                  struct in_addr sin_addr;
                                  char sin_addr;
                                  char sin_zero[8];
                           };
			 */


   Active_T dataStatus; /* ???!! */
   SessionState_T currentSessionState;
   pthread_mutex_t curSessionState_mutex;

   double deltaP; /* not used in version 1.1 */
   double defaultDeltaP;
   MOV_INFO_T movInfo;

   char  *nextRTPPkt_p; /* the address of the next RTP packet */
   UInt32 nextRTPPktSeqNo;
   UInt32 nextRTPPktSize;
   SInt64 nextRTPPktSentOutTime; /*??? in milli-seconds since Jan 1, 1970 */
                                 /* in Micro-seconds since Jan 1, 1970 */
   UInt32 nextRTPPktTimeStamp;

   /* teardown client crash flag */
   UInt16 crashFlg; /*if set to 1, not Teardown respond to RTSP server */

   /* used by Flib module for buffer management <begin> */
   Boolean lastRTPPktOfBlock_Flg; /* whether this is the last RTP */
                                  /* packet of Block of not       */
   Boolean MovieEndHandlingFlg;   /* */

   char  *blockPtr;               /* block pointer */
   /* used by Flib module for buffer management <end> */

   SInt64 startPlayTime;   /* start play time in milli-second since Jan 1, 1970 */
   SInt64 maxAdvancedTime; /* current mux advanced time for this stream */

   UInt32 previousRTPPktSeqNo;  
   SInt64 previousRTPPktSentOutTime; /* in milli-seconds since Jan 1, 1970 */
   UInt32 SSRC; //samarjit
} Session_T;

pthread_mutex_t FLIB_access_mutex; /* added for ???!!! */

#endif  /* _SCHEDULER_SYSMON_H_ */



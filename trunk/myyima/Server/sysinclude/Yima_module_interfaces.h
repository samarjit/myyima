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

#ifndef _YIMA_MODULE_INTERFACES_H_
#define _YIMA_MODULE_INTERFACES_H_

/* include files */
#include "OSDefines.h"


#define BASE_PORT_NUMBER 50000

#define RTSPSPORT		(BASE_PORT_NUMBER)
#define LINKPORT		(BASE_PORT_NUMBER+1)
#define SYSMOND_WAITING_PORT 	(BASE_PORT_NUMBER+2)
#define SERVER_RTP_PORT_NO 	(BASE_PORT_NUMBER+3)
#define SERVER_RTCP_PORT_NO 	(BASE_PORT_NUMBER+4)

/*==============================================================================*/
/* data structures shared by all interfaces */
#define YIMA_MAX_MOVIE_NAME_LEN  256 /* the maximum length of movie */

typedef struct NPT_RANGE_T {
  float startLocation;
  float stopLocation;  
} NPT_RANGE_T;

typedef struct MOV_INFO_T {
  float duration_in_seconds;
  UInt64 movie_size_in_bytes;
  int timestamp_step_perPkt;
} MOV_INFO_T;


/*==============================================================================*/
/*****************************************************
 * Interface to access SystemMonD Modules  <begin>   *
 *****************************************************/
/* The following are commands that a application can send */
/* to the SysMonD Module */
typedef enum {
  SYSMOND_RTSP_OPTIONS_RESP,	/* options response  */
  SYSMOND_RTSP_OPTIONS_REQ,		/* options request  */
  SYSMOND_RTSP_DESCRIBE_REQ,	/* describe request  */
  SYSMOND_RTSP_DESCRIBE_RESP,   /* describe response */
  SYSMOND_RTSP_SETUP_REQ,		/* setup request  */
  SYSMOND_RTSP_SETUP_RESP,     	/* setup response */
  SYSMOND_RTSP_PLAY_REQ,        /* play request */
  SYSMOND_RTSP_PLAY_RESP,       /* play response */
  SYSMOND_RTSP_PAUSE_REQ,       /* pause request */
  SYSMOND_RTSP_PAUSE_RESP,      /* pause response */
  SYSMOND_RTSP_RESUME_REQ,      /* resume request */
  SYSMOND_RTSP_RESUME_RESP,     /* resume response */
  SYSMOND_RTSP_TEARDOWN_REQ,    /* teardown request */
  SYSMOND_RTSP_TEARDOWN_RESP,   /* teardown response */
  SYSMOND_RTSP_EXIT_REQ         /* sysmonD asks rtsps of the same node
                                    for grace exit */ 
} SysmonDcode;

/* The following are error information that the Flib can send */
/* to the SysmonD Module */
typedef enum {
  SYSMOND_OK,	   /* request processing is successful */
  SYSMOND_FAIL     /* request processing is failed */
} SysmonDerror;

typedef struct SYSMOND_RTSP_DESCRIBE_REQ_INFO {
  UInt64 session_id;
  char movieName[YIMA_MAX_MOVIE_NAME_LEN];
  struct sockaddr_in remoteaddr;           /* remote address */
} SYSMOND_RTSP_DESCRIBE_REQ_INFO;

typedef struct SYSMOND_RTSP_DESCRIBE_RESP_INFO {
  UInt64 session_id;
  NPT_RANGE_T npt_range;
  float duration_in_seconds; //added by samarjit
} SYSMOND_RTSP_DESCRIBE_RESP_INFO;

typedef struct SYSMOND_RTSP_SETUP_REQ_INFO {
  UInt64 session_id;
  UInt16 clientRTPPort;
  UInt16 clientRTCPPort;
} SYSMOND_RTSP_SETUP_REQ_INFO;

typedef struct SYSMOND_RTSP_SETUP_RESP_INFO {
  UInt64 session_id;
  UInt16 serverRTPPort;
  UInt16 serverRTCPPort;
  UInt32 SSRC;
} SYSMOND_RTSP_SETUP_RESP_INFO;


typedef struct SYSMOND_RTSP_PLAY_REQ_INFO {
  UInt64 sessionID;
  NPT_RANGE_T npt_range;
} SYSMOND_RTSP_PLAY_REQ_INFO;

typedef struct SYSMOND_RTSP_PLAY_RESP_INFO {
  UInt64 sessionID;
  NPT_RANGE_T npt_range;//samarjit
  UInt32 nextrtp_seqqno;
  UInt32 nextrtp_timestamp;
} SYSMOND_RTSP_PLAY_RESP_INFO;

typedef struct SYSMOND_RTSP_PAUSE_REQ_INFO {
  UInt64 sessionID;
} SYSMOND_RTSP_PAUSE_REQ_INFO;

typedef struct SYSMOND_RTSP_PAUSE_RESP_INFO {
  UInt64 sessionID;
} SYSMOND_RTSP_PAUSE_RESP_INFO;

typedef struct SYSMOND_RTSP_RESUME_REQ_INFO {
  UInt64 sessionID;
} SYSMOND_RTSP_RESUME_REQ_INFO;

typedef struct SYSMOND_RTSP_RESUME_RESP_INFO {
  UInt64 sessionID;
} SYSMOND_RTSP_RESUME_RESP_INFO;

typedef struct SYSMOND_RTSP_TEARDOWN_REQ_INFO {
  UInt64 sessionID;
  UInt16 crashFlg; /* if set to 1, not Teardown respond to RTSP server */
} SYSMOND_RTSP_TEARDOWN_REQ_INFO;

typedef struct SYSMOND_RTSP_TEARDOWN_RESP_INFO {
  UInt64 sessionID;
} SYSMOND_RTSP_TEARDOWN_RESP_INFO;

typedef struct SYSMOND_RTSP_EXIT_REQ_INFO {
} SYSMOND_RTSP_EXIT_REQ_INFO;

//samarjit
typedef struct SYSMOND_RTSP_OPTIONS_REQ_INFO {
	UInt64 session_id;
}SYSMOND_RTSP_OPTIONS_REQ_INFO;
typedef struct SYSMOND_RTSP_OPTIONS_RESP_INFO {
	UInt64 session_id;
}SYSMOND_RTSP_OPTIONS_RESP_INFO;


typedef struct SysmonDMsg_T {
  SysmonDcode     cmdType;            /* command message type */
  SysmonDerror    cmdError;           /* return value or error */
  union
  {
	 SYSMOND_RTSP_OPTIONS_REQ_INFO   sysmonD_rtsp_options_req_info;
	 SYSMOND_RTSP_OPTIONS_RESP_INFO   sysmonD_rtsp_options_resp_info;
     SYSMOND_RTSP_DESCRIBE_REQ_INFO   sysmonD_rtsp_describe_req_info;
     SYSMOND_RTSP_DESCRIBE_RESP_INFO  sysmonD_rtsp_describe_resp_info;
     SYSMOND_RTSP_SETUP_REQ_INFO      sysmonD_rtsp_setup_req_info;
     SYSMOND_RTSP_SETUP_RESP_INFO     sysmonD_rtsp_setup_resp_info;
     SYSMOND_RTSP_PLAY_REQ_INFO       sysmonD_rtsp_play_req_info;
     SYSMOND_RTSP_PLAY_RESP_INFO      sysmonD_rtsp_play_resp_info;
     SYSMOND_RTSP_PAUSE_REQ_INFO      sysmonD_rtsp_pause_req_info;
     SYSMOND_RTSP_PAUSE_RESP_INFO     sysmonD_rtsp_pause_resp_info;
     SYSMOND_RTSP_RESUME_REQ_INFO     sysmonD_rtsp_resume_req_info;
     SYSMOND_RTSP_RESUME_RESP_INFO    sysmonD_rtsp_resume_resp_info;
     SYSMOND_RTSP_TEARDOWN_REQ_INFO   sysmonD_rtsp_teardown_req_info;
     SYSMOND_RTSP_TEARDOWN_RESP_INFO  sysmonD_rtsp_teardown_resp_info;
     SYSMOND_RTSP_EXIT_REQ_INFO       sysmonD_rtsp_exit_req_info;
  } u;
} SysmonDMsg_T;


/*****************************************************
 * Interface to access SystemMomD Modules  <end>     *
 *****************************************************/


/*==============================================================================*/


/*****************************************************
 * Interface to access FlibModules   <begin>         *
 *****************************************************/


/* The following are commands that a application can send */
/* to the Flib Module */
typedef enum {
  FLIB_GET_NPT_RANGE_REQ,	/* get npt range request  */
  FLIB_GET_NPT_RANGE_RESP,      /* get npt range response */
  FLIB_PLAY_REQ,		/* play request  */
  FLIB_PLAY_RESP,		/* play response */
  FLIB_TEARDOWN_REQ,            /* teardown request */
  FLIB_TEARDOWN_RESP,           /* teardown response */
  FLIB_LAST_PKT_OF_BLOCK        /* inform the Flib module the last packet of Block */
} FLIBcode;

/* The following are error information that the Flib can send */
/* to the SysmonD Module */
typedef enum {
  FLIB_OK,	  /* request processing is successful */
  FLIB_FAIL,      /* request processing is failed */
} FLIBerror;

typedef struct FLIB_GET_NPT_RANGE_REQ_INFO {
  char movieName[YIMA_MAX_MOVIE_NAME_LEN];
} FLIB_GET_NPT_RANGE_REQ_INFO;

typedef struct FLIB_GET_NPT_RANGE_RESP_INFO {
  NPT_RANGE_T npt_range;
  MOV_INFO_T mov_info;
} FLIB_GET_NPT_RANGE_RESP_INFO;

typedef struct FLIB_PLAY_REQ_INFO {
  UInt64 sessionID;
  char movieName[YIMA_MAX_MOVIE_NAME_LEN];
  NPT_RANGE_T npt_range;
} FLIB_PLAY_REQ_INFO;

typedef struct NextRTPPktInfo_T {
   char   *nextRTPPkt_p;        /* the address of the next RTP packet */
   UInt32  nextRTPPktSeqNo;     /* next RTP packet sequence number */
   UInt32  nextRTPPktSize;      /* next RTP packet size */
   UInt32  nextRTPPktTimestamp; /* next RTP packet timestamp*/

   Boolean lastPktFlg;          /* if this packet is the last packet of the block, */
                                /* set this flag to TRUE; otherwise, set it FALSE  */
   char   *blockPtr;            /* corresponding block pointer for this RTP packet */
} NextRTPPktInfo_T;

typedef struct FLIB_PLAY_RESP_INFO {
   NextRTPPktInfo_T nextRTPPktInfo; /* next RTP packet information */
} FLIB_PLAY_RESP_INFO;

typedef struct FLIB_TEARDOWN_REQ_INFO {
   UInt64 sessionID;
   Boolean movEndFlg;
} FLIB_TEARDOWN_REQ_INFO;

typedef struct FLIB_TEARDOWN_RESP_INFO {
} FLIB_TEARDOWN_RESP_INFO;

typedef struct FLIB_LAST_PKT_OF_BLOCK_INFO {
   UInt64 sessionID;  /* !!!???? new */
   char *blockPtr;    /* the address to the block */
} FLIB_LAST_PKT_OF_BLOCK_INFO;

typedef struct MP4FlibMsg_T {
  FLIBcode        cmdType;          /* command message type */
  UInt64          cmdSeqNo;         /* command sequence number */
                                    /* right now, I use sessionID */
  FLIBerror       cmdError;         /* return value or error */
  union {
     FLIB_GET_NPT_RANGE_REQ_INFO   flib_get_npt_range_req_info;
     FLIB_GET_NPT_RANGE_RESP_INFO  flib_get_npt_range_resp_info;
     FLIB_PLAY_REQ_INFO            flib_play_req_info;
     FLIB_PLAY_RESP_INFO           flib_play_resp_info;
     FLIB_TEARDOWN_REQ_INFO        flib_teardown_req_info;
     FLIB_TEARDOWN_RESP_INFO       flib_teardonw_resp_info;
     FLIB_LAST_PKT_OF_BLOCK_INFO   flib_last_pkt_of_block_info;
  } u;
} MP4FlibMsg_T;

/*****************************************************
 * Interface the access FlibModules   <end>          *
 *****************************************************/

/*==============================================================================*/


#endif  /* _YIMA_MODULE_INTERFACES_H_ */

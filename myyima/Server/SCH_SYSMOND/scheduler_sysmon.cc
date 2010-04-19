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

/*#############################*/
/*# Header Files              #*/
/*#############################*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sched.h>
#include "pthread.h"
#include "../sysinclude/OSDebug.h"
#include "OS.h"
#include "qpthr/qp.h"
#include "scheduler_sysmon.h"
#include "../FLIB/interface.h"

// you may disable some debug meesage.
#define SCHEDULER_SYSMON_DEBUG 1
#define SHOW_FLIB_NPT_RANGE_GET_DEBUG 1
#define SHOW_NEXT_PACKET_INFORMATION 1
#define SHOW_CLIENT_ADDR 1
#define SHOW_MIN_INTERVAL 1
#define DEBUG_CALCULATE_MIN_INTERVAL 1
#define DEBUG_SYSMOND_RTSP_PLAY_REQ 1
#define DEEP_DEBUG 1

__USE_QPTHR_NAMESPACE

/* sys parameters */
int MaxAdvancedTime =  MAX_ADVANCED_TIME;  /* 1000 */
int DefaultDeltaP = DEFAULT_DELTAP; /* 372 */
int WaitingPort = SYSMOND_WAITING_PORT;

/* interface provided by Flib module <begin> */
extern "C" int Flib_getNextPacket(UInt64, char**, UInt32*, UInt32*, UInt32*, Boolean*, char**);
extern "C" FLIBerror Flib_get_npt_range(const char *movieName, NPT_RANGE_T *nptRange, MOV_INFO_T *movInfo);
extern "C" Boolean Flib_play(UInt64, const char*, NPT_RANGE_T);
extern "C" int Flib_tear_down(UInt64 sId, Boolean MovieEndFlg);
extern "C" int Flib_lastpktofBlock(UInt64 sId, char *blockPtr);
extern "C" void Flib_init();
/* interface provided by Flib module <end> */


int SCHEDULER_main(void);

/*#########################################*/
/*# Type Definitions                      #*/
/*#########################################*/

/* MsgQueue for SysmonD module, Other modules could send */
/* messages to this queue to communicate with SysmonD */
typedef QpQueue< list< MP4FlibMsg_T > > SysMonD_MsgQueue_T;


/* MsgQueue for MP4Flib module, Other modules could send */
/* messages to this queue to communicate with SysmonD */
typedef QpQueue< list< MP4FlibMsg_T > > MP4Flib_MsgQueue_T;


/* Session List Definition */
typedef list<Session_T> SessionList_T;

/*#####################################*/
/*# Constant Definitions              #*/
/*#####################################*/
#define	SA	           struct sockaddr
#define	LISTENQ	           1024	              /* 2nd argument to listen() */

/*#####################################*/
/*# Global Data Structure Definitions #*/
/*#####################################*/
SessionList_T g_session_List;

/*#####################################*/
/*# Main thread Declarations           #*/
/*#####################################*/

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*  Scheduler thread  <begin> */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
class Scheduler: public QpThread
{
    private:
	SessionList_T *sessionList_p;
        QpMutex *mutex_sessionList_p;         //pointer to the lock for sessionList_p

        QpCond *cond_numOfPlayingSession_p;   //Condition Variable for numOfPlayingSession
        int *numOfPlayingSession_p;
        QpMutex *mutex_numOfPlayingSession_p; //pointer to the lock for numOfPlayingSession


        MP4Flib_MsgQueue_T *mp4Flib_queue;    //sending MsgQueue
        UInt32 *cmdSeqNo_p;                   //pointer to the command sequence number
        QpMutex *mutex_cmdSeqNo_p;            //pointer to the lock for cmdSeqNo

        /* member variables related with RTP Server <begin> */
        int rtp_sockfd;
        /* member variables related with RTP Server <end> */


        void run_scheduler(void);
        void blockIfNoPlayingSession(int* numOfPlayingSession, QpCond* cond_p);
        int  play_session(list<Session_T>::iterator iterator_in);
        SInt64 calculateMinInterval();

        /* member functions related with RTP Server <begin> */
        int SendOutThisRTPpacket(int fSocket, UInt32 inRemoteAddr, UInt16 inRemotePort, char* inBuffer, UInt32 inLength,UInt32 SSRC,UInt64 startPlayTime);
        /* member functions related with RTP Server <end> */

    public:
	Scheduler(SessionList_T *p, QpMutex *sL_Lk_p, QpCond *c_p, int *n_p, QpMutex *m_p, MP4Flib_MsgQueue_T *mq, UInt32 *cmdSNp, QpMutex *muCSN_p):
               sessionList_p(p),
               mutex_sessionList_p(sL_Lk_p),
               cond_numOfPlayingSession_p(c_p),
               numOfPlayingSession_p(n_p),
               mutex_numOfPlayingSession_p(m_p),
               mp4Flib_queue(mq),
               cmdSeqNo_p(cmdSNp),
               mutex_cmdSeqNo_p(muCSN_p)
        {
	    /* init rtpsockfd */
            rtp_sockfd = ::socket(PF_INET, SOCK_DGRAM, 0);
            struct sockaddr_in myrtp_addr;
            myrtp_addr.sin_family = AF_INET;
              myrtp_addr.sin_port = htons(SERVER_RTP_PORT_NO);
              myrtp_addr.sin_addr.s_addr = INADDR_ANY;
              memset(&(myrtp_addr.sin_zero), '\0', 8);
            if (bind(rtp_sockfd,(struct sockaddr *)&myrtp_addr,sizeof(struct sockaddr)) == -1) {
           	     	fprintf(stderr,"(RTSPS/frontend/interface binding RTP port)");
           	         }
        }

	~Scheduler()
        {
	    /* close rtp_sockfd */
            ::close(rtp_sockfd);
            Join();
        }
	virtual void Main() {
	    run_scheduler();
	}

};


/*######################################*/
/*#    Scheduler:                      #*/
/*#    calculateMinInterVal            #*/
/*######################################*/
SInt64 Scheduler::calculateMinInterval()
{
   list<Session_T>::iterator iter_tmp;
   list<Session_T>::iterator iter_end_tmp;
   list<Session_T>::iterator iter_tmp_min;

   SInt64 initSetFlg = 0;
   SInt64 tmpMinSendOutTime = 0;
   SInt64 tmpSendOutTime = 0;

   SInt64 curMinInterval = 0;
   SInt64 currentTime = 0;
   #if DEBUG_CALCULATE_MIN_INTERVAL
   int i = 0;
   #endif

   //loop through each session object in the session object list
   mutex_sessionList_p->Lock(); //<---!!
   iter_tmp = sessionList_p->begin();
   iter_end_tmp = sessionList_p->end();
   mutex_sessionList_p->Unlock(); //<---!!

   #if DEBUG_CALCULATE_MIN_INTERVAL
       printf("\n====<begin> debugging calculate min_interval ====\n");
   #endif

   while (iter_tmp != sessionList_p->end())
   {
	   ASSERT((*numOfPlayingSession_p) >= 0);
	   if ( *numOfPlayingSession_p <= 0 )
		   return(0);

	   if (iter_tmp->currentSessionState == PLAYING)
	   {

		   /* init tmpMinSendOutTime */
		   if (initSetFlg == 0)
		   {
			   /* !!! */
			   tmpMinSendOutTime = iter_tmp->nextRTPPktSentOutTime - iter_tmp->maxAdvancedTime;
			   iter_tmp_min = iter_tmp;
			   initSetFlg = 1;
		   }
		   else
		   {
			   tmpSendOutTime = iter_tmp->nextRTPPktSentOutTime - iter_tmp->maxAdvancedTime;
			   if (tmpSendOutTime < tmpMinSendOutTime)
			   {
				   tmpMinSendOutTime = tmpSendOutTime;
				   iter_tmp_min = iter_tmp;
			   }
		   }

#if DEBUG_CALCULATE_MIN_INTERVAL
		   i++;
		   printf(" <session Counter [%d]>\n", i);
		   printf(" nextRTPPktSendOutTime = %lld \n", iter_tmp->nextRTPPktSentOutTime );
		   printf(" tmpMinSendOutTime  = %lld\n", tmpMinSendOutTime);
#endif
	   }

	   mutex_sessionList_p->Lock(); //<---!!
	   ++iter_tmp;  //next session
	   mutex_sessionList_p->Unlock(); //<---!!
   }

   /* get current time */
   currentTime = OS::Microseconds();

   /* calculate min interval */
   curMinInterval = tmpMinSendOutTime - currentTime;

   #if DEBUG_CALCULATE_MIN_INTERVAL
        printf(" currentTime = %lld , tmpMinSendOutTime= %lld\n", currentTime, tmpMinSendOutTime);
   #endif


   if (curMinInterval < 0)
       curMinInterval = 0;

   #if DEBUG_CALCULATE_MIN_INTERVAL
       printf("====<end> debugging calculate min_interval[%lld] ====\n", curMinInterval);
   #endif
   return(curMinInterval);
}


/*######################################*/
/*#    Scheduler:                      #*/
/*#    main function                   #*/
/*######################################*/
void Scheduler::run_scheduler()
{
   SInt64 minInterval = 0;
   list<Session_T>::iterator iter_tmp;
   list<Session_T>::iterator iter_cur_tmp;
   struct timespec nano_sleepTimeOut;


   pid_t sch_pid;
   int priority_max;
   sched_param sch_param;

   sch_pid=getpid();
   priority_max = sched_get_priority_max(SCHED_RR);
   sched_getparam(sch_pid, &sch_param);
   sch_param.sched_priority = priority_max;
   sched_setparam(sch_pid, &sch_param);
   sched_setscheduler(sch_pid, SCHED_RR, NULL);


   while(1)
   {
        //block if no playing session
        blockIfNoPlayingSession(numOfPlayingSession_p, cond_numOfPlayingSession_p);

        #if SCHEDULER_SYSMON_DEBUG
        printf(" <Call Scheduler::run_scheduler()>\n");
        #endif

        //calculate the minimum interval among all the next RTP packets;
        minInterval = calculateMinInterval();

        #if SHOW_MIN_INTERVAL
            printf("\n~~~show min interval~~\n");
            printf("minInterval=%lld\n", minInterval);
            printf("~~~~~~~~~~~~~~~~~~~~~\n");
        #endif

        if (minInterval > 0)
	{
            nano_sleepTimeOut.tv_sec = minInterval / 1000000;
            nano_sleepTimeOut.tv_nsec = (minInterval % 1000000) * 1000;

            if (minInterval > 200000000)
	           printf("WOW!! waiting: %lld ms\n", minInterval);
	    nanosleep(&nano_sleepTimeOut, NULL);
        }

        //loop through each session object in the session object list
        mutex_sessionList_p->Lock(); //<---
        iter_cur_tmp = sessionList_p->begin();
        mutex_sessionList_p->Unlock(); //<---

        while (iter_cur_tmp != sessionList_p->end())
	{
	    play_session(iter_cur_tmp);

            if (iter_cur_tmp->currentSessionState == TEARDOWN)
            { //delete related session node from session list
              mutex_sessionList_p->Lock(); //<---!!
              iter_tmp = iter_cur_tmp;
              ++iter_cur_tmp;
              sessionList_p->erase(iter_tmp);   /* !!!KunFu debug!! */
              mutex_sessionList_p->Unlock(); //<---!!
            }
            else
	    {
               mutex_sessionList_p->Lock(); //<---!!
               ++iter_cur_tmp;  //next session
               mutex_sessionList_p->Unlock(); //<---!!
            }
	}
   }
}

void Scheduler::blockIfNoPlayingSession(int *numOfPlayingSession, QpCond *cond_p)
{
   if (*numOfPlayingSession <= 0)
   {
       #if DEEP_DEBUG
       printf("\n\n NOW Block <Scheduler>: Number of Playing Session = %d\n\n", *numOfPlayingSession_p);
       #endif
       cond_p->Wait();
   }
}


/*####################################*/
/*#                                  #*/
/*#  Running_entry for each session  #*/
/*#                                  #*/
/*####################################*/
/*#  Within each session object      #*/
/*####################################*/
int Scheduler::play_session(list<Session_T>::iterator iterator_in)
{
    SInt64 currentTime = 0;
    SInt64 playOutTime = 0;
    SInt64 theRelativePacketTime = 0;
    MP4FlibMsg_T tmp_mp4FlibMsg;
    SInt64 prevRTPTimestamp =0 ;
    bool retValBool = FALSE; //<--

    int retVal = 0;


    while(1)
    {
       #if SCHEDULER_SYSMON_DEBUG
       printf(" <Call Scheduler::play_session()>\n");
       #endif


       if (iterator_in->currentSessionState != PLAYING)
       {   //this session is not at PLAYING state, skip it
           return 0;
       }

       if (iterator_in->nextRTPPkt_p == NULL)
       {
           return 1;
       }

       /* get current time */
       currentTime = OS::Microseconds();

       //
       if (iterator_in->previousRTPPktSentOutTime < iterator_in->startPlayTime)
       {   //there is a RESUME command received after sending out the
	   //previous RTP packet, in this case the playOutTime
	   //should be the startPlayTime
           playOutTime = iterator_in->startPlayTime;
       }
       else
       {   //there is NO RESUME command received after sending out the
	   //previous RTP packet, in this case the nextRTPSendOutTime
	   //should be the startPlayTime
 	   playOutTime = iterator_in->nextRTPPktSentOutTime;
       }


       /* calculate the relative time */
       theRelativePacketTime = playOutTime - currentTime;

       if (theRelativePacketTime <=  (iterator_in->maxAdvancedTime))
       {
	   //send out this RTP packet
    	   SendOutThisRTPpacket(rtp_sockfd, iterator_in->clientIPAddr, iterator_in->clientRTPPort, iterator_in->nextRTPPkt_p, iterator_in->nextRTPPktSize,iterator_in->SSRC,iterator_in->startPlayTime);
    	  /*
    	   * iterator_in->nextRTPPkt_p is CORRUPT after this place do not use it
    	   * until Flib_getNextPacket() refreshes it
    	   */

    	   #if SHOW_CLIENT_ADDR
	   printf("\n~~~show client Addr~~\n");
           printf("clientIP=%lu, clientRTPport=%d\n",iterator_in->clientIPAddr, iterator_in->clientRTPPort);
           printf("\n~~~~~~~~~~~~~~~~~~~~~\n");
           #endif


           //check if this is the last RTP packet for a particular block
           //if it is, then inform Flib module by sending a command to Flib's
           //command message queue
           if (iterator_in->lastRTPPktOfBlock_Flg == TRUE)
	       {
              tmp_mp4FlibMsg.cmdType = FLIB_LAST_PKT_OF_BLOCK;

              mutex_cmdSeqNo_p->Lock();
              *cmdSeqNo_p = *cmdSeqNo_p+1;
              tmp_mp4FlibMsg.cmdSeqNo = *cmdSeqNo_p;
              mutex_cmdSeqNo_p->Unlock();

              tmp_mp4FlibMsg.u.flib_last_pkt_of_block_info.sessionID = iterator_in->session_id;
              tmp_mp4FlibMsg.u.flib_last_pkt_of_block_info.blockPtr = iterator_in->blockPtr;
              retValBool = mp4Flib_queue->Send(tmp_mp4FlibMsg);
              ASSERT(retValBool == TRUE);
           }


           //update previous packet related information
           iterator_in->previousRTPPktSeqNo = iterator_in->nextRTPPktSeqNo;
           iterator_in->previousRTPPktSentOutTime = OS::Microseconds();

           #if SCHEDULER_SYSMON_DEBUG
           printf(" <Right before Flib_getNextPacket!!!> SessionId: %llu \n", iterator_in->session_id);
           #endif

           pthread_mutex_lock( &(iterator_in->curSessionState_mutex) );
           if (iterator_in->currentSessionState != PLAYING)
		   {   //this session is not at PLAYING state, skip it
			   pthread_mutex_unlock( &(iterator_in->curSessionState_mutex));
			   return 0;
		   }

           pthread_mutex_lock(&FLIB_access_mutex);
           retVal = Flib_getNextPacket(iterator_in->session_id,
					&(iterator_in->nextRTPPkt_p),
					&(iterator_in->nextRTPPktSeqNo),
					&(iterator_in->nextRTPPktSize),
					&(iterator_in->nextRTPPktTimeStamp),
					&(iterator_in->lastRTPPktOfBlock_Flg),
					&(iterator_in->blockPtr));
           if (retVal == 0)
           {
               #if DEEP_DEBUG
               printf("!! Finish!!sessionID=[%llu] !!\n", iterator_in->session_id);
               #endif

	       /* change the session status to */
               /* changed related session information */
	       iterator_in->currentSessionState = FINISH; /* READY; */
               iterator_in->MovieEndHandlingFlg = TRUE;

#if 0
               // leave as if served.
               mutex_numOfPlayingSession_p->Lock();
               *numOfPlayingSession_p = *numOfPlayingSession_p - 1;
               mutex_numOfPlayingSession_p->Unlock();
#endif

               pthread_mutex_unlock(&FLIB_access_mutex);
               pthread_mutex_unlock( &(iterator_in->curSessionState_mutex));
               return 2; //At this time, there is no RTP packets for this session to be sent out!
           }
	   pthread_mutex_unlock(&FLIB_access_mutex);
	   pthread_mutex_unlock( &(iterator_in->curSessionState_mutex));

           #if SHOW_NEXT_PACKET_INFORMATION
           printf("\n=========================\n");
           printf("next Packet information\n");
           printf("pktSeqNo = %lu \n", iterator_in->nextRTPPktSeqNo);
           printf("pktTimeStamp = %lu \n", iterator_in->nextRTPPktTimeStamp);
           printf("pktSize = %lu \n", iterator_in->nextRTPPktSize);
           printf("lastRTPPktOfBlock_Fl = %d \n",iterator_in->lastRTPPktOfBlock_Flg);
           printf("=========================\n");
           #endif


	   // TODO:
	   // calculate the delivery time of next RTP packet
	   // you need to carefully calculudate the next delivery time
	   // from timestamp.
           // What is the time resolution of the timestamp defined in RTP ?
//          if(prevRTPTimestamp == 0 )prevRTPTimestamp = iterator_in->nextRTPPktTimeStamp;
//
//          if(prevRTPTimestamp < iterator_in->nextRTPPktTimeStamp){
//           iterator_in->nextRTPPktSentOutTime = iterator_in->previousRTPPktSentOutTime+( iterator_in->nextRTPPktTimeStamp - prevRTPTimestamp)/90;// + iterator_in->nextRTPPktTimeStamp;
//
//          }else{
//        	  iterator_in->nextRTPPktSentOutTime = iterator_in->previousRTPPktSentOutTime;
//          }
          iterator_in->nextRTPPktSentOutTime = iterator_in->startPlayTime + ((iterator_in->nextRTPPktTimeStamp *(double)(100.0/9))) ;
          prevRTPTimestamp = iterator_in->nextRTPPktTimeStamp;
          // printf("\nTODO: compute iterator_in->nextRTPPktSendOutTime %llu timestamp %lu starttime: %llu\n", iterator_in->nextRTPPktSentOutTime,iterator_in->nextRTPPktTimeStamp,iterator_in->startPlayTime);



        }
        else
        {
            return 2; //At this time, there is no RTP packets for this session to be sent out!
        }
    }
}


/* RTP server send out the this RTP packet */
int Scheduler::SendOutThisRTPpacket(int fSocket, UInt32 inRemoteAddr, UInt16 inRemotePort, char* inBuffer, UInt32 inLength,UInt32 SSRC,UInt64 startPlayTime)
{
        int retVal = 0;
        struct sockaddr_in 	theRemoteAddr;


	ASSERT(inBuffer != NULL);

        #if SCHEDULER_SYSMON_DEBUG
        printf(" <Call Scheduler::SendOutThisRTPpacket() [fSocket=%d, inRemoteAddr=%d, inRemotePort=%d, inLength=%d]>\n", fSocket, inRemoteAddr, inRemotePort, inLength);
        #endif


	theRemoteAddr.sin_family = PF_INET;
	theRemoteAddr.sin_port = htons((ushort) inRemotePort);
	theRemoteAddr.sin_addr.s_addr = htonl(inRemoteAddr);


	int theErr = -1;

        do
        {
//        	 struct rtp_header *nextRtpPacketPtr = NULL;
//        	           nextRtpPacketPtr = (struct rtp_header *)inBuffer;
//        	           printf("My <SendOutThisRTPpacket>_1 V=%u seqno:%lu timestamp:%lu ",nextRtpPacketPtr->V,nextRtpPacketPtr->SeqNum,nextRtpPacketPtr->TimeStamp);

        	struct rtp_header *	header;
         header = (rtp_header *)inBuffer;
        	unsigned int testblock[3];
        	memset(testblock,0,sizeof(unsigned int)*3);
        					//memcpy(testblock,header,sizeof(unsigned int)*3);
        	 	testblock[2] = 0;
        					 unsigned int tmp;
        					tmp = header->V;
        					tmp = tmp <<30;
        					tmp &= 0xc0000000;
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->V;tmp = tmp <<30;//V
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->P;tmp = tmp <<29;//P
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->X;tmp = tmp <<28;//X
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->CC;tmp = tmp <<24;//CC
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->M;tmp = tmp <<23;//M
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->Pt;tmp = tmp <<16;//Pt
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->SeqNum;tmp = tmp <<0;//Pt
        					testblock[0] = testblock[0] | tmp ;
        					tmp = header->TimeStamp;tmp = tmp <<0;//Pt
        					testblock[1] = testblock[1] | tmp ;
        					tmp = SSRC;//header->SSRC;
        					tmp = tmp <<0;//Pt
        					testblock[2] = testblock[2] | tmp ;

        					testblock[0] = ntohl(testblock[0]);
        					testblock[1] = ntohl(testblock[1]);
        					testblock[2] = ntohl(testblock[2]);
        					memcpy(header,testblock,12	);
           theErr = ::sendto(fSocket, inBuffer, inLength, 0, (sockaddr*)&theRemoteAddr, sizeof(theRemoteAddr));
        }
        while ((theErr == -1) && (errno == EWOULDBLOCK));

	if (theErr != 0)
		retVal = theErr;
	return retVal;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/*  Scheduler thread  <end>   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/





/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* SysMonD_w thread   <begin> */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* SysMonD_w thread */
/* waiting for the request from RTSP Servers */
class SysMonD_w: public QpThread
{
    private:
        MP4Flib_MsgQueue_T *mp4Flib_queue; //sending MsgQueue
	SessionList_T *sessionList_p;
        QpMutex *mutex_sessionList_p;         //pointer to the lock for sessionList_p

        QpCond *cond_numOfPlayingSession_p;   //Condition Variable for numOfPlayingSession
        int *numOfPlayingSession_p;           //pointer to numOfPlayingSession
        QpMutex *mutex_numOfPlayingSession_p; //pointer to the lock for numOfPlayingSession

        UInt32 *cmdSeqNo_p;                   //pointer to command sequence number
        QpMutex *mutex_cmdSeqNo_p;            //pointer to the lock for cmdSeqNo

        /* member functions */
        int run_sysMonD_w();
        int sysMon_recv(int fd);
        Boolean isANewSessionID(UInt64 sessionID_in, SessionList_T *sessionList_p_in);
        list<Session_T>::iterator findSessionNode(UInt64, SessionList_T *);

    public:
	SysMonD_w(MP4Flib_MsgQueue_T *mq, SessionList_T *p, QpMutex *sL_Lk_p, QpCond *c_p, int *n_p, QpMutex *m_p, UInt32 *cmdSNp, QpMutex *muCSN_p):
                    mp4Flib_queue(mq),
                    sessionList_p(p),
                    mutex_sessionList_p(sL_Lk_p),
                    cond_numOfPlayingSession_p(c_p),
                    numOfPlayingSession_p(n_p),
                    mutex_numOfPlayingSession_p(m_p),
                    cmdSeqNo_p(cmdSNp),
                    mutex_cmdSeqNo_p(muCSN_p)
                    {}
	~SysMonD_w() { Join();}
	virtual void Main()
        {
           run_sysMonD_w();
	}
};


/*######################################*/
/*#                                    #*/
/*#    main function                   #*/
/*#    for SysMonD_w thread            #*/
/*######################################*/
int SysMonD_w::run_sysMonD_w()
{

   int	  listenfd;
   struct sockaddr_in	servaddr;
   int  tmp_retVal = 0;

   fd_set permfds, tmpfds;  /* for IO mux testing  */
   int maxfd, fd, newfd, cnt; /* for IO mux testing */

   #if DEEP_DEBUG
   int num_client = 0; /* for debugging */
   #endif
   int i = 0;
   int on;


   /* preparations */
   /* socket preparations */
   listenfd = socket(AF_INET, SOCK_STREAM, 0);
   ASSERT(listenfd >= 0);

   /* filling the address fields */
   bzero(&servaddr, sizeof(servaddr));
   servaddr.sin_family      = AF_INET;
   servaddr.sin_addr.s_addr = INADDR_ANY;
   servaddr.sin_port        = htons(WaitingPort);

   /* socket address reuse */
   on = 1;
   if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
      perror("setsockopt(SO_REUSEADDR) failed");
   }


   /* bind address */
   tmp_retVal = bind(listenfd, (SA *) &servaddr, sizeof(servaddr));
   ASSERT(tmp_retVal >= 0);

   /* LISTEN */
   tmp_retVal = listen(listenfd, LISTENQ);
   ASSERT(tmp_retVal >= 0);

   /* for IO multiplexing <begin>  */
   FD_ZERO(&permfds);
   FD_ZERO(&tmpfds);

   FD_SET(listenfd, &permfds);
   maxfd = listenfd;

   fd = 0;


   /* main loop */
   for (;;)
   {
      tmpfds = permfds;
      /*or memcpy (&tmpfds, &permfds, sizeof (fd_set)); */
      cnt = select(maxfd+1, &tmpfds, NULL, NULL, NULL);

      #if SCHEDULER_SYSMON_DEBUG
       printf(" <Call SysMonD_w::run_sysMonD_w()>\n");
      #endif

      if (cnt > 0)
      {
         for (i = 0; i <= maxfd; i++)
         {
            if (FD_ISSET (fd, &tmpfds))
            {
                if (fd == listenfd)
                {
                    /* it's an incoming new client */
                    newfd = accept(listenfd, NULL, NULL);

                    #if DEEP_DEBUG
		    /* for debugging */
                    num_client ++;
                    printf("SysMonD_w::run_sysMonD_w(): num_client = %d\n", num_client);
                    #endif

                    if (newfd < 0)
                    {
                        perror("select()");
                        ASSERT(0);
                    }
                    else
                    {
                        FD_SET(newfd, &permfds);
                        if (newfd > maxfd)  /* add to master set */
			    maxfd = newfd;   /* keep track of the max */
                    }
                }
                else
                {
                    /* it's a client connection; read() */
                    /* from it, or write() to it */
                    tmp_retVal = sysMon_recv(fd);
                    if (tmp_retVal == -1)
		    {
                        close(fd); 		// bye!

                        FD_CLR(fd, &permfds);	// remove from master set
                    }
                }
            }
            fd = (fd+1) % (maxfd+1);
         }
      }
      else
      {
         perror("select()");
         exit(errno);
      }
   }
   /* for IO multiplexing <end>  */
}

Boolean SysMonD_w::isANewSessionID(UInt64 sessionID_in, SessionList_T *sessionList_p_in)
{
   list<Session_T>::iterator iter_tmp;
   list<Session_T>::iterator iter_end_tmp;
   Boolean retVal = TRUE;

   //loop through each session object in the session object list
   mutex_sessionList_p->Lock();
   iter_tmp = sessionList_p_in->begin();
   iter_end_tmp = sessionList_p_in->end();

   while (iter_tmp != iter_end_tmp)
   {
       if (iter_tmp->session_id == sessionID_in)
       {
           retVal = FALSE;
           break;
       }

       ++iter_tmp;  //next session
       iter_end_tmp = sessionList_p_in->end();
   }

   mutex_sessionList_p->Unlock();
   return(retVal);
}

/* return Value: -1: client stop connection  */
int SysMonD_w::sysMon_recv(int fd)
{
      int numbytes;
      SysmonDMsg_T cmdMsg;
      Boolean newSID_flg = FALSE;
      UInt64 tmpSID = 0;
      Session_T sessionObj;
      MP4FlibMsg_T tmp_mp4FlibMsg;

      UInt16 tmp_clientRTPPort;
      UInt16 tmp_clientRTCPPort;

      NPT_RANGE_T tmp_nptRange;

      list<Session_T>::iterator iter_tmp;
      int tmp_retVal = 0;
      SysmonDMsg_T rspMsg;

      bool retValBool = FALSE; //<--!!
      SessionState_T old_sessionState = INIT;


      //getting SysMonD RTSP request command from RTSP server
      if ((numbytes = recv(fd, &cmdMsg, sizeof(SysmonDMsg_T), 0)) <= 0)
      {
         if (numbytes == 0) 	// connection suddenly closed by client
             perror("(RTSPS/frontend): socket suddenly closed by RTSP server!\n");
         else
	 {
             perror("(SysMonD_w::sysMon_recv)");
             ASSERT(0);
         }

         return -1;
      }
      else
      {
         ASSERT(numbytes == sizeof(SysmonDMsg_T));

         //handling the requests from RTSP server
         switch(cmdMsg.cmdType)
	 {		//samarjit
			 case SYSMOND_RTSP_OPTIONS_REQ:
				  tmpSID = cmdMsg.u.sysmonD_rtsp_describe_req_info.session_id;
				  newSID_flg = isANewSessionID(tmpSID, sessionList_p);
#if DEEP_DEBUG
                  printf("<SysMonD_w::sysMon_recv()> [RTSP_OPTIONS_REQ], {sessionID=%llu} newSID_flg:[%d]\n", tmpSID,newSID_flg );
#endif
                   iter_tmp = findSessionNode(tmpSID, sessionList_p);
                   //ASSERT(iter_tmp != sessionList_p->end());
                   rspMsg.cmdType = SYSMOND_RTSP_OPTIONS_RESP;
				   rspMsg.cmdError = SYSMOND_OK;
				   rspMsg.u.sysmonD_rtsp_options_resp_info.session_id = tmpSID;

				   /* the old rtsps_fd should be equal to fd */
				    //ASSERT(iter_tmp->rtsps_fd == fd);
				   //send result back to related RTSP server
				   tmp_retVal = send(fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
				   ASSERT(tmp_retVal == sizeof(SysmonDMsg_T));
        	 break;
            case SYSMOND_RTSP_DESCRIBE_REQ:	/* describe request */
                 tmpSID = cmdMsg.u.sysmonD_rtsp_describe_req_info.session_id;
                 newSID_flg = isANewSessionID(tmpSID, sessionList_p);
                 if(newSID_flg == TRUE)
		 {
                   #if DEEP_DEBUG
                   printf("<SysMonD_w::sysMon_recv()> [RTSP_DESCRIBE_REQ], {sessionID=%llu}\n", tmpSID );
                   #endif

		   /* create a new session node */
                   sessionObj.session_id = tmpSID;
                   sessionObj.rtsps_fd = fd;
                   strncpy(sessionObj.mov_name, cmdMsg.u.sysmonD_rtsp_describe_req_info.movieName, MAX_MOVIE_NAME_LEN);
                   sessionObj.clientRTPPort = (UInt16)-1;
                   sessionObj.clientRTCPPort = (UInt16)-1;
                   sessionObj.clientIPAddr =(UInt32) ntohl(*((UInt32*)(&(cmdMsg.u.sysmonD_rtsp_describe_req_info.remoteaddr.sin_addr))));
                   sessionObj.dataStatus = INACTIVE; /* may not needed */
                   sessionObj.currentSessionState = INIT;
                   pthread_mutex_init(&sessionObj.curSessionState_mutex, NULL);

		   sessionObj.deltaP = DefaultDeltaP;
                   sessionObj.nextRTPPkt_p = NULL;
                   sessionObj.nextRTPPktSeqNo = (UInt32)-1;
                   sessionObj.nextRTPPktSize = (UInt32)-1;
                   sessionObj.nextRTPPktSentOutTime = -1;
                   sessionObj.lastRTPPktOfBlock_Flg = FALSE;
                   sessionObj.MovieEndHandlingFlg = FALSE;
                   sessionObj.blockPtr = NULL;
                   sessionObj.startPlayTime = -1;
                   sessionObj.previousRTPPktSeqNo = (UInt32)-1;
                   sessionObj.previousRTPPktSentOutTime = -1;
                   int ssrc;
                  		  ssrc = rand();
                   sessionObj.SSRC  = ssrc;
                   /* insert the new session node into session List */
                   mutex_sessionList_p->Lock();
                   sessionList_p->insert(sessionList_p->begin(), sessionObj);
                   mutex_sessionList_p->Unlock();

                   /* send FLIB_GET_NPT_RANGE_REQ to Flib module */
                    tmp_mp4FlibMsg.cmdType = FLIB_GET_NPT_RANGE_REQ;
                    tmp_mp4FlibMsg.cmdSeqNo = tmpSID; /* sessionID is used now */
                    printf("movieee name:%s",sessionObj.mov_name);
                    strncpy(tmp_mp4FlibMsg.u.flib_get_npt_range_req_info.movieName,sessionObj.mov_name, MAX_MOVIE_NAME_LEN);
                   retValBool = mp4Flib_queue->Send(tmp_mp4FlibMsg);
                    ASSERT(retValBool == TRUE);
                 }
                 else
		 { /* this is not a new session */
                    sessionObj.rtsps_fd = fd; /*update rtsps_fd, maybe used by fault-recovery later*/
		 }
                 //samarjit
                 iter_tmp = findSessionNode(tmpSID, sessionList_p);
                                 //  ASSERT(iter_tmp != sessionList_p->end());
                                    rspMsg.cmdType = SYSMOND_RTSP_DESCRIBE_RESP;
                 				   rspMsg.cmdError = SYSMOND_OK;
                 				   rspMsg.u.sysmonD_rtsp_options_resp_info.session_id = tmpSID;

                 				   /* the old rtsps_fd should be equal to fd */
                 				  //   ASSERT(iter_tmp->rtsps_fd == fd);
                 				   //send result back to related RTSP server
                 			//	     tmp_retVal = send(fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
                 			//	      ASSERT(tmp_retVal ==sizeof(SysmonDMsg_T));
                 break;

            case SYSMOND_RTSP_SETUP_REQ:	/* setup request  */
                 tmpSID = cmdMsg.u.sysmonD_rtsp_setup_req_info.session_id;
                 tmp_clientRTPPort = cmdMsg.u.sysmonD_rtsp_setup_req_info.clientRTPPort;
                 tmp_clientRTCPPort = cmdMsg.u.sysmonD_rtsp_setup_req_info.clientRTCPPort;

                 #if DEEP_DEBUG
                    printf("<SysMonD_w::sysMon_recv()> [RTSP_SETUP_REQ], {sessionID=%llu clientRTPPort =%d clientRTCPport=%d}\n", tmpSID, tmp_clientRTPPort, tmp_clientRTCPPort );
		 #endif

                 iter_tmp = findSessionNode(tmpSID, sessionList_p);
                 ASSERT(iter_tmp != sessionList_p->end());
                 iter_tmp->clientRTPPort = tmp_clientRTPPort;
                 iter_tmp->clientRTCPPort = tmp_clientRTCPPort;
                 iter_tmp->currentSessionState = READY;
                 /* the old rtsps_fd should be equal to fd */
                 ASSERT(iter_tmp->rtsps_fd == fd);

                 /* construct result msg to be send to RTSP server */
                 rspMsg.cmdType = SYSMOND_RTSP_SETUP_RESP;
                 rspMsg.cmdError = SYSMOND_OK;
                 rspMsg.u.sysmonD_rtsp_setup_resp_info.session_id = iter_tmp->session_id;
                 rspMsg.u.sysmonD_rtsp_setup_resp_info.serverRTPPort = SERVER_RTP_PORT_NO;
                 rspMsg.u.sysmonD_rtsp_setup_resp_info.serverRTCPPort = SERVER_RTCP_PORT_NO;
                 rspMsg.u.sysmonD_rtsp_setup_resp_info.SSRC = iter_tmp->SSRC;
                 printf("SysMonD_w::sysMon_recv() command type:%d\n",rspMsg.cmdType);
                 //send result back to related RTSP server
                  tmp_retVal = send(iter_tmp->rtsps_fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
                 ASSERT(tmp_retVal == sizeof(SysmonDMsg_T));

                 break;

            case SYSMOND_RTSP_PLAY_REQ:         /* play request */
                 tmpSID = cmdMsg.u.sysmonD_rtsp_play_req_info.sessionID;
                 tmp_nptRange.startLocation = cmdMsg.u.sysmonD_rtsp_play_req_info.npt_range.startLocation;
                 tmp_nptRange.stopLocation = cmdMsg.u.sysmonD_rtsp_play_req_info.npt_range.stopLocation;

                 #if DEEP_DEBUG
                   printf("<SysMonD_w::sysMon_recv()> [RTSP_RTSP_PLAY_REQ], {sessionID=%llu}\n", tmpSID );
                 #endif

                 #if DEBUG_SYSMOND_RTSP_PLAY_REQ
                     printf("\n~~~show SYSMOND_RTSP_PLAY_REQ npt_range  ~~\n");
                     printf("nptRange[%f, %f]\n", tmp_nptRange.startLocation, tmp_nptRange.stopLocation);
                     printf("~~~~~~~~~~~~~~~~~~~~~\n");
				 #endif

                 /* find correspinding session */
                 iter_tmp = findSessionNode(tmpSID, sessionList_p); /* !!! */

                 /* update related field */

		 // TODO:
		 // assign "iter_tmp->startPlayTime" when you start to transmit RTP packets.
		 // note that OS::Microseconds() returns current time expressed as microsecond unit.
		 printf("\nTODO: assign iter_tmp->startPlayTime. NOT CRUCIAL\n");

				 iter_tmp->startPlayTime = OS::Microseconds();
                 iter_tmp->maxAdvancedTime = MaxAdvancedTime;
                 iter_tmp->MovieEndHandlingFlg = FALSE;

                 /* send FLIB_PLAY_REQ to Flib module */
                 /* construct FLIB_PLAY_REQ */
                 tmp_mp4FlibMsg.cmdType = FLIB_PLAY_REQ;
                 tmp_mp4FlibMsg.cmdSeqNo = tmpSID; /*!!I use sessionID for now */
                 tmp_mp4FlibMsg.u.flib_play_req_info.sessionID = tmpSID;
                 tmp_mp4FlibMsg.u.flib_play_req_info.npt_range.startLocation = tmp_nptRange.startLocation;
                 tmp_mp4FlibMsg.u.flib_play_req_info.npt_range.stopLocation = tmp_nptRange.stopLocation;
                 strncpy(tmp_mp4FlibMsg.u.flib_play_req_info.movieName, iter_tmp->mov_name,MAX_MOVIE_NAME_LEN);
                 retValBool = mp4Flib_queue->Send(tmp_mp4FlibMsg);
                 ASSERT(retValBool == TRUE);
                 break;

            case SYSMOND_RTSP_PAUSE_REQ:        /* pause request */
                 tmpSID = cmdMsg.u.sysmonD_rtsp_pause_req_info.sessionID;

                 /* find correspinding session */
                 iter_tmp = findSessionNode(tmpSID, sessionList_p);

                 #if DEEP_DEBUG
                   printf("<SysMonD_w::sysMon_recv()> [RTSP_RTSP_PAUSE_REQ], {sessionID=%llu}\n", tmpSID );
                 #endif

                 /* changed related session information */
                 iter_tmp->currentSessionState = READY;

                 //change numOfPlayingSession accordingly
                 mutex_numOfPlayingSession_p->Lock();
                 *numOfPlayingSession_p = *numOfPlayingSession_p - 1;
                 mutex_numOfPlayingSession_p->Unlock();

                 /* construct result msg to be send to RTSP server */
                 rspMsg.cmdType = SYSMOND_RTSP_PAUSE_RESP;
                 rspMsg.cmdError = SYSMOND_OK;
                 rspMsg.u.sysmonD_rtsp_setup_resp_info.session_id = iter_tmp->session_id;

                 //send result back to related RTSP server
                 tmp_retVal = send(iter_tmp->rtsps_fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
                 ASSERT(tmp_retVal == sizeof(SysmonDMsg_T));

                 break;

            case SYSMOND_RTSP_RESUME_REQ:       /* resume request */
                 tmpSID = cmdMsg.u.sysmonD_rtsp_resume_req_info.sessionID;

                 /* find correspinding session */
                 iter_tmp = findSessionNode(tmpSID, sessionList_p);

                 #if DEEP_DEBUG
                   printf("<SysMonD_w::sysMon_recv()> [RTSP_RTSP_RESUME_REQ], {sessionID=%llu}\n", tmpSID );
                 #endif

                 /* changed related session information */
		 if (iter_tmp->currentSessionState != READY)
		 {
                     printf("session [%llu] currentSessionState = %d\n",tmpSID, iter_tmp->currentSessionState);
                 }

                 old_sessionState = iter_tmp->currentSessionState; //<- Kun
		 ASSERT((iter_tmp->currentSessionState == READY) ||(iter_tmp->currentSessionState == PLAYING));
                 iter_tmp->currentSessionState = PLAYING;
                 iter_tmp->startPlayTime = OS::Microseconds();
                 iter_tmp->maxAdvancedTime = 0;  /* when resume NO maxAdvancedTime needed!!! */

                 //change numOfPlayingSession accordingly
                 if (old_sessionState == READY)
		 {
                 mutex_numOfPlayingSession_p->Lock();
                 *numOfPlayingSession_p = *numOfPlayingSession_p + 1;
                 mutex_numOfPlayingSession_p->Unlock();
                 cond_numOfPlayingSession_p->Signal();
                 }

                 #if SCHEDULER_SYSMON_DEBUG
                 printf("\n\n <SysMonD_w> Number of Playing Session = %d\n\n", *numOfPlayingSession_p);
                 #endif

                 /* construct result msg to be send to RTSP server */
                 rspMsg.cmdType = SYSMOND_RTSP_RESUME_RESP;
                 rspMsg.cmdError = SYSMOND_OK;
                 rspMsg.u.sysmonD_rtsp_resume_resp_info.sessionID = iter_tmp->session_id;

                 //send result back to related RTSP server
                 tmp_retVal = send(iter_tmp->rtsps_fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
                 ASSERT(tmp_retVal == sizeof(SysmonDMsg_T));
                 break;

            case SYSMOND_RTSP_TEARDOWN_REQ:     /* teardown request */
                 tmpSID = cmdMsg.u.sysmonD_rtsp_teardown_req_info.sessionID;

                 /* find correspinding session */
                 iter_tmp = findSessionNode(tmpSID, sessionList_p); /* */

                 /* if iter_tmp == NULL, do NOTHING!! */
                 if ( iter_tmp != sessionList_p->end())
		 {
                    /* set client crash flag */
                    iter_tmp->crashFlg = cmdMsg.u.sysmonD_rtsp_teardown_req_info.crashFlg;

                    #if DEEP_DEBUG
                    printf("<SysMonD_w::sysMon_recv()> [RTSP_RTSP_TEARDOWN_REQ], {sessionID=%llu}\n", tmpSID );
                    #endif

                    /* changed related session information */
	            pthread_mutex_lock(&(iter_tmp->curSessionState_mutex));
                    if (iter_tmp->currentSessionState == PLAYING)
		    {
                       //change numOfPlayingSession accordingly
                       mutex_numOfPlayingSession_p->Lock();
                       *numOfPlayingSession_p = *numOfPlayingSession_p - 1;
                       mutex_numOfPlayingSession_p->Unlock();

                       #if SCHEDULER_SYSMON_DEBUG
                          printf("\n\n <SysMonD_w> Number of Playing Session = %d\n\n", *numOfPlayingSession_p);
                       #endif
                    }
                    iter_tmp->currentSessionState = INIT;
                    pthread_mutex_unlock(&(iter_tmp->curSessionState_mutex));

                    /* send FLIB_TEARDOWN_REQ to Flib module */
                    /* construct FLIB_TEARDOWN_REQ */
                    tmp_mp4FlibMsg.cmdType = FLIB_TEARDOWN_REQ;
                    tmp_mp4FlibMsg.cmdSeqNo = tmpSID; /* !!!I use sessionID for now */
                    tmp_mp4FlibMsg.u.flib_teardown_req_info.sessionID = tmpSID;
                    tmp_mp4FlibMsg.u.flib_teardown_req_info.movEndFlg = iter_tmp->MovieEndHandlingFlg;
                    retValBool = mp4Flib_queue->Send(tmp_mp4FlibMsg);
                    ASSERT(retValBool == TRUE);
                 }

                 break;
	    default:
                 ASSERT(0);
         }
      }

      return(0);
}

/* return NULL: if sessionID not found!! */
list<Session_T>::iterator SysMonD_w::findSessionNode(UInt64 sessionID_in, SessionList_T *sessionList_p_in)
{
   list<Session_T>::iterator iter_tmp;
   list<Session_T>::iterator iter_end_tmp;
   Boolean retVal = FALSE;

   //loop through each session object in the session object list
   mutex_sessionList_p->Lock();
   iter_tmp = sessionList_p_in->begin();
   iter_end_tmp = sessionList_p_in->end();
   //mutex_sessionList_p->Unlock();  //<--!!

   while (iter_tmp != iter_end_tmp)
   {
       if (iter_tmp->session_id == sessionID_in)
       {
           retVal = TRUE;
           break;
       }

       ++iter_tmp;  //next session
       iter_end_tmp = sessionList_p_in->end();

   }

   mutex_sessionList_p->Unlock(); //<--!!

   if (retVal == TRUE)
       return(iter_tmp);
   else
   {   /* SID not found */
       /* ASSERT(0); */
       return(sessionList_p_in->end());
   }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* SysMonD_w thread   <end>   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/




/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* SysMonD_rsp thread <begin> */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* SysMonD_rsp thread */
/* waiting for response Messages from MP4lib module */
class SysMonD_rsp: public QpThread
{
    private:
        SysMonD_MsgQueue_T  *sysMonD_queue;   //receiving MsgQueue
	SessionList_T *sessionList_p;
        QpMutex *mutex_sessionList_p;         //pointer to the lock for sessionList_p

        QpCond *cond_numOfPlayingSession_p;   //Condition Variable for numOfPlayingSession
        int *numOfPlayingSession_p;           //pointer to numOfPlayingSession
        QpMutex *mutex_numOfPlayingSession_p; //Lock for numOfPlayingSession

        /* member functions */
        int run_sysMonD_rsp();
        list<Session_T>::iterator findSessionNode(UInt64, SessionList_T *);

    public:
	SysMonD_rsp(SysMonD_MsgQueue_T  *sq, SessionList_T *p, QpMutex *sL_Lk_p, QpCond *c_p, int *n_p, QpMutex *m_p ):
         sysMonD_queue(sq),
         sessionList_p(p),
         mutex_sessionList_p(sL_Lk_p),
         cond_numOfPlayingSession_p(c_p),
         numOfPlayingSession_p(n_p),
         mutex_numOfPlayingSession_p(m_p) {}
	~SysMonD_rsp() { Join();}

	virtual void Main()
        {
            run_sysMonD_rsp();
	}
};

list<Session_T>::iterator SysMonD_rsp::findSessionNode(UInt64 sessionID_in, SessionList_T *sessionList_p_in)
{
   list<Session_T>::iterator iter_tmp;
   list<Session_T>::iterator iter_end_tmp;
   Boolean retVal = FALSE;

   //loop through each session object in the session object list
   mutex_sessionList_p->Lock(); //<--!!
   iter_tmp = sessionList_p_in->begin();
   iter_end_tmp = sessionList_p_in->end();

   while (iter_tmp != iter_end_tmp)
   {
       if (iter_tmp->session_id == sessionID_in)
       {
           retVal = TRUE;
           break;
       }

       ++iter_tmp;  //next session
       iter_end_tmp = sessionList_p_in->end();
   }

   mutex_sessionList_p->Unlock(); //<--!!

   if (retVal == TRUE)
       return(iter_tmp);
   else
   {   /* SID not found */
       ASSERT(0);
       return(sessionList_p_in->end());
   }
}

/*######################################*/
/*#    main function                   #*/
/*#    for SysMonD_rsp thread          #*/
/*######################################*/
int SysMonD_rsp::run_sysMonD_rsp()
{
   list<Session_T>::iterator iter_tmp;
   MP4FlibMsg_T tmp_flib_resp;
   int tmp_retVal = 0;
   SysmonDMsg_T rspMsg;
   float duration_in_seconds;
   UInt64 movie_size_in_bytes;



   while(1)
   {

        //waiting for command messages from Flib thread
        sysMonD_queue->Receive(&tmp_flib_resp);

        #if SCHEDULER_SYSMON_DEBUG
        printf(" <Call SysMonD_rsp::run_sysMonD_rsp()>\n");
        #endif


        //handling these reponses
        switch(tmp_flib_resp.cmdType)
	{
        case FLIB_GET_NPT_RANGE_RESP:
	     //send RTSP_DESCRIBE response to RTSP server
	     //return the npt_range info back to client
	     //find related session node
	     iter_tmp = findSessionNode(tmp_flib_resp.cmdSeqNo, sessionList_p); /* !!! */

	     //and change session state and send by response
             duration_in_seconds = tmp_flib_resp.u.flib_get_npt_range_resp_info.mov_info.duration_in_seconds;
             movie_size_in_bytes = tmp_flib_resp.u.flib_get_npt_range_resp_info.mov_info.movie_size_in_bytes;
             iter_tmp->deltaP = ((double) 1000000/((movie_size_in_bytes/duration_in_seconds)*1.5/500));
             iter_tmp->defaultDeltaP = ((double) 1000000/((movie_size_in_bytes/duration_in_seconds)*1.5/500)); /* defaultDeltaP */
             iter_tmp->movInfo.timestamp_step_perPkt = tmp_flib_resp.u.flib_get_npt_range_resp_info.mov_info.timestamp_step_perPkt;
             iter_tmp->movInfo.duration_in_seconds = tmp_flib_resp.u.flib_get_npt_range_resp_info.mov_info.duration_in_seconds;
             iter_tmp->movInfo.movie_size_in_bytes = tmp_flib_resp.u.flib_get_npt_range_resp_info.mov_info.movie_size_in_bytes;

             #if  SHOW_FLIB_NPT_RANGE_GET_DEBUG
	     printf("---- SHOW_FLIB_NPT_RANGE_GET_DEBUG -<begin>--\n");
             printf("ITER movie deltaP =[%f]", iter_tmp->deltaP);
             printf("ITER movie Duration=[%f]", iter_tmp->movInfo.duration_in_seconds);
             printf("ITER movie Size=[%llu]", iter_tmp->movInfo.movie_size_in_bytes);
             printf("ITER movie timestamp_step_perPkt=[%d]", iter_tmp->movInfo.timestamp_step_perPkt);

             printf("---- SHOW_FLIB_NPT_RANGE_GET_DEBUG -<end> --\n");
             #endif

             //construct result msg to be send to RTSP server
             rspMsg.cmdType = SYSMOND_RTSP_DESCRIBE_RESP;
             rspMsg.u.sysmonD_rtsp_describe_resp_info.session_id = iter_tmp->session_id;
             rspMsg.cmdError = (SysmonDerror) tmp_flib_resp.cmdError; /* !!!! to do later */
             rspMsg.u.sysmonD_rtsp_describe_resp_info.npt_range.startLocation = tmp_flib_resp.u.flib_get_npt_range_resp_info.npt_range.startLocation;
             rspMsg.u.sysmonD_rtsp_describe_resp_info.npt_range.stopLocation = tmp_flib_resp.u.flib_get_npt_range_resp_info.npt_range.stopLocation;
             //samarjit
             rspMsg.u.sysmonD_rtsp_describe_resp_info.duration_in_seconds = iter_tmp->movInfo.duration_in_seconds;
             //rspMsg.u.sysmonD_rtsp_describe_resp_info.movie_name = tmp_flib_resp.u.flib_get_npt_range_resp_info.mov_info
             //send result back to related RTSP server
             tmp_retVal = send(iter_tmp->rtsps_fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
             ASSERT(tmp_retVal == sizeof(SysmonDMsg_T));
             break;

	case FLIB_PLAY_RESP:
	     //return RTSP_PLAY Reply to RTSP server
             //find related session node
	     iter_tmp = findSessionNode(tmp_flib_resp.cmdSeqNo, sessionList_p); /* !!! */

             //update related session information
             iter_tmp->nextRTPPkt_p = tmp_flib_resp.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPkt_p;
             iter_tmp->nextRTPPktSeqNo = tmp_flib_resp.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPktSeqNo;
             iter_tmp->nextRTPPktSize = tmp_flib_resp.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPktSize;
             iter_tmp->lastRTPPktOfBlock_Flg = tmp_flib_resp.u.flib_play_resp_info.nextRTPPktInfo.lastPktFlg;
             iter_tmp->blockPtr = tmp_flib_resp.u.flib_play_resp_info.nextRTPPktInfo.blockPtr;

             iter_tmp->nextRTPPktSentOutTime = OS::Microseconds();

             //change session state
             iter_tmp->currentSessionState = PLAYING;

             //construct result msg to be send to RTSP server
             rspMsg.cmdType = SYSMOND_RTSP_PLAY_RESP;
             rspMsg.cmdError = (SysmonDerror) tmp_flib_resp.cmdError; /* !!! to do later */
             rspMsg.u.sysmonD_rtsp_play_resp_info.sessionID = iter_tmp->session_id;
             //samarjit
             rspMsg.u.sysmonD_rtsp_play_resp_info.nextrtp_seqqno = tmp_flib_resp.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPktSeqNo;
             rspMsg.u.sysmonD_rtsp_play_resp_info.nextrtp_timestamp =tmp_flib_resp.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPktTimestamp;
             //send result back to related RTSP server
             tmp_retVal = send(iter_tmp->rtsps_fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
             ASSERT(tmp_retVal == sizeof(SysmonDMsg_T));

             //change numOfPlayingSession and activate conditional Var
             mutex_numOfPlayingSession_p->Lock();
             *numOfPlayingSession_p = *numOfPlayingSession_p + 1;
             mutex_numOfPlayingSession_p->Unlock();
             cond_numOfPlayingSession_p->Signal();

             #if SCHEDULER_SYSMON_DEBUG
             printf("\n\n <SysMonD_rsp> Number of Playing Session = %d\n\n", *numOfPlayingSession_p);
             #endif

             break;

	case FLIB_TEARDOWN_RESP:
	     //return RTSP_TEARDOWN response to RTSP server
	     //return RTSP_PLAY Reply to RTSP server

             #if DEEP_DEBUG
             printf("SysMonD_rsp::run_sysMonD_rsp: FLIB_TEARDOWN_RESP: Received!!\n");
             #endif

             //find related session node
	     iter_tmp = findSessionNode(tmp_flib_resp.cmdSeqNo, sessionList_p); /* !!! */
             if (iter_tmp == sessionList_p->end())
	     {
                printf("SysMonD_rsp::run_sysMonD_rsp: FLIB_TEARDOWN_RESP: NULL ptr!\n");
             }
             else
	     {
                 /* if client is NOT crash, send response back to client !!! */
                 if (iter_tmp->crashFlg != 1)
	         {
                    #if DEEP_DEBUG
		    printf("CrashFlg!! send TearDown resp to RTSPS \n");
                    #endif

                    //construct result msg to be send to RTSP server
                    rspMsg.cmdType = SYSMOND_RTSP_TEARDOWN_RESP;
                    rspMsg.cmdError = (SysmonDerror) tmp_flib_resp.cmdError;
                    rspMsg.u.sysmonD_rtsp_teardown_resp_info.sessionID = iter_tmp->session_id;

                    //send result back to related RTSP server
                    tmp_retVal = send(iter_tmp->rtsps_fd, &rspMsg, sizeof(SysmonDMsg_T), 0);
                    ASSERT(tmp_retVal == sizeof(SysmonDMsg_T));
                 }

                 //change session state
                 iter_tmp->currentSessionState = TEARDOWN;
             }

             #if DEEP_DEBUG
             printf("SysMonD_rsp::run_sysMonD_rsp: FLIB_TEARDOWN_RESP: Done!!\n");
             #endif
             break;

        default:
             break;
        }
   }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* SysMonD_rsp thread <end>   */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* MP4Flib thread <start>     */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* MP4Flib thread */
/* waiting for response Messages from MP4lib module */
class MP4Flib: public QpThread
{
    private:
        MP4Flib_MsgQueue_T  *mp4Flib_queue; //receiving MsgQueue
        SysMonD_MsgQueue_T  *sysMonD_queue; //sending MsgQueue

        /* member functions */
        void FLib_main(MP4Flib_MsgQueue_T*,  SysMonD_MsgQueue_T*);

    public:
	MP4Flib(MP4Flib_MsgQueue_T *mq, SysMonD_MsgQueue_T *sq): mp4Flib_queue(mq), sysMonD_queue(sq) {}
	~MP4Flib() { Join();}
	virtual void Main()
        {
	  FLib_main(mp4Flib_queue, sysMonD_queue);
	}
};


void MP4Flib::FLib_main(MP4Flib_MsgQueue_T* recvMsgQ_p_in,  SysMonD_MsgQueue_T* sendMsgQ_p_in)
{
   MP4FlibMsg_T tmp_flib_req;
   MP4FlibMsg_T rspMsg;

   MP4Flib_MsgQueue_T  *interface_receive_queue; //receiving MsgQueue
   SysMonD_MsgQueue_T  *interface_send_queue; //sending MsgQueue


   interface_receive_queue = recvMsgQ_p_in;
   interface_send_queue = sendMsgQ_p_in;


   Boolean retErrorNo = FALSE;
   bool retValBool = FALSE;


   while(1)
   {

      // waiting for command message from Flib Thread
      interface_receive_queue->Receive(&tmp_flib_req);

      #if SCHEDULER_SYSMON_DEBUG
       printf(" <Call  MP4Flib::FLib_main()>\n");
      #endif

      //handling requests
      switch(tmp_flib_req.cmdType)
      {
        case FLIB_GET_NPT_RANGE_REQ:{
             rspMsg.cmdType = FLIB_GET_NPT_RANGE_RESP;
             rspMsg.cmdSeqNo = tmp_flib_req.cmdSeqNo;

             #if SCHEDULER_SYSMON_DEBUG
                 printf(" <Call  MP4Flib::FLib_main() FLIB_GET_NPT_RANGE_REQ>\n");
             #endif

             pthread_mutex_lock(&FLIB_access_mutex);
             Flib_get_npt_range(tmp_flib_req.u.flib_get_npt_range_req_info.movieName, &rspMsg.u.flib_get_npt_range_resp_info.npt_range, &rspMsg.u.flib_get_npt_range_resp_info.mov_info); /* !!!  */
             pthread_mutex_unlock(&FLIB_access_mutex);

             #if  SHOW_FLIB_NPT_RANGE_GET_DEBUG
	     printf("---- SHOW_FLIB_NPT_RANGE_GET_DEBUG -<begin>--\n");
             printf("movie Duration=[%f]", rspMsg.u.flib_get_npt_range_resp_info.mov_info.duration_in_seconds);
             printf("movie Size=[%llu]", rspMsg.u.flib_get_npt_range_resp_info.mov_info.movie_size_in_bytes);
             printf("movie timestamp_step_perPkt=[%d]", rspMsg.u.flib_get_npt_range_resp_info.mov_info.timestamp_step_perPkt);

             printf("---- SHOW_FLIB_NPT_RANGE_GET_DEBUG -<end> --\n");
             #endif

             rspMsg.cmdError = FLIB_OK;

             //send result back to SystemMonitor
             retValBool = interface_send_queue->Send(rspMsg);
             ASSERT(retValBool == TRUE);
             }break;

        case FLIB_PLAY_REQ:{
	     //construct result msg to be send to RTSP server
             rspMsg.cmdType = FLIB_PLAY_RESP;
             rspMsg.cmdSeqNo = tmp_flib_req.cmdSeqNo;

             #if SCHEDULER_SYSMON_DEBUG
                 printf(" <Call  MP4Flib::FLib_main()  FLIB_PLAY_REQ>\n");
             #endif

	     pthread_mutex_lock(&FLIB_access_mutex); //
	     retErrorNo = Flib_play(tmp_flib_req.u.flib_play_req_info.sessionID, tmp_flib_req.u.flib_play_req_info.movieName, tmp_flib_req.u.flib_play_req_info.npt_range);

	     NextRTPPktInfo_T *next_pkt = &(rspMsg.u.flib_play_resp_info.nextRTPPktInfo);
	     (void) Flib_getNextPacket( 	tmp_flib_req.u.flib_play_req_info.sessionID,
					 	&(next_pkt->nextRTPPkt_p),
						&(next_pkt->nextRTPPktSeqNo),
						&(next_pkt->nextRTPPktSize),
						&(next_pkt->nextRTPPktTimestamp),
						&(next_pkt->lastPktFlg),
						&(next_pkt->blockPtr) );

             pthread_mutex_unlock(&FLIB_access_mutex);

             rspMsg.cmdError = FLIB_OK;

             #if SHOW_NEXT_PACKET_INFORMATION
              printf("\n=========================\n");
              printf("RTSP PLAY REQ next Packet information\n");
              printf("pktSeqNo = %lu \n", rspMsg.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPktSeqNo);
              printf("pktTimeStamp = %lu \n", rspMsg.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPktTimestamp);
              printf("pktSize = %lu \n", rspMsg.u.flib_play_resp_info.nextRTPPktInfo.nextRTPPktSize);
              printf("=========================\n");
             #endif

             //send result back to SysMonD
             retValBool = interface_send_queue->Send(rspMsg);
             ASSERT(retValBool == TRUE);
             }break;

        case FLIB_TEARDOWN_REQ:{
	     //construct result msg to be send to RTSP server
             rspMsg.cmdType = FLIB_TEARDOWN_RESP;

             pthread_mutex_lock(&FLIB_access_mutex);
             Flib_tear_down(tmp_flib_req.u.flib_teardown_req_info.sessionID, tmp_flib_req.u.flib_teardown_req_info.movEndFlg);
             pthread_mutex_unlock(&FLIB_access_mutex);

	     rspMsg.cmdSeqNo = tmp_flib_req.cmdSeqNo;
             rspMsg.cmdError = FLIB_OK;

             #if DEEP_DEBUG
             printf(" MP4Flib::FLib_main() FLIB_TEARDOWN_REQ> Done!!\n");
             #endif

             //send result back to SysMonD
             retValBool = interface_send_queue->Send(rspMsg);
             ASSERT(retValBool == TRUE);
	     }break;

	case FLIB_LAST_PKT_OF_BLOCK:{
	  /* NOTE: this command has NO response */
             pthread_mutex_lock(&FLIB_access_mutex);
	     Flib_lastpktofBlock(tmp_flib_req.u.flib_last_pkt_of_block_info.sessionID, tmp_flib_req.u.flib_last_pkt_of_block_info.blockPtr);
             pthread_mutex_unlock(&FLIB_access_mutex);

             }break;

        default:
             break;
      }
  }
}


/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/
/* MP4Flib thread <end>       */
/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/


/*##################*/
/*# Main function  #*/
/*##################*/
int SCHEDULER_main()
{
        QpInit qp_init;    // init qpLibrary

        int i;

        /* SysMonD message queue size */
        int sysMonDMsg_queue_size = 1024;
        /* MP4Flib message queue size */
        int mp4FlibMsg_queue_size = 1024;

        /* number of threads in different class */
        int thr_Scheduler = 1;  //Scheduler thread
        int thr_SysMon_w  = 1;  //SysMonD_w thread
        int thr_SysMon_rsp = 1; //SysMonD_rsp thread
        int thr_MP4Flib = 1;    //MP4Flib thread

        /* for thread sync */
        QpMutex mutex_sessionList;                 //lock for g_session_List

        QpMutex mutex_cond_numOfPlayingSession;    //lock for conditional variable con_numOfPlayingSession;
        QpCond  cond_numOfPlayingSession(mutex_cond_numOfPlayingSession);  //Condition Variable for numOfPlayingSession
        int     numOfPlayingSession = 0;      //numOfPlayingSession
        QpMutex mutex_numOfPlayingSession;    //lock for the numOfPlayingSession
        UInt32  cmdSeqNo = 0;                 //the command sequence number
        QpMutex mutex_cmdSeqNo;               //lock for the cmdSeqNo


        #if DEEP_DEBUG
        printf("RTP packets will be delivered %d (ms) before their timestamp expires\n",MaxAdvancedTime/1000);
	printf("Yima node external socket port number (SYSMOND_WAITING_PORT) = %d\n",WaitingPort);
        #endif

        /* !!! OS initialize */
        OS::Initialize();

        /* flib initialiations */
        Flib_init();

        /* */
        pthread_mutex_init(&FLIB_access_mutex, NULL);

        numOfPlayingSession = 0;
        cmdSeqNo = 0;

	//
        typedef Scheduler *Scheduler_t;
	typedef SysMonD_w *SysMonD_w_t;
        typedef SysMonD_rsp *SysMonD_rsp_t;
	typedef MP4Flib *MP4Flib_t;

        //create message queues
        SysMonD_MsgQueue_T sysMonD_MsgQueue(sysMonDMsg_queue_size);
        MP4Flib_MsgQueue_T mp4Flib_MsgQueue(mp4FlibMsg_queue_size);

        //create threads
        Scheduler_t *scheduler = new Scheduler_t[thr_Scheduler];
        SysMonD_w_t *sysMonD_w = new SysMonD_w_t[thr_SysMon_w];
        SysMonD_rsp_t *sysMonD_rsp = new SysMonD_rsp_t[thr_SysMon_rsp];
        MP4Flib_t *mp4Flib = new MP4Flib_t[thr_MP4Flib];

        //start threads
        for (i = 0; i < thr_Scheduler; i++)
        {
		scheduler[i] = new Scheduler(&g_session_List, &mutex_sessionList, &cond_numOfPlayingSession, &numOfPlayingSession, &mutex_numOfPlayingSession, &mp4Flib_MsgQueue, &cmdSeqNo, &mutex_cmdSeqNo);
		scheduler[i]->Start();
	}

        for (i = 0; i < thr_SysMon_w; i++)
        {
		sysMonD_w[i] = new SysMonD_w(&mp4Flib_MsgQueue, &g_session_List, &mutex_sessionList, &cond_numOfPlayingSession, &numOfPlayingSession, &mutex_numOfPlayingSession, &cmdSeqNo, &mutex_cmdSeqNo);
		sysMonD_w[i]->Start();
	}

	for (i = 0; i < thr_SysMon_rsp; i++)
        {
		sysMonD_rsp[i] = new SysMonD_rsp(&sysMonD_MsgQueue, &g_session_List, &mutex_sessionList, &cond_numOfPlayingSession, &numOfPlayingSession, &mutex_numOfPlayingSession);
		sysMonD_rsp[i]->Start();
	}

	for (i = 0; i <thr_MP4Flib ; i++)
        {
		mp4Flib[i] = new MP4Flib(&mp4Flib_MsgQueue, &sysMonD_MsgQueue);
		mp4Flib[i]->Start();
	}

        //shutdown threads
        for (i = 0; i < thr_Scheduler; i++)
        {
		scheduler[i]->Join();
		delete scheduler[i];
	}
        delete [] scheduler;

        for (i = 0; i < thr_SysMon_w; i++)
        {
		sysMonD_w[i]->Join();
		delete sysMonD_w[i];
	}
        delete [] sysMonD_w;

	for (i = 0; i < thr_SysMon_rsp; i++)
        {
		sysMonD_rsp[i]->Join();
		delete sysMonD_rsp[i];
	}
        delete [] sysMonD_rsp;

	for (i = 0; i <thr_MP4Flib ; i++)
        {
		mp4Flib[i]->Join();
		delete mp4Flib[i];
	}
        delete [] mp4Flib;

        sysMonD_MsgQueue.SendDone();
        mp4Flib_MsgQueue.SendDone();

        return 0;
}

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

#include "rtsps.h"


struct requestinfo {
    UInt64 SID;
    int pending;
    bool valid;
} reqtable[MAXNUMCLIENTS];


int numnodes;
int *nodes;

int link_b;


void InitiateBackend(void);
UInt64 GetSID_Request(SysmonDMsg_T *message);
UInt64 GetSID_Response(SysmonDMsg_T *message);
int FindFreeEntry(void);
int FindRequest(UInt64 SID);
void TerminateBackend(void);

void FatalError(char *where);
void Assert(bool condition,const char *statement);


int RunBackend(void)
{
    fd_set master;                      // master file descriptor list for select()
    fd_set read_fds;                    // temp file descriptor list for select()
    int fdmax,fdmin;                    // minimum and maximum file descriptor number
    char buf[MAXMESSAGESIZE];           // buffer for client data
    char buf2[MAXMESSAGESIZE];           // buffer for client data
    int numbytes;
    SysmonDMsg_T message;
    UInt64 SID;
    int request;
    bool flag;
    int i,j;

    InitiateBackend();

    srandom(SEED);


    FD_ZERO(&master);

    FD_SET(link_b, &master);          

    fdmin = fdmax = link_b;

    for(i = 0;i < numnodes;i++) {
        FD_SET(nodes[i], &master);          
        
        if (nodes[i] < fdmin)  {
	   fdmin = nodes[i];
	}
        if (nodes[i] > fdmax) {
	   fdmax = nodes[i];
	}
    }

                                        // main loop
    for(;;) {
        read_fds = master;              // reset temp fd_set
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            FatalError("(RTSPS/backend/select)");
        }
                                        // run through the existing connections looking for data to read
        for (i = fdmin; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds) == 0)
				continue;

			// message arrived
			if (i == link_b) { // handle a new request
				if ((numbytes = recv(link_b, buf, sizeof(SysmonDMsg_T), 0))
						<= 0) {/* !!!message size problem??? Kun Debug */
					if (numbytes == 0) { // connection suddenly closed by frontend
						fprintf(stderr,
								"(RTSPS/backend): frontend suddenly closed the link!\n");
					}
					FatalError("(RTSPS/backend/link)");
				} else {
					if (numbytes != sizeof(SysmonDMsg_T)) {
						fprintf(
								stderr,
								"message size=%d, msg.cmdType=%d, msg.cmdError=%d",
								numbytes, message.cmdType, message.cmdError);
					}
					Assert(numbytes == sizeof(SysmonDMsg_T),
							"(RTSPS/backend/link): wrong message received from frontend!");
					memcpy(&message, (char *) buf, sizeof(SysmonDMsg_T));

					Assert((request = FindFreeEntry()) != NA,
							"(RTSPS/backend/link): overflow! cannot accept new request");

					flag = false;
					if (message.cmdType == SYSMOND_RTSP_OPTIONS_REQ) {
						printf("runBackEnd():SYSMOND_RTSP_OPTIONS_REQ req:%llu resp:%llu  \n",
								GetSID_Request(&message),message.u.sysmonD_rtsp_options_resp_info.session_id);

						flag = false;
					}
					if (message.cmdType == SYSMOND_RTSP_TEARDOWN_REQ) {
						if (message.u.sysmonD_rtsp_teardown_req_info.crashFlg
								== 1) {
							flag = true;
						}
					}
					if (!flag) {
						reqtable[request].SID = GetSID_Request(&message);
						reqtable[request].pending = numnodes;
						reqtable[request].valid = true;
					}
					printf("session id here SID:[%llu]\n",reqtable[request].SID);
					for (j = 0; j < numnodes; j++) {
						Assert(send(nodes[j], buf, sizeof(SysmonDMsg_T), 0)
								== sizeof(SysmonDMsg_T),
								"(RTSPS/backend/node): cannot send complete message to node");
					}
				}
			} // end if (i==link_b)
			else { // handle data from a node
				if ((numbytes = recv(i, buf, sizeof(SysmonDMsg_T), 0)) <= 0) {
					if (numbytes == 0) { // connection suddenly closed by node
						fprintf(
								stderr,
								"(RTSPS/backend): socket %d suddenly closed by node!\n",
								i);
					} else {
						fprintf(stderr, "(RTSPS/backend): %d received!\n",
								numbytes);
					}
					FatalError("(RTSPS/backend/node)");
				} else { // we got some data from a node
					Assert(numbytes == sizeof(SysmonDMsg_T),
							"(RTSPS/backend/node): wrong message received from a node!");
					memcpy(&message, (char *) buf, sizeof(SysmonDMsg_T));

					if (message.cmdType == SYSMOND_RTSP_EXIT_REQ) {
						Assert(send(link_b, buf, sizeof(SysmonDMsg_T), 0)
								== sizeof(SysmonDMsg_T),
								"(RTSPS/backend/link): cannot send 'exit' command to frontend");
						TerminateBackend();
						return 0;
					}
					printf("rtsp_backend We got reqType=%d\n",message.cmdType);
					Assert((SID = GetSID_Response(&message)) != NA,
							"(RTSPS/backend/node): cannot find session ID in node response");
					//if (message.cmdType == SYSMOND_RTSP_OPTIONS_RESP) {
						printf("SID:[%llu], request:%d, \n",GetSID_Request(&message),request);
					//}
					Assert((request = FindRequest(SID)) != NA,
							"(RTSPS/backend/node): wrong session ID received from node");

					if (reqtable[request].pending > 1) {
						reqtable[request].pending -= 1;
					} else {
						Assert(send(link_b, buf, sizeof(SysmonDMsg_T), 0)
								== sizeof(SysmonDMsg_T),
								"(RTSPS/backend/link): cannot send complete message to frontend");

						reqtable[request].SID = NA;
						reqtable[request].pending = 0;
						reqtable[request].valid = false;
#if DEEP_DEBUG
						printf("Released entry in backend: %d\n", request);
#endif
					}
				}
			} // end else (i==link_b)
		} // end for (i =fdmin
    }  // end for
}    


void InitiateBackend()
{
    FILE *config;
    char IPaddr[20];
    char *ptr;
    unsigned short int portnum;
    struct sockaddr_in peer_addr;

    struct sockaddr_in my_addr,   
                       frontend_addr; 
    int tempfd;
    int sin_size;    

    int i,j;
    unsigned int yes = 1;


    numnodes = 1; 

    nodes = new int[numnodes];

    for (i = 0;i < numnodes;i++) { 
        ptr = (char *)IPaddr;
        strncpy(ptr, "127.0.0.1",20);
        portnum = SYSMOND_WAITING_PORT;

        if ((nodes[i] = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
            FatalError("(RTSPS/backend/interface)");
	}

        peer_addr.sin_family = AF_INET;
        peer_addr.sin_port = htons(portnum);
        peer_addr.sin_addr.s_addr = inet_addr(IPaddr);
        memset(&(peer_addr.sin_zero), '\0', 8);

        if (connect(nodes[i],(struct sockaddr *)&peer_addr, sizeof(struct sockaddr)) == -1) {
            FatalError("(RTSPS/backend/interface)");
	}
    }

    if ((tempfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        FatalError("(RTSPS/backend/interface)");
    }

    printf("RTSP back-end socket port number (LINKPORT) = %d\n", LINKPORT);

    my_addr.sin_family = AF_INET;         
    my_addr.sin_port = htons(LINKPORT);     
    my_addr.sin_addr.s_addr = INADDR_ANY; 
    memset(&(my_addr.sin_zero), '\0', 8);

    if (setsockopt(tempfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
	FatalError("(RTSPS/backend/interface)");
    }

    if (bind(tempfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) {
        FatalError("(RTSPS/backend/interface)");
    }

    if (listen(tempfd,1) == -1) {
        FatalError("(RTSPS/backend/interface)");
    }

    sin_size = sizeof(struct sockaddr_in);
    if ((link_b = accept(tempfd, (sockaddr *)&frontend_addr, (socklen_t *)&sin_size)) == -1) {
        FatalError("(RTSPS/backend/interface)");
    }

    close(tempfd);


    for (i = 0;i < MAXNUMCLIENTS;i++) {
        reqtable[i].SID = NA;
        reqtable[i].pending = 0;
        reqtable[i].valid = false;
    }
}

UInt64 GetSID_Request(SysmonDMsg_T *message)
{
    switch(message->cmdType) {
    case SYSMOND_RTSP_OPTIONS_REQ: return (message->u.sysmonD_rtsp_options_req_info.session_id);
		case SYSMOND_RTSP_DESCRIBE_REQ: return (message->u.sysmonD_rtsp_describe_req_info.session_id);
        case SYSMOND_RTSP_SETUP_REQ: return (message->u.sysmonD_rtsp_setup_req_info.session_id);
        case SYSMOND_RTSP_PLAY_REQ: return (message->u.sysmonD_rtsp_play_req_info.sessionID);
        case SYSMOND_RTSP_RESUME_REQ: return (message->u.sysmonD_rtsp_resume_req_info.sessionID);
        case SYSMOND_RTSP_PAUSE_REQ: return (message->u.sysmonD_rtsp_pause_req_info.sessionID);
        case SYSMOND_RTSP_TEARDOWN_REQ: return (message->u.sysmonD_rtsp_teardown_req_info.sessionID);	
        default: return NA;
    }
}

UInt64 GetSID_Response(SysmonDMsg_T *message)
{
    switch(message->cmdType) {
        case SYSMOND_RTSP_DESCRIBE_RESP: 
          #if DEEP_DEBUG
	  fprintf(stderr,"(RTSPS/backend): get SYSMOND_RTSP_DESCRIBE_RESP!\n"); //<- added by kunfu for debugging
          #endif
             return (message->u.sysmonD_rtsp_describe_resp_info.session_id);
        case SYSMOND_RTSP_OPTIONS_RESP: return (message->u.sysmonD_rtsp_options_resp_info.session_id);
        case SYSMOND_RTSP_SETUP_RESP: return (message->u.sysmonD_rtsp_setup_resp_info.session_id);
        case SYSMOND_RTSP_PLAY_RESP: return (message->u.sysmonD_rtsp_play_resp_info.sessionID);
        case SYSMOND_RTSP_RESUME_RESP: return (message->u.sysmonD_rtsp_resume_resp_info.sessionID);
        case SYSMOND_RTSP_PAUSE_RESP: return (message->u.sysmonD_rtsp_pause_resp_info.sessionID);
        case SYSMOND_RTSP_TEARDOWN_RESP: return (message->u.sysmonD_rtsp_teardown_resp_info.sessionID);	
        default: return NA;
    }
}

int FindFreeEntry(void)
{
    int i;
        
    for (i = 0;i < MAXNUMCLIENTS;i++) {
        if (!(reqtable[i].valid)) {
          return i;
	}
    }

    return NA;
}

int FindRequest(UInt64 SID)
{
    int i;

    for (i = 0;i < MAXNUMCLIENTS;i++) {
        if (reqtable[i].valid) {
            if (reqtable[i].SID == SID) {
		return i;
	    }
	}
    }

    return NA;
}

void TerminateBackend(void)
{
    int i;

    close(link_b);
    for (i = 0;i < numnodes;i++)  {
	close(nodes[i]);
    }
}

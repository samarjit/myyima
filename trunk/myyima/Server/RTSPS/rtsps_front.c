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
#include <stdlib.h>
#include <ctype.h>
#include "rtsps.h"
#include <string.h>
#include <stdio.h>

struct connection {
    char SID[SID_LEN];
    UInt64 intSID;
    int fd;
    int state;
    bool pending;
    bool valid;
    struct sockaddr_in addr;
    char movie_url[YIMA_MAX_MOVIE_NAME_LEN];
    char movie_name[YIMA_MAX_MOVIE_NAME_LEN];

    // TODO:
    // You may freely store intermediate parsed data here.
    int Cseq;
    int clientRtpPort;
    int clientRtcpPort;
} conntable[MAXNUMCLIENTS];

int listener;

int link_f;


void InitiateFrontend(void);
char *GenerateSID(char SID[]); 
UInt64 ConvertSID(char SID[]);
int NextState(int prestate,char *command); 
void ConvertRequest(char *message,SysmonDMsg_T *cnvmessage,int session);
void ConvertResponse(SysmonDMsg_T *message,char *cnvmessage,int session,char *command);
int FindEmptyEntry(void);
int FindEntry(UInt64 SID);
void TerminateFrontend(void);

UInt64 GetSID_Response(SysmonDMsg_T *message);

void FatalError(char *where);
void Assert(bool condition,const char *statement);


int RunFrontend(void) 
{
    fd_set master;   			// master file descriptor list
    fd_set read_fds; 			// temp file descriptor list for select()
    int fdmax,fdmin; 			// minimum and maximum file descriptor number
    struct sockaddr_in client_addr; 	// client address
    int addrlen;
    char buf[MAXMESSAGESIZE];    	// buffer for client data
    int numbytes;
    SysmonDMsg_T message;
    SysmonDMsg_T cnvmessage;
    char *ptr;
    int newfd;        			// newly accept()ed socket descriptor
    int session;
    UInt64 SID;
    int state;
    char command[MAXCOMMANDLEN];
    size_t cmdlen;
    int i,j;


    InitiateFrontend();

    srandom(SEED);    


    FD_ZERO(&master);    		// clear the master and temp sets
    FD_ZERO(&read_fds);

    FD_SET(listener, &master);		// add the listener to the master set
    FD_SET(link_f, &master);		// add the listener to the master set

    if (listener > link_f) {   		// keep track of the biggest file descriptor
        fdmax = listener;
        fdmin = link_f;
    } 
    else {
        fdmax = link_f;
        fdmin = listener;
    }
         				// main loop
    for(;;) {
        read_fds = master; 		// reset temp fd_set
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            FatalError("(RTSPS/frontend/select)");
			}

	// run through the existing connections looking for data to read
	for (i = fdmin; i <= fdmax; i++) {
			if (FD_ISSET(i, &read_fds) == 0)
				continue;

			if (i == listener) { // handle a new connection
				addrlen = sizeof(client_addr);
				if ((newfd = accept(listener, (sockaddr *) &client_addr,
						(socklen_t *) &addrlen)) == -1) {
					FatalError("(RTSPS/frontend/listener)");
				} else {
					FD_SET(newfd, &master); // add to master set
					if (newfd > fdmax) {
						fdmax = newfd; // keep track of the max
					}

					Assert((session = FindEmptyEntry()) != NA,
							"(RTSPS/frontend/listener): overflow! cannot accept new client");
					GenerateSID(conntable[session].SID);
					conntable[session].intSID = ConvertSID(
							conntable[session].SID);
					conntable[session].fd = newfd;
					conntable[session].state = INIT;
					conntable[session].pending = false;
					conntable[session].valid = true;
					conntable[session].addr = client_addr;
				}
			} // end if (i==lister)
			else if (i == link_f) { // response received from backend
				if ((numbytes = recv(link_f, buf, sizeof(SysmonDMsg_T), 0))
						<= 0) {
					if (numbytes == 0) { // connection suddenly closed by backend
						fprintf(stderr,
								"(RTSPS/frontend): backend suddenly closed the link!\n");
					}
					FatalError("(RTSPS/frontend/link)");
				} else {
					if (numbytes != sizeof(SysmonDMsg_T)) {
						fprintf(
								stderr,
								"message size=%d, msg.cmdType=%d, msg.cmdError=%d",
								numbytes, message.cmdType, message.cmdError);
					}
					Assert(numbytes == sizeof(SysmonDMsg_T),
							"(RTSPS/frontend/link): wrong message received from backend");
					memcpy(&message, (char *) buf, sizeof(SysmonDMsg_T));

					if (message.cmdType == SYSMOND_RTSP_EXIT_REQ) {
						TerminateFrontend();
						return 0;
					}

					Assert((SID = GetSID_Response(&message)) != NA,
							"(RTSPS/frontend/link): cannot find session ID in backend response");

					session = FindEntry(SID);
					if (session == NA) {
						printf(
								"(RTSPS/frontend/link): wrong session ID to respond");
						continue;
					}
					Assert(conntable[session].pending,
							"(RTSPS/frontend/link): response for non-pending client!");

					ConvertResponse(&message, buf, session, command);

					numbytes = strlen(buf);
					Assert(send(conntable[session].fd, buf, numbytes, 0)
							== numbytes,
							"(RTSPS/frontend/link): cannot send complete message to client");

					conntable[session].pending = false;

					state = NextState(conntable[session].state, command);
					conntable[session].state = state;
				}
			} // end else if (i == link_f)
			else { // handle data from a client
				if ((numbytes = recv(i, buf, MAXMESSAGESIZE, 0)) <= 0) {
					// error condition
					if (numbytes == 0) { // connection suddenly closed by client
#if DEEP_DEBUG
						fprintf(
								stderr,
								"(RTSPS/frontend): socket %d suddenly closed by client!\n",
								i);
#endif
					} else {
						fprintf(stderr,
								"(RTSPS/frontend/client) numbytes=%d, %s\n",
								numbytes, buf);
					}

					for (j = 0; j < MAXNUMCLIENTS; j++) {
						if (conntable[j].fd == i) {
							break;
						}
					}
					Assert((session = j) < MAXNUMCLIENTS,
							"(RTSPS/frontend/client): non-registered socket descriptor observed. NEW");
					strcpy(buf, "teardown QUIT RTSP/1.0\r\n");

					ConvertRequest(buf, &cnvmessage, session);

					memcpy((char *) buf, (char *) &cnvmessage,
							sizeof(SysmonDMsg_T));

					Assert(send(link_f, buf, sizeof(SysmonDMsg_T), 0)
							== sizeof(SysmonDMsg_T),
							"(RTSPS/frontend/link): cannot send complete message to backend");

					close(i); // bye!

					FD_CLR(i, &master); // remove from master set

					for (j = 0; j < SID_LEN; j++) {
						conntable[session].SID[j] = '\0';
					}
					conntable[session].intSID = NA;
					conntable[session].fd = NA;
					conntable[session].state = TERMINATE;
					conntable[session].pending = false;
					conntable[session].valid = false;
				} else { // we got some data from a client
					char * temp;

					buf[numbytes] = '\0';

					for (j = 0; j < MAXNUMCLIENTS; j++) {
						if (conntable[j].fd == i) {
							break;
						}
					}
					Assert((session = j) < MAXNUMCLIENTS,
							"(RTSPS/frontend/client): non-registered socket descriptor observed. OLD");

					command[0] = '\0';
					cmdlen = strcspn(buf, " ");
					strncat(command, buf, cmdlen);

					char *temp_Cseq;
					char output[100];
					temp_Cseq = strcasestr(buf,"cseq:");
					int cmdlen2 = strcspn(temp_Cseq, "\r\n");

					 strncpy(output,temp_Cseq+5,cmdlen2-5);
					 output[cmdlen2-5]='\0';
					 int itmpCseq = atoi(output);
					conntable[session].Cseq = itmpCseq;
					printf("************trial Cseq:%i\n",itmpCseq);
					state = NextState(conntable[session].state, command);
#if DEEP_DEBUG
					printf("Command received: [%s], command:[%s], state:[%d], conntable[%d].State:[%d]\n", buf,
							command,state,session,conntable[session].state); /* added by Kun Fu */
#endif
					fflush(stdout);
					Assert((state
							= NextState(conntable[session].state, command))
							!= NA,
							"(RTSPS/frontend/client): request does not match the valid request pattern");

					ConvertRequest(buf, &cnvmessage, session);

					memcpy((char *) buf, (char *) &cnvmessage,
							sizeof(SysmonDMsg_T));

					Assert(send(link_f, buf, sizeof(SysmonDMsg_T), 0)
							== sizeof(SysmonDMsg_T),
							"(RTSPS/frontend/link): cannot send complete message to backend");

#if DEEP_DEBUG
					fprintf(stdout, "send command to SYSmonD server.\n"); //<- added by kunfu for debugging
#endif

					conntable[session].pending = true;
				}
			} // end else
		} // end for inner
    } // end for outer
}    

void InitiateFrontend(void)
{
    struct sockaddr_in backend_addr;
    struct sockaddr_in my_addr;
    int i,j;
    int on;

    sleep(WAIT4LINK);

    if ((link_f = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        FatalError("(RTSPS/frontend/interface)");
    }

    backend_addr.sin_family = AF_INET; 
    backend_addr.sin_port = htons(LINKPORT);
    backend_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(backend_addr.sin_zero), '\0', 8); 

    if (connect(link_f,(struct sockaddr *)&backend_addr, sizeof(struct sockaddr)) == -1) {
        FatalError("(RTSPS/frontend/interface)");
    }

    if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        FatalError("(RTSPS/frontend/interface)");
    }

    printf("RTSP front-end socket port number (RTSPSPORT) = %d\n", RTSPSPORT);
    printf("Server-side RTP/RTCP socket port pair = %d, %d\n", SERVER_RTP_PORT_NO, SERVER_RTCP_PORT_NO);
    
    my_addr.sin_family = AF_INET; 
    my_addr.sin_port = htons(RTSPSPORT);
    my_addr.sin_addr.s_addr = INADDR_ANY;
    memset(&(my_addr.sin_zero), '\0', 8); 

    on = 1;
    if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) < 0) {
        FatalError("(RTSPS/frontend/interface)");
    }
                       
    if (bind(listener,(struct sockaddr *)&my_addr,sizeof(struct sockaddr)) == -1) {       
	FatalError("(RTSPS/frontend/interface)");
    }

    if (listen(listener,BACKLOG) == -1) {
        FatalError("(RTSPS/frontend/interface)");
    }

   
    for (i = 0;i < MAXNUMCLIENTS;i++) {
        for (j = 0;j < SID_LEN;j++) {
            conntable[i].SID[j] = NULL;
	}
 	conntable[i].intSID = NA;
 	conntable[i].fd = NA;
        conntable[i].state = TERMINATE;
	conntable[i].pending = false;
	conntable[i].valid = false;
    }
}

char *GenerateSID(char SID[]) 
{
    int i;

    /* added by KunFu <begin> */
    struct timeval t;
    struct timezone tz;
    int theErr = gettimeofday(&t, &tz);
    SInt64 curTime;

    curTime = t.tv_sec;
    curTime *= 1000;		// sec -> msec
    curTime += t.tv_usec / 1000;// usec -> msec

    for (i = 0;i < SID_LEN;i++) 
    {
      SID[i] = ((random()+curTime) % 10) + '0';
    }

    return SID;
}

UInt64 ConvertSID(char SID[])
{
    UInt64 intSID;
    UInt64 temp;
    int i;

    intSID = 0;
    temp = 1; 
    for(i = 0; i < SID_LEN; i++){
        intSID += temp * (UInt64)(SID[SID_LEN-1-i] - '0');        
        temp = temp * 10;
    }

    return intSID;
}

int NextState(int prestate,char *command) 
{
    
    switch (prestate) {
        case INIT: 	if (!strcmp(command,"OPTIONS")) 	return INIT;
					else if (!strcmp(command,"DESCRIBE")) 	return START;
                    else  				return NA;

                    break;
        case START: if (!strcmp(command,"SETUP")) 	return READY;
		    else		 		return NA;

                    break;
        case READY: if (!strcmp(command,"PLAY"))	return PLAY;
		    else if (!strcmp(command,"SETUP"))  return READY;
		    else 				return NA;

                    break;
        case PLAY:  if (!strcmp(command,"PLAY"))   // covers "resume"
                         return PLAY;
                    else if (!strcmp(command,"PAUSE"))    return PAUSE;
                    else if (!strcmp(command,"TEARDOWN")) return TERMINATE;
		    else 				  return NA;

                    break;
        case PAUSE: if (!strcmp(command,"PLAY")) 	  return PLAY; 
                    else if (!strcmp(command,"TEARDOWN")) return TERMINATE;
                    else				  return NA;

                    break;
        default:    
                    fprintf(stderr,"(RTSPS/frontend): preState=%d cmd=%s\n",prestate, command);
    }
}


void ConvertRequest(char *message,SysmonDMsg_T *cnvmessage,int session)
{
    char command[MAXCOMMANDLEN];
    char buf[MAXFIELDLEN];
    char *ptr;
    char *temp;
    int i;
    int track_id;
    char portinfo[2048];

    if (!message || !cnvmessage) return;


    // TODO:
    // You may need to store some important intermediate data that may be
    // used in ConvertResponse().
    strcpy(portinfo,message); //samarjit

    printf("\nTODO: store some additional information in conntable\n");

    command[0] = '\0';
    strcpy(command,strtok(message," "));
   
    if (!strcmp(command,"OPTIONS")) {
    	fprintf(stdout,"<YimaJade 1.1> Options received now.\n");
    	cnvmessage->cmdType = SYSMOND_RTSP_OPTIONS_REQ;
    	cnvmessage->cmdError = SYSMOND_OK;
    	cnvmessage->u.sysmonD_rtsp_options_req_info.session_id = ConvertSID(conntable[session].SID);
    	// TODO
    	 printf("\nTODO: incomplete OPTIONS at ConvertRequest()\n");
    }else if (!strcmp(command,"DESCRIBE")) {
        fprintf(stdout,"<YimaJade 1.1> Describe received now.\n");
        cnvmessage->cmdType = SYSMOND_RTSP_DESCRIBE_REQ;
        cnvmessage->cmdError = SYSMOND_OK;

        cnvmessage->u.sysmonD_rtsp_describe_req_info.session_id = ConvertSID(conntable[session].SID);

        strcpy(buf,strtok(NULL," "));
        Assert(strlen(buf) <= YIMA_MAX_MOVIE_NAME_LEN,
               "(RTSPS/frontend/client): long movie name!");
	strcpy(conntable[session].movie_url, buf); /* Be careful */

        temp = strrchr(buf,'/');
        strcpy(cnvmessage->u.sysmonD_rtsp_describe_req_info.movieName,temp+1);       

        strcpy(conntable[session].movie_name,cnvmessage->u.sysmonD_rtsp_describe_req_info.movieName);

        cnvmessage->u.sysmonD_rtsp_describe_req_info.remoteaddr = conntable[session].addr;
       
	char *p = strtok(NULL, " "); 

        #if DEEP_DEBUG
        fprintf(stdout,"ConvertRequest():Describe received now.\n"); //<- added by kunfu for debugging
        #endif

    } else if (!strcmp(command,"SETUP")) {

        fprintf(stdout,"<YimaJade 1.1> setup received now.\n");
        cnvmessage->cmdType = SYSMOND_RTSP_SETUP_REQ;
        cnvmessage->cmdError = SYSMOND_OK;

        cnvmessage->u.sysmonD_rtsp_setup_req_info.session_id = ConvertSID(conntable[session].SID);

	// TODO:
	// configure cnvmessage->u.sysmonD_rtsp_setup_req_info.clientRTPPort and clientRTCPPort correctly from input string.
        char *temp_rtpport;
		char *temp_rtcpport;
		char output[100];
		temp_rtpport = strcasestr(portinfo, "client_port=");
		int cmdlen2 = strcspn(temp_rtpport, "\r\n");

		strncpy(output, temp_rtpport + 12, cmdlen2 - 12);
		output[cmdlen2 - 12] = '\0';

		temp_rtpport = strtok(output, "-");
		temp_rtcpport = strtok(NULL, "-");
        			printf("Client ports rtp and rtcp :%s %s ",message,temp_rtpport);
        cnvmessage->u.sysmonD_rtsp_setup_req_info.clientRTPPort = atoi(temp_rtpport);
        conntable[session].clientRtpPort = atoi(temp_rtpport);
        cnvmessage->u.sysmonD_rtsp_setup_req_info.clientRTCPPort= atoi(temp_rtcpport);
        conntable[session].clientRtcpPort = atoi(temp_rtcpport);
        printf("\nTODO: incomplete at ConvertRequest() SETUP\n");

    } else if (!strcmp(command,"PLAY")) {
        cnvmessage->cmdError = SYSMOND_OK;

	/* retrieve NPT information from input parameters. */

	// TODO:
	// read npt values from input string
	// if there is no NPT value,
	// 	create "SYSMOND_RTSP_RESUME_REQ" cmd and fill out session ID field.
 	// if existing,
	//	create "SYSMOND_RTSP_PLAY_REQ" and fill out session ID, NPT fields.	
	printf("\nTODO: incomplete at ConvertRequest()\n");

    } else if (!strcmp(command,"PAUSE")) {

      fprintf(stdout,"<YimaJade 1.1> PAUSE received now.\n");

        cnvmessage->cmdType = SYSMOND_RTSP_PAUSE_REQ;
        cnvmessage->cmdError = SYSMOND_OK;

        cnvmessage->u.sysmonD_rtsp_pause_req_info.sessionID = ConvertSID(conntable[session].SID);

    } else if (!strcmp(command,"TEARDOWN")) {
        fprintf(stdout,"<YimaJade 1.1> teardown received now.\n");

        cnvmessage->cmdType = SYSMOND_RTSP_TEARDOWN_REQ;
        cnvmessage->cmdError = SYSMOND_OK;

        cnvmessage->u.sysmonD_rtsp_teardown_req_info.sessionID = ConvertSID(conntable[session].SID);

        strcpy(buf,strtok(NULL," "));
        if (!strcmp(buf,"QUIT"))
          cnvmessage->u.sysmonD_rtsp_teardown_req_info.crashFlg = 1;
        else        
          cnvmessage->u.sysmonD_rtsp_teardown_req_info.crashFlg = 0;
    }
    else {
	printf("(RTSPS/frontend/client): unknown command[%s] in client request!",command);
    }
}



void ConvertResponse(SysmonDMsg_T *message,char *cnvmessage,int session,char *command)
{
    char buf[MAXMESSAGESIZE];

    if (!message || !cnvmessage) return;

    *cnvmessage = '\0';
    printf("ConvertResponse command type:%d\n",message->cmdType);
    switch (message->cmdType) { 
		case SYSMOND_RTSP_OPTIONS_RESP:
                strcpy(command,"OPTIONS");

                if (message->cmdError == SYSMOND_OK) {
                    strcat(cnvmessage,"RTSP/1.0 200 OK\r\nServer: YimaPE\r\nCseq: ");
                    char stmpCseq[10];
                    printf("Cseq===%d",conntable[session].Cseq);
                    sprintf(stmpCseq,"%d",conntable[session].Cseq);
                    strcat(cnvmessage,stmpCseq);
                    strcat(cnvmessage,"\r\nPublic: DESCRIBE, SETUP, TEARDOWN, PLAY\r\n\r\n");
    	    }
                else {
                    strcat(cnvmessage,"RTSP/1.0 FAIL\r\n");
    	    }

    	    // TODO:
    	    // you need to append remaining characters to "cnvmessage".
    	    printf("\nTODO: incomplete OPTIONS RESPONSE at ConvertResponse()..done\n");

                break;
        case SYSMOND_RTSP_DESCRIBE_RESP:
            strcpy(command,"DESCRIBE");

            if (message->cmdError == SYSMOND_OK) {
                strcat(cnvmessage,"RTSP/1.0 200 OK\r\n");
			}
				else {
					strcat(cnvmessage,"RTSP/1.0 FAIL\r\n");
			}
            printf("hello something happening here");
	    // TODO:
	    // you need to append remaining characters to "cnvmessage".
            char sdpMsg[1000];
            sdpMsg[0] ='\0' ;
            char tmpsdpMsg[1000];

            tmpsdpMsg[0] = '\0';
            strcat(cnvmessage,"Cseq: ");
            sprintf(tmpsdpMsg,"%d\r\n",conntable[session].Cseq);
            strcat(cnvmessage,tmpsdpMsg);

            strcpy(sdpMsg,"v=0\r\n");
            struct timeval t;
               struct timezone tz;
               int theErr ;
               theErr = gettimeofday(&t, &tz);

               sprintf(tmpsdpMsg,"o=YimaServer %llu %d IN IP4 %s\r\n",message->u.sysmonD_rtsp_describe_resp_info.session_id,t.tv_sec
						,inet_ntoa(conntable[session].addr.sin_addr));
            strcat(sdpMsg,tmpsdpMsg);
            sprintf(tmpsdpMsg,"s=/%s\r\n",conntable[session].movie_name);
            strcat(sdpMsg,tmpsdpMsg);
            strcat(sdpMsg,"u=http:///\r\n");
            strcat(sdpMsg,"e=admin@YimaServer\r\n");
            strcat(sdpMsg,"c=IN IP4 0.0.0.0\r\n");
            char sdpfile[200];
            strncpy(sdpfile,conntable[session].movie_name,strlen(conntable[session].movie_name)-4);
            sdpfile[strlen(conntable[session].movie_name)-4]='\0';
            strcat(sdpfile,"_sdp.txt");
            printf("\nSDP file _sdp.txt is: %s",sdpfile);

            FILE *fp;
            fp = fopen(sdpfile,"r+");
            size_t len;
            len=0;
            ssize_t read;
            char *sline;
            sline = NULL;
            if((read  =  getline(&sline,&len,fp))!=-1){
             	sline[strlen(sline)-1]=13;
             	strcat(sline,"\n");
            //	sline[strlen(sline)-1]=0;
            	strcat(sdpMsg,sline);
            }
            sprintf( tmpsdpMsg,"t=%f %f\r\n", message->u.sysmonD_rtsp_describe_resp_info.npt_range.startLocation,
            		message->u.sysmonD_rtsp_describe_resp_info.npt_range.stopLocation);
            strcat(sdpMsg,tmpsdpMsg);

            strcat(sdpMsg,"a=controls:*\r\n");
            if((read  =  getline(&sline,&len,fp))!=-1){
             	sline[strlen(sline)-1]=13;
             	strcat(sline,"\n");
                       	strcat(sdpMsg,sline);
                       }
            sprintf( tmpsdpMsg,"a=range:npt=0- %.5f\r\n",(float) message->u.sysmonD_rtsp_describe_resp_info.duration_in_seconds);
            strcat(sdpMsg,tmpsdpMsg);
             read  =  getline(&sline,&len,fp);//skip one eol
            while((read  =  getline(&sline,&len,fp))!=-1){
             	sline[strlen(sline)-1]=13;
             	strcat(sline,"\n");
                                   	strcat(sdpMsg,sline);
                                   }
            fclose(fp);


			int sdp_len;
			sdp_len = strlen(sdpMsg);
			sprintf(tmpsdpMsg,"Content-length: %d\r\n\r\n",sdp_len);
			 strcat(cnvmessage,tmpsdpMsg);
			 strcat(cnvmessage,sdpMsg);
			 strcat(cnvmessage,"\r\n");
			printf("\nTODO: incomplete DESCRIBE RESPONSE at ConvertResponse() %s\n",sdpMsg);

            break;  

        case SYSMOND_RTSP_SETUP_RESP:
        	printf("SYSMOND_RTSP_SETUP_RESP block in switch\n");
            strcpy(command,"SETUP");
            memset(cnvmessage,'\0',2048);
            strcat(cnvmessage,"RTSP/1.0 ");
            if (message->cmdError == SYSMOND_OK) {
                strcat(cnvmessage," 200 OK\r\n");
	    }
            else  {
                strcat(cnvmessage,"FAIL\r\n");
	    }
	    
	    // TODO:
	    // you need to append remaining characters to "cnvmessage".

		  // char tmpsdpMsg[1000];

		  tmpsdpMsg[0] = '\0';
		  strcat(cnvmessage,"Cseq: ");
		  sprintf(tmpsdpMsg,"%d\r\n",conntable[session].Cseq);
		  strcat(cnvmessage,tmpsdpMsg);

		  sprintf(tmpsdpMsg,"Session: %s\r\nTransport: RTP/AVP;",conntable[session].SID);
		  strcat(cnvmessage,tmpsdpMsg);
		  sprintf(tmpsdpMsg,"unicast;source=%s;",inet_ntoa(conntable[session].addr.sin_addr));
		  strcat(cnvmessage,tmpsdpMsg);//ntohs(conntable[session].addr.sin_port)
		  sprintf(tmpsdpMsg,"client_port=%d-%d;", conntable[session].clientRtpPort, conntable[session].clientRtcpPort);
		  strcat(cnvmessage,tmpsdpMsg);
		  sprintf(tmpsdpMsg,"server_port=%d-%d;",message->u.sysmonD_rtsp_setup_resp_info.serverRTPPort,message->u.sysmonD_rtsp_setup_resp_info.serverRTCPPort);
		  strcat(cnvmessage,tmpsdpMsg);
		  sprintf(tmpsdpMsg,"ssrc=%s\r\n\r\n","1234");
		  strcat(cnvmessage,tmpsdpMsg);


	    printf("\nTODO: incomplete SETUP RESPONSE at ConvertResponse()\n");
             
            break;  
        case SYSMOND_RTSP_PLAY_RESP:
            strcpy(command,"PLAY");           
             
            strcat(cnvmessage,"RTSP/1.0 ");
            if (message->cmdError == SYSMOND_OK) {
                strcat(cnvmessage," 200 OK\r\n");
	    }
            else { 
                strcat(cnvmessage,"FAIL\r\n");
            }

	    // TODO:
	    // you need to append remaining characters to "cnvmessage".
	    printf("\nTODO: incomplete PLAY RESPONSE at ConvertResponse()\n");
             
            break;  

        case SYSMOND_RTSP_PAUSE_RESP:
            strcpy(command,"PAUSE");           
             
            strcat(cnvmessage,"RTSP/1.0 ");
            if (message->cmdError == SYSMOND_OK) {
                strcat(cnvmessage," 200 OK\r\n");
	    }
            else  {
                strcat(cnvmessage,"FAIL\r\n");
	    }

	    // TODO:
	    // you need to append remaining characters to "cnvmessage".
	    printf("\nTODO: incomplete PAUSE RESPONSE at ConvertResponse()\n");

            break;  

        case SYSMOND_RTSP_RESUME_RESP:
            strcpy(command,"PLAY");           
             
            strcat(cnvmessage,"RTSP/1.0 ");
            if (message->cmdError == SYSMOND_OK) {
                strcat(cnvmessage,"200 OK\r\n");
	    }
            else {
                strcat(cnvmessage,"FAIL\r\n");
	    }

	    // TODO:
	    // you need to append remaining characters to "cnvmessage".
	    printf("\nTODO: incomplete RESUME RESPONSE at ConvertResponse()\n");

            break;  

        case SYSMOND_RTSP_TEARDOWN_RESP:
            strcpy(command,"TEARDOWN");           
             
            strcat(cnvmessage,"RTSP/1.0 ");
            if (message->cmdError == SYSMOND_OK) {
                strcat(cnvmessage,"200 OK\r\n");
	    }
            else { 
                strcat(cnvmessage,"FAIL\r\n");
	    }

	    // TODO:
	    // you need to append remaining characters to "cnvmessage".
	    printf("\nTODO: incomplete RESUME RESPONSE at ConvertResponse()\n");

            break; 	 
    }
}

int FindEmptyEntry(void) 
{
    int i;   

    for (i = 0;i < MAXNUMCLIENTS;i++) {
        if (!(conntable[i].valid)) {	  
          return i;
	}
    }

    return NA;
}

int FindEntry(UInt64 SID)
{
    int i;
 
    for (i = 0;i < MAXNUMCLIENTS;i++) {
        if (conntable[i].valid) {
            if (conntable[i].intSID == SID) {
		return i;
	    }
	}
    }
    
    return NA;
}

void TerminateFrontend(void)
{
    int i;

    close(listener);
    close(link_f);
    for (i = 0;i < MAXNUMCLIENTS;i++) {
        if (conntable[i].valid) {
	   close(conntable[i].fd);
	}
    }
}

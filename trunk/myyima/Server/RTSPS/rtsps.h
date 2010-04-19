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

#ifndef RTSPS_H
#define RTSPS_H

					// standard libraries
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
					// socket related libraries
#include <unistd.h>
#include <sys/time.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "../sysinclude/Yima_module_interfaces.h"

#define WAIT4LINK	2		// amount of time (in seconds) frontend
  					//  waits for backend initialization
					//  before asking for link


#define MAXNUMCLIENTS   100		// max number of clients;
					//  hard admission control!

#define BACKLOG MAXNUMCLIENTS 		// how many pending connection requests
 					//  queue will hold
#define SID_LEN	19			// length of session ID in characters
#define SEED	8734			// seed for random generator

#define MAXMESSAGESIZE	2048
#define MAXFIELDLEN	40
#define MAXCOMMANDLEN	10

					// connection states in RTSPS
#define INIT	10
#define START	20
#define READY	30
#define PLAY	40
#define PAUSE	50
#define TERMINATE	100
#define HTTPOK  200
#define REQ501 501
#define SET_PARAM 41

#define NA	-5
#define DEEP_DEBUG 1

#ifdef __cplusplus
}
#endif

#endif /* RTSPS_H */

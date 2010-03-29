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

extern int RunBackend(void);
extern int RunFrontend(void);

void FatalError(char *where);
void Assert(bool condition,const char *statement);
int RTSPS_main(void);


int RTSPS_main()
{
    pid_t status;

    if (!(status = fork())) {
        RunFrontend();     

        exit(0);
    } else {
        if (status == -1) FatalError("(RTSPS/main/fork)");	
        RunBackend();
    }  

    wait(NULL); 

    return(0);
}

void FatalError(char *where) 
{
    perror(where);
    fprintf(stderr,"Abort.\n");
    fflush(stderr);
    exit(1);
}

void Assert(bool condition,const char *statement) 
{
  if(!(condition)) {  
    fprintf(stderr,"Assertion failed in %s\n",statement);
    fprintf(stderr,"Abort.\n");
    fflush(stderr);
    exit(1);
  }
}

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
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>

#include "OS.h"
#include "../sysinclude/OSDebug.h"

SInt64  OS::sMsecSince1900 = 0;
SInt64  OS::sInitialMsec = 0;

void OS::Initialize()
{
	struct timeval t;
	struct timezone tz;
	int theErr = ::gettimeofday(&t, &tz);
	ASSERT(theErr == 0);

	//t.tv_sec is number of seconds since Jan 1, 1970. Convert to seconds since 1900
	sMsecSince1900 = 24 * 60 * 60;
	sMsecSince1900 *= (70 * 365) + 17;
	sMsecSince1900 += (SInt64)t.tv_sec;
	sMsecSince1900 *= 1000;//convert to msec from sec
	
	sInitialMsec = Milliseconds();
}


SInt64 OS::Milliseconds()
{
	struct timeval t;
	struct timezone tz;
	int theErr = ::gettimeofday(&t, &tz);
	ASSERT(theErr == 0);

	SInt64 curTime;
	curTime = t.tv_sec;
	curTime *= 1000;		// sec -> msec
	curTime += t.tv_usec / 1000;	// usec -> msec

	return curTime - sInitialMsec;
}

SInt64 OS::Microseconds()
{
	struct timeval t;
	struct timezone tz;
	int theErr = ::gettimeofday(&t, &tz);
	ASSERT(theErr == 0);

	SInt64 curTime;
	curTime = t.tv_sec;
	curTime *= 1000000;	        // sec -> usec
	curTime += t.tv_usec;

	return curTime - (sInitialMsec * 1000);
}

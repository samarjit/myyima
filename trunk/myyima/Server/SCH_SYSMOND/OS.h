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

#ifndef _OS_H_
#define _OS_H_

#include "../sysinclude/OSDefines.h"

class OS
{
	public:
	
		//call this before calling anything else
		static void	Initialize();

		static SInt32   Min(SInt32 a, SInt32 b) 	{ if (a < b) return a; return b; }
		
		static SInt64 	Milliseconds();
		static SInt64	Microseconds();

		static SInt64	HostToNetworkSInt64(SInt64 hostOrdered);		
		static SInt64	ConvertToSecondsSince1900(SInt64 inMilliseconds)
							{ return ConvertMsecToFixed64Sec(sMsecSince1900 + inMilliseconds); }
		static SInt64	ConvertMsecToFixed64Sec(SInt64 inMilliseconds)
							{ return (SInt64)(.5 + inMilliseconds * 4294967.296L); }
				
	private:
		static SInt64 sMsecSince1900;
		static SInt64 sInitialMsec;
};
#endif /* _OS_H_ */

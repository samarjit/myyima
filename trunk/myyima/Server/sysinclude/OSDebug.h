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

#ifndef _OSDebug_H_
#define _OSDebug_H_

#include <stdio.h>
#define DEBUG 1

/* Constant Definitions */
#ifndef NULL
    #define NULL (void *)0
#endif


/*#####################################*/
/*# Debug Macro Definitions           #*/
/*#####################################*/
#ifdef DEBUG
static void _YimaAssert(char *strFile, unsigned uLine); /* prototype */
       #define ASSERT(f)\
	       if (f)   \
		  {}    \
               else     \
	          _YimaAssert(__FILE__, __LINE__)
#else
       #define ASSERT(f)
#endif

#ifdef DEBUG
static void _YimaAssert(char *strFile, unsigned uLine)
       {
          fflush(NULL);
          fprintf(stderr, "\nAssertion failed: %s, line %u\n", strFile, uLine);
          fflush(stderr);
          abort();
          /* exit(1); */
       }
#endif

#endif    /* _OSDebug_H_ */

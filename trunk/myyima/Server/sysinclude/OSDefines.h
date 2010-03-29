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

#ifndef _OSDefines_H_
#define _OSDefines_H_

/* Constant Definitions */
#ifndef NULL
    #define NULL (void *)0
#endif


/* type definitions */
typedef unsigned char		UInt8;
typedef signed char	       	SInt8;
typedef unsigned short		UInt16;
typedef signed short		SInt16;
typedef unsigned long		UInt32;
typedef signed long		SInt32;
typedef signed long long	SInt64;
typedef unsigned long long	UInt64;
typedef float	       		Float32;
typedef double	       		Float64;


typedef unsigned int  WORD;
typedef unsigned char BYTE;

typedef enum Boolean {
   FALSE,
   TRUE
} Boolean;
#endif    /* _OSDefines_H_ */

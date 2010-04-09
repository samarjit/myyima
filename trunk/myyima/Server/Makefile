#*******************************************************************************
#
#                          Yima Personal Edition v1.0
#                               January 10, 2003
#
#                            Streaming Media System
#                        http://streamingmedia.usc.edu
#
#                        Department of Computer Science
#                        Integrated Media Systems Center
#                       University of Southern California
#                             Los Angeles, CA 90089
#
#    Developed by:
#
#    Roger Zimmermann
#    Cyrus Shahabi
#    Kun Fu
#    Sahitya Gupta
#    Mehrdad Jahangiri
#    Farnoush Banaei-Kashani
#    Hong Zhu
#
#
#Copyright 2003, University of Southern California. All Rights Reserved.
#
#This software is experimental in nature and is provided on an AS-IS basis only.
#The University SPECIFICALLY DISCLAIMS ALL WARRANTIES, EXPRESS AND IMPLIED,
#INCLUDING WITHOUT LIMITATION ANY WARRANTY AS TO MERCHANTIBILITY OR FITNESS FOR
#A PARTICULAR PURPOSE.
#
#This software may be reproduced and used for non-commercial purposes only,
#so long as this copyright notice is reproduced with each such copy made.
#
#*******************************************************************************
LIBNAME=libqpthr.a
LDFLAGS=
LDLIBS=-lpthread -lstdc++ -lm -Lqpthread-1.3.1/
CFLAGS=$(DEBUG_FL) -Wall -Wno-deprecated -g -O2 -D_REENTRANT -I.. -Iqpthread-1.3.1/
CXX=g++
CC=g++
LIBS = 
RTSPCC = g++
RTSPCFLAGS = -g -w
RTSP_SRC_DIR = ./RTSPS/
FLIB_SRC_DIR = ./FLIB/
SCH_SRC_DIR = ./SCH_SYSMOND/
#DEBUG_FL = -ggdb
QPTHRLIB= -lqpthr

PROG= yimaserver

YIMAPE_OBJ= $(SCH_SRC_DIR)OS.o \
	$(SCH_SRC_DIR)scheduler_sysmon.o \
	$(SCH_SRC_DIR)server_manager.o \
	interface.o \
	retrieve.o \
	rtsps.o \
	rtsps_back.o \
	rtsps_front.o

all: ${PROG}

.cc.o:
	$(CXX) ${CFLAGS} -c -o $@ $<
.c.o:
	$(CC) -c $(CFLAGS) $*.c

interface.o:	$(FLIB_SRC_DIR)interface.c retrieve.o $(FLIB_SRC_DIR)FileInterface.h
	$(CC) -c $(CFLAGS) $(FLIB_SRC_DIR)interface.c

retrieve.o:	$(FLIB_SRC_DIR)retrieve.c $(FLIB_SRC_DIR)FileInterface.h
	$(CC) -c $(CFLAGS) $(FLIB_SRC_DIR)retrieve.c


rtsps.o: $(RTSP_SRC_DIR)rtsps.c $(RTSP_SRC_DIR)rtsps.h
	$(RTSPCC) $(RTSPCFLAGS) -c $(RTSP_SRC_DIR)rtsps.c

rtsps_back.o: $(RTSP_SRC_DIR)rtsps_back.c $(RTSP_SRC_DIR)rtsps.h
	$(RTSPCC) $(RTSPCFLAGS) -c $(RTSP_SRC_DIR)rtsps_back.c

rtsps_front.o: $(RTSP_SRC_DIR)rtsps_front.c $(RTSP_SRC_DIR)rtsps.h
	$(RTSPCC) $(RTSPCFLAGS) -c $(RTSP_SRC_DIR)rtsps_front.c


yimaserver: $(YIMAPE_OBJ)
	$(CXX) ${LDFLAGS} -o yimaserver $(YIMAPE_OBJ)  -L..  $(LDLIBS) $(QPTHRLIB)


clean:
	/bin/rm -rf *.o */*.o *~ core a.out ${PROG}

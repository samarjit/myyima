include ../Make.def

#DEBUG_FL=-g -D_DEBUG

#
# Solaris Sparc, SUNWspro
#
ifeq ($(DEST), Solaris_Sparc)
LIBNAME=libqpthr.so
LDFLAGS=-mt
LDLIBS=-lposix4 -lpthread
#CFLAGS=$(DEBUG_FL) -pta -mt -D_REENTRANT -I..
CFLAGS=$(DEBUG_FL) -mt -D_REENTRANT -I..
ifeq ($(USE_STL), YES)
CFLAGS+=-I../$(STL_INC)
endif
CXX=CC
endif

#
# Solaris Sparc, egcs
#
ifeq ($(DEST), Solaris_Sparc_egcs)
LIBNAME=libqpthr.so
LDFLAGS=
LDLIBS=-lposix4 -lpthread
CFLAGS=$(DEBUG_FL) -Wall -D_REENTRANT -D_SVID_SOURCE -DARCH_TAS -DSOLARIS -I..
CXX=g++
endif

#
# Solaris Intel, SUNWspro
#
ifeq ($(DEST), Solaris_Intel)
LIBNAME=libqpthr.so
LDFLAGS=-pta -mt
LDLIBS=-lposix4 -lpthread
CFLAGS=$(DEBUG_FL) -pta -mt -D_REENTRANT -I..
ifeq ($(USE_STL), YES)
CFLAGS+=-I../$(STL_INC)
endif
CXX=CC
endif

#
# Linux Intel
#
ifeq ($(DEST), Linux_Intel)
LIBNAME=libqpthr.so.0
LDFLAGS=
LDLIBS=-lpthread -lstdc++
CFLAGS=$(DEBUG_FL) -Wall -O2 -D_REENTRANT -I..
CXX=g++
endif

#
# Linux Alpha
#
ifeq ($(DEST), Linux_Alpha)
LIBNAME=libqpthr.so.0
LDFLAGS=
LDLIBS=-lpthread -lstdc++
CFLAGS=$(DEBUG_FL) -Wall -O2 -D_REENTRANT -I..
CXX=g++
endif

#
# Linux Sparc
#
ifeq ($(DEST), Linux_Sparc)
LIBNAME=libqpthr.so.0
LDFLAGS=
LDLIBS=-lpthread -lstdc++
CFLAGS=$(DEBUG_FL) -Wall -O2 -D_REENTRANT -I..
CXX=g++
endif

#
# FreeBSD Intel
#
ifeq ($(DEST), FreeBSD_Intel)
LIBNAME=libqpthr.so.0
LDFLAGS=
LDLIBS= -lstdc++
CFLAGS=$(DEBUG_FL) -Wall -O -D_REENTRANT -I..
CXX=g++ -pthread
endif

#
# DEC Alpha, DEC C++ 6.0
#
ifeq ($(DEST), OSF1_Alpha)
LIBNAME=libqpthr.so
LDFLAGS=
LDLIBS=-lrt -pthread
#CFLAGS=$(DEBUG_FL) -pta -pthread -D_REENTRANT -I..
CFLAGS=-O4 -pta -pthread -D_REENTRANT -I..
CXX=cxx
endif

#LIBNAME=libqpthr.a

QPTHRLIB=-lqpthr

PROG=hello hellorun synchperf queue sleep hellopool signal timer pool cancel \
	threadtest stack once tsd tsdt rwlock except

all: ${PROG}


.cc.o:
	$(CXX) ${CFLAGS} -c -o $@ $<

ifeq ($(DEST), Solaris_Intel)
synchperf.o: synchperf.cc
	$(CXX) ${CFLAGS} -DZERO_RUSAGE -c -o $@ $<
endif

hello: hello.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

hellorun: hellorun.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

synchperf: synchperf.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

queue: queue.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

sleep: sleep.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

hellopool: hellopool.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

signal: signal.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS) -lm

timer: timer.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

pool: pool.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

cancel: cancel.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

threadtest: threadtest.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

stack: stack.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

once: once.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

tsd: tsd.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

tsdt: tsdt.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

rwlock: rwlock.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)

except: except.o 
	$(CXX) ${LDFLAGS} -o $@ $< -L.. $(QPTHRLIB) $(LDLIBS)


clean:
	rm -rf *.o core a.out Templates.DB cxx_repository ${PROG} SunWS_cache

cleandist: clean

clean-dist: clean

dist-clean: clean

distclean: clean

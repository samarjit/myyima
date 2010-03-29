/*
 *
 *	This is free software. You can redistribute it and/or modify under
 *	the terms of the GNU Library General Public License version 2.
 * 
 *	Copyright (C) 1999 by kra
 *
 */
#ifndef __QP_H
#define __QP_H

// switch of the warning 4786 on Microsoft compiler
#ifdef _MSC_VER
#pragma warning( disable : 4786 )
#endif

#ifndef WIN32
#include <sys/time.h>
#include <unistd.h>
#endif
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <vector>
#include <list>
#include <deque>

#ifdef WIN32
#include <sched.h>
#include <iostream>
#endif

#if !defined( _REENTRANT ) && !defined( WIN32 )
#error you have to compile with -D_REENTRANT
#endif

#include <qpthr/qp_ns.h>

__BEGIN_QPTHR_NAMESPACE_PURE

const unsigned int QP_PRIO_DFL    = 0;
const unsigned int QP_PRIO_MIN    = 1;
const unsigned int QP_PRIO_AVGMIN = 2;
const unsigned int QP_PRIO_AVG	  = 3;
const unsigned int QP_PRIO_AVGMAX = 4;
const unsigned int QP_PRIO_MAX	  = 5;

const unsigned int QP_PRIO_OFFSET = 1000000;

__END_QPTHR_NAMESPACE

#include <qpthr/qp_base.h>
#include <qpthr/qp_except.h>
#include <qpthr/qp_synch.h>
#include <qpthr/qp_thr.h>
#include <qpthr/qp_sig.h>
#include <qpthr/qp_queue.h>
#include <qpthr/qp_timer.h>
#include <qpthr/qp_misc.h>
#include <qpthr/qp_rwlock.h>

__BEGIN_QPTHR_NAMESPACE

class QpInit {
    public:
	enum QP_CAP { QP_NONE = 0, QP_SIGNAL = 1, QP_TIMER = 2, QP_ALL=255 };
	enum QP_SCOPE { QP_SCOPE_SYSTEM = 0, QP_SCOPE_PROCESS = 1 };
	enum QP_SCHED { QP_SCHED_OTHER, QP_SCHED_FIFO, QP_SCHED_RR };
    private:
	static QpInit 	*qp_initialized;
	static QP_CAP   qp_capability;
	static int	qp_scope;
	static int	qp_sched_policy;
	static int	qp_prio_table[QP_PRIO_MAX + 1];
	static QpSignal *qp_signal;
	static QpTimer  *qp_timer;

    public:	
	QpInit(QP_CAP capability = QP_NONE, QP_SCOPE scope = QP_SCOPE_SYSTEM, QP_SCHED sched = QP_SCHED_OTHER);
	~QpInit();

	static bool Initialized() { if (qp_initialized) return true; else return false;}
	static QP_CAP Capability(QP_CAP mask = QP_ALL);
	static int DflSchedPolicy();
	static int DflScope();
	static int PrioValue(unsigned int p);
	static void AdjustSchedParam(unsigned int prio, struct sched_param *sch);
};

__END_QPTHR_NAMESPACE

#endif

/*
 *
 *	This is free software. You can redistribute it and/or modify under
 *	the terms of the GNU Library General Public License version 2.
 * 
 *	Copyright (C) 1999 by kra
 *
 */
#include <qpthr/qp.h>

__BEGIN_QPTHR_NAMESPACE

QpInit		 *QpInit::qp_initialized = NULL;
QpInit::QP_CAP   QpInit::qp_capability = QpInit::QP_NONE;
#ifndef WIN32
int		 QpInit::qp_scope = PTHREAD_SCOPE_SYSTEM;
#else
int		 QpInit::qp_scope = 0; // dummy
#endif // WIN32
int		 QpInit::qp_sched_policy = SCHED_OTHER;
int 		 QpInit::qp_prio_table[QP_PRIO_MAX + 1];
QpSignal 	 *QpInit::qp_signal = NULL;
QpTimer  	 *QpInit::qp_timer = NULL;

QpInit::QpInit(QP_CAP capability, QP_SCOPE scope, QP_SCHED sched)
{
	int sched_min, sched_max, sched_4;

	if (!qp_initialized) {
		qp_capability = capability;
#ifndef WIN32
		switch (scope) {
		    case QP_SCOPE_SYSTEM:
			qp_scope = PTHREAD_SCOPE_SYSTEM;
			break;
		    case QP_SCOPE_PROCESS:
			qp_scope = PTHREAD_SCOPE_PROCESS;
			break;
		}
#endif // WIN32
		switch (sched) {
		    case QP_SCHED_OTHER:
			qp_sched_policy = SCHED_OTHER;
			break;
		    case QP_SCHED_FIFO:
			qp_sched_policy = SCHED_FIFO;
			break;
		    case QP_SCHED_RR:
			qp_sched_policy = SCHED_RR;
			break;
		}
		
		sched_min = sched_get_priority_min(DflSchedPolicy());
		sched_max = sched_get_priority_max(DflSchedPolicy());
		sched_4 = (sched_max - sched_min) / 4;
		qp_prio_table[QP_PRIO_MIN] = sched_min;
		qp_prio_table[QP_PRIO_AVGMIN] = sched_min + sched_4;
		qp_prio_table[QP_PRIO_AVG] = sched_min + (sched_max - sched_min) / 2;
		qp_prio_table[QP_PRIO_AVGMAX] = sched_max - sched_4;
		qp_prio_table[QP_PRIO_MAX] = sched_max;
		qp_prio_table[QP_PRIO_DFL] = qp_prio_table[QP_PRIO_AVG];
		
		if (capability & QP_SIGNAL)
			QpSignal::QpInit();

		QpThread *qp_main_thr = new QpThread(QP_PRIO_DFL, 0, "Main"); /* Main thread */
		if (!qp_main_thr)
			throw QpOutOfMemoryException();
		qp_main_thr->ThrInitFirst(&capability);
		qp_initialized = this;
		
		if (capability & QP_SIGNAL) {
#ifdef WIN32
			throw QpUnsupportedException();
#else
			qp_signal = new QpSignal;
			if (!qp_signal)
				throw QpOutOfMemoryException();
#endif
		}		
		if (capability & QP_TIMER) {
			qp_timer = new QpTimer;
			if (!qp_timer)
				throw QpOutOfMemoryException();
		}
	}
}

QpInit::~QpInit()
{
	if (qp_initialized == this) {
		if (qp_capability & QP_TIMER)
			delete qp_timer;
		if (qp_capability & QP_SIGNAL)
			delete qp_signal;
		delete QpThread::MainThread();
		QpThread::WaitForAllThreads();
		qp_initialized = NULL;
		
		if (qp_capability)
			QpSignal::QpDone();
	}
}

QpInit::QP_CAP QpInit::Capability(QP_CAP mask)
{
	return (QP_CAP) (qp_capability & mask);
}

int QpInit::DflSchedPolicy()
{
	return qp_sched_policy;
}

int QpInit::DflScope()
{
	return qp_scope;
}

int QpInit::PrioValue(unsigned int p)
{
	if (p < QP_PRIO_DFL || p > QP_PRIO_MAX)
		throw QpErrorException(QP_ERROR_ARG, "PrioValue");
	return qp_prio_table[p];
}

void QpInit::AdjustSchedParam(unsigned int prio, struct sched_param *sch)
{
	if (prio > QP_PRIO_MAX) {
		if (prio < QP_PRIO_OFFSET + qp_prio_table[QP_PRIO_MIN] ||
		    prio > QP_PRIO_OFFSET + qp_prio_table[QP_PRIO_MAX])
			throw QpErrorException(QP_ERROR_ARG, "AdjustSchedParam");
		sch->sched_priority = prio - QP_PRIO_OFFSET;
	} else
		sch->sched_priority = qp_prio_table[prio];
}


__END_QPTHR_NAMESPACE


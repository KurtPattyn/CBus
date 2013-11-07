#include "signalwaiter.h"


SignalWaiter::SignalWaiter(const QObject *sender, const char *member) :
	QObject(0),
	m_timerId(0),
	m_signalCaught(false),
	m_loop()
{
	connect(sender, member, this, SLOT(signalCaught()));
}

SignalWaiter::~SignalWaiter()
{
	if (m_timerId)
	{
		killTimer(m_timerId);
		m_timerId = 0;
	}
}

bool SignalWaiter::wait(int msecs)
{
	if (msecs > 0)
	{
		m_timerId = startTimer(msecs, Qt::PreciseTimer);
	}
	int retVal = m_loop.exec(QEventLoop::AllEvents /*| QEventLoop::WaitForMoreEvents*/);
	if (m_timerId)
	{
		killTimer(m_timerId);
		m_timerId = 0;
	}
	m_signalCaught = (retVal == 0);
	return m_signalCaught;
}

void SignalWaiter::timerEvent(QTimerEvent *)
{
	killTimer(m_timerId);
	m_timerId = 0;
	m_loop.exit(-1);
}

void SignalWaiter::signalCaught()
{
	if (m_timerId)
	{
		killTimer(m_timerId);
		m_timerId = 0;
	}
	m_loop.exit(0);
}

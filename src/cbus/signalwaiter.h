#ifndef SIGNALWAITER_H
#define SIGNALWAITER_H

#include <QObject>
#include <QEventLoop>
#include <QTimerEvent>

class SignalWaiter:public QObject
{
	Q_OBJECT
public:
	SignalWaiter(const QObject *sender, const char *member);
	~SignalWaiter();

	bool wait(int msecs = -1);
	bool hasCapturedSignal() const;

private Q_SLOTS:
	void timerEvent(QTimerEvent *);
	void signalCaught();

private:
	int m_timerId;
	bool m_signalCaught;
	QEventLoop m_loop;
};

#endif // SIGNALWAITER_H

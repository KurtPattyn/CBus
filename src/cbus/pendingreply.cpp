#include "pendingreply.h"
//#include "cbus.h"
#include "signalwaiter.h"
#include <QMetaMethod>
#include <QDebug>

PendingReply::PendingReply(QObject *parent) :
    QObject(parent),
    m_result(),
    m_errorString(),
    m_error(cbus::NO_ERROR),
    m_isReady(false),
    m_timer()
{
    QObject::connect(this, SIGNAL(ready()), this, SIGNAL(finished()));
    QObject::connect(this, SIGNAL(error()), this, SIGNAL(finished()));
}

PendingReply::PendingReply(int timeout, QObject *parent) :
    QObject(parent),
    m_result(),
    m_errorString(),
    m_error(cbus::NO_ERROR),
    m_isReady(false),
    m_timer()
{
    if (timeout > 0)
    {
        m_timer.setInterval(timeout);
        m_timer.setSingleShot(true);
        connect(&m_timer, SIGNAL(timeout()), this, SLOT(onTimeout()));
        m_timer.start();
    }
}

QVariant PendingReply::getResult() const
{
    return m_result;
}

void PendingReply::setResult(const QVariant &result)
{
    m_timer.stop();
    m_result = result;
    m_isReady = true;
    Q_EMIT ready();
}

void PendingReply::setError(cbus::Error err, QString errorString)
{
    m_timer.stop();
    m_error = err;
    m_errorString = errorString;
    m_isReady = true;
    Q_EMIT error();
}

void PendingReply::setError(cbus::Error err, const char *errorString)
{
    setError(err, QString::fromLatin1(errorString));
}

cbus::Error PendingReply::getError() const
{
    return m_error;
}

QString PendingReply::getErrorString() const
{
    return m_errorString;
}

bool PendingReply::isReady() const
{
    return m_isReady;
}

void PendingReply::connectNotify(const QMetaMethod &signal)
{
    if ((signal.name() == "ready") && m_isReady && (m_error == cbus::NO_ERROR))
    {
        Q_EMIT ready();
    }
    else if ((signal.name() == "error") && m_isReady && (m_error != cbus::NO_ERROR))
    {
        Q_EMIT error();
    }
    else if ((signal.name() == "finished") && m_isReady)
    {
        Q_EMIT finished();
    }
}

void PendingReply::onTimeout()
{
    //qDebug() << "PendingReply: Reply timed out";
    setError(cbus::TIMEOUT, "Timeout occurred.");
}

bool PendingReply::isOK() const
{
    return m_error == cbus::NO_ERROR;
}

bool PendingReply::waitForResult(int timeout)
{
    SignalWaiter w(this, SIGNAL(finished()));
    bool result = w.wait(timeout);
    if (!result)
    {
        setError(cbus::TIMEOUT, "Timeout occurred.");
    }
    return (getError() != cbus::TIMEOUT);
}

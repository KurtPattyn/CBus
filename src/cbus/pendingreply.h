#ifndef PENDINGREPLY_H
#define PENDINGREPLY_H

#include <QObject>
#include <QTimer>
#include <QString>
#include <QVariant>
#include "error.h"
#include "global.h"

class QMetaMethod;

class PendingReply : public QObject
{
    Q_OBJECT
public:
    explicit PendingReply(QObject *parent = 0);
    explicit PendingReply(int timeout, QObject *parent = 0);

    QVariant getResult() const;
    void setResult(const QVariant &result);

    void setError(cbus::Error error, QString errorString);
    void setError(cbus::Error error, const char *errorString);
    cbus::Error getError() const;
    QString getErrorString() const;
    bool isOK() const;

    bool waitForResult(int timeout = -1);

    bool isReady() const;

    template <typename Lambda>
    void onReady(Lambda callback)
    {
        QObject::connect(this, &PendingReply::ready, function() {
                             callback();
                             deleteLater();
                         });
    }
    template <typename Lambda>
    void onError(Lambda callback)
    {
        QObject::connect(this, &PendingReply::error, function() {
                             callback();
                             deleteLater();
                         });
    }
    template <typename Lambda>
    void onFinished(Lambda callback)
    {
        QObject::connect(this, &PendingReply::finished, function() {
                             callback();
                             deleteLater();
                         });
    }

protected:
    void connectNotify(const QMetaMethod &signal);

Q_SIGNALS:
    void ready();
    void error();
    void finished();

private Q_SLOTS:
    void onTimeout();

private:
    QVariant m_result;
    QString m_errorString;
    cbus::Error m_error;
    bool m_isReady;
    QTimer m_timer;
};

#endif // PENDINGREPLY_H

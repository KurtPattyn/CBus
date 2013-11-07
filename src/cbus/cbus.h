#ifndef CBUS_H
#define CBUS_H

#include <QObject>
#include <QVariantList>
#include <QMap>
#include <QMetaMethod>
#include <QPair>
#include <QJsonArray>
#include "utility.h"
#include "functor.h"
#include "error.h"
#include "global.h"
#include "pendingreply.h"

class SocketIOClient;
class Object;

#define CBUS_SERVICE	"com.barco.cbus"
#define CBUS_OBJECT     "CBus"

class CBus : public QObject
{
    Q_OBJECT

public:
    explicit CBus(QObject *parent = 0);

    bool connect(const QUrl &serverUrl);

    bool isConnected() const;

    //execute a remote method
    PendingReply *execute(const QString &serviceName, const QString &objectName, const QString &method, const QVariantList &arguments, bool expectReturnValue = true);
    PendingReply *execute(const QString &serviceName, const QString &objectName, const QString &method, const QVariant &arguments, bool expectReturnValue = true);

    PendingReply *registerService(const QString &serviceName);
    PendingReply *registerObject(const QString &serviceName, const QString &objectName, QObject *pObject);
    PendingReply *getServices();
    PendingReply *subscribeToSignal(const QString &serviceName, const QString &objectName, const QString &signalName, QObject *target, const char *member, bool sendParametersAsIs = false);
    void emitSignal(const QString &serviceName, const QString &objectName, const QString &signalName, const QVariant &arguments);
    PendingReply *subscribeToMethod(const QString &serviceName, const QString &objectName, const QString &methodName, QObject *receiver, const char *member);
    PendingReply *subscribeToMethod(const QString &serviceName, const QString &objectName, const QString &methodName, QObject *receiver, QMetaMethod member);

    template <typename Function>
    typename cbus::utility::enableIf<cbus::functor::FunctionChecker<Function>::isLambda, bool>::Type
    getObject(const QString &serviceName, const QString &objectName, Function callback)
    {
        PendingReply *pendingReply = getObject(serviceName, objectName);
        pendingReply->onReady(function() {
            callback(qvariant_cast<Object *>(pendingReply->getResult()));
            pendingReply->deleteLater();
        });
        return true;
    }

    PendingReply *getObject(const QString &serviceName, const QString &objectName);

Q_SIGNALS:
    void connected(QString endpoint);
    void receivedReply(QString messageId, QVariant arguments);
    void receivedError(QString messageId, int errNo, QString errDescription);
    void objectRegistered(QString objectName);

private Q_SLOTS:
    void onEventReceived(const QString &message, const QJsonArray arguments);
    void onMessageReceived(QString message);
    void onAckReceived(int messageId, const QJsonArray arguments);
    void onErrorReceived(QString reason, QString advice);
    void onConnected(QString endpoint);
    void onDisconnected(QString endpoint);
    void onReceivedReply(QString messageId, QVariant result);
    void onReceivedError(QString messageId, int errNo, QString errDescription);
    void onServiceRegistered(QString serviceName);

private:
    SocketIOClient *m_pSocketIOClient;
    int m_requestId;
    bool m_isConnected;
    QMap<QString, PendingReply *> m_pendingReplies;

    typedef QPair<QObject *, QMetaMethod> EventEntry;
    QMap<QString, QPair<EventEntry, bool> > m_events;

    QString createUniqueId();

    void sendReply(QString requestId, const QVariantList &arguments);
};

class Model:public QObject
{
    Q_OBJECT
public:
    Model(QObject *parent = 0);

Q_SIGNALS:
    void propertyRemoved(QByteArray propertyName);
    void propertyChanged(QByteArray propertyName);

protected:
    bool event(QEvent *pEvent);
};

#endif // CBUS_H

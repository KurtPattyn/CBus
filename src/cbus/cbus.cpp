#include "cbus.h"

#include "socketioclient.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QVariantList>
#include <QVariant>
#include <QMetaMethod>
#include <QString>
#include <QMap>
#include <QDebug>
#include "objectadaptor.h"
#include "pendingreply.h"
#include "callhelper.h"
#include "object.h"
#include "objectproxy.h"

CBus::CBus(QObject *parent) :
    QObject(parent),
    m_pSocketIOClient(0),
    m_requestId(0),
    m_isConnected(false),
    m_pendingReplies(),
    m_events()
{
    //TODO: socketIOClient should go into a Transport object
    //in this way, we can provide different transports for the cbus (WebSockets, socket.io, AMQP, ...)
    m_pSocketIOClient = new SocketIOClient(this);

    QObject::connect(m_pSocketIOClient, SIGNAL(connected(QString)), this, SLOT(onConnected(QString)));
    QObject::connect(m_pSocketIOClient, SIGNAL(ackReceived(int,QJsonArray)), this, SLOT(onAckReceived(int,QJsonArray)));
    QObject::connect(m_pSocketIOClient, SIGNAL(disconnected(QString)), this, SLOT(onDisconnected(QString)));
    QObject::connect(m_pSocketIOClient, SIGNAL(errorReceived(QString,QString)), this, SLOT(onErrorReceived(QString,QString)));
    QObject::connect(m_pSocketIOClient, SIGNAL(eventReceived(QString,QJsonArray)), this, SLOT(onEventReceived(QString,QJsonArray)));
    QObject::connect(m_pSocketIOClient, SIGNAL(messageReceived(QString)), this, SLOT(onMessageReceived(QString)));

    QObject::connect(this, SIGNAL(receivedReply(QString,QVariant)), this, SLOT(onReceivedReply(QString,QVariant)));
    QObject::connect(this, SIGNAL(receivedError(QString,int,QString)), this, SLOT(onReceivedError(QString,int,QString)));
}

bool CBus::connect(const QUrl &serverUrl)
{
    return m_pSocketIOClient->open(serverUrl);
}

bool CBus::isConnected() const
{
    return m_isConnected;
}

PendingReply *CBus::execute(const QString &serviceName, const QString &objectName, const QString &method, const QVariantList &arguments, bool expectReturnValue)
{
    Q_UNUSED(expectReturnValue)
    //TODO: if local object call object directly
    //and set the result in the pendingreply, in a timeout event
    QJsonObject jo;
    jo["serviceName"] = serviceName;
    jo["objectName"] = objectName;
    jo["targetName"] = method;
    jo["parameters"] = QJsonArray::fromVariantList(arguments);
    QJsonArray ja;
    ja.append(jo);
    PendingReply *pendingReply = new PendingReply(this);
//	if (expectReturnValue)
    {
        QString uniqueId = createUniqueId();
        ja.append(uniqueId);
        m_pendingReplies.insert(uniqueId, pendingReply);
    }
//	else
//	{
//		pendingReply->setResult(QVariant());
//	}
    m_pSocketIOClient->emitMessage("execute", ja.toVariantList());
    return pendingReply;
}

PendingReply *CBus::execute(const QString &serviceName, const QString &objectName, const QString &method, const QVariant &arguments, bool expectReturnValue)
{
    return execute(serviceName, objectName, method, QVariantList() << arguments, expectReturnValue);
}

void CBus::emitSignal(const QString &serviceName, const QString &objectName, const QString &signalName, const QVariant &arguments)
{
    //TODO: if local object emit directly
    //qDebug() << "CBus::emitSignal:" << serviceName << "." << objectName << "." << signalName << "(" << arguments << ")";
    QJsonObject jo;
    QJsonArray params;
    params.append(QJsonValue::fromVariant(arguments));
    jo["serviceName"] = serviceName;
    jo["objectName"] = objectName;
    jo["targetName"] = signalName;
    jo["parameters"] = params;
    QJsonArray ja;
    ja.append(jo);
    m_pSocketIOClient->emitMessage("emitSignal", ja.toVariantList());
}

PendingReply *CBus::registerService(const QString &serviceName)
{
    QString uniqueId = createUniqueId();
    QVariantList args = QVariantList() << serviceName << uniqueId;
    PendingReply *pendingReply = new PendingReply(this);
    m_pendingReplies.insert(uniqueId, pendingReply);
    m_pSocketIOClient->emitMessage("registerService", args);
    return pendingReply;
}

PendingReply *CBus::registerObject(const QString &serviceName, const QString &objectName, QObject *pObject)
{
    QString uniqueId = createUniqueId();
    ObjectAdaptor *mba = new ObjectAdaptor(*this, serviceName, objectName, pObject, this);
    //TODO: add to registered objects
    PendingReply *pendingReply = new PendingReply(this);
    m_pendingReplies.insert(uniqueId, pendingReply);
    mba->connect(mba, &ObjectAdaptor::ready, function() {
        //qDebug().nospace() << "Object " << qPrintable(serviceName) << "." << qPrintable(objectName) << " registered.";
        pendingReply->setResult(true);
    });
    return pendingReply;
}

PendingReply *CBus::getServices()
{
    QString uniqueId = createUniqueId();
    PendingReply *pendingReply = new PendingReply(this);
    m_pendingReplies.insert(uniqueId, pendingReply);

    m_pSocketIOClient->emitMessage("getServices", QVariantList() << QVariant() << uniqueId);

    return pendingReply;
}

void CBus::sendReply(QString requestId, const QVariantList &arguments)
{
    QVariantList args = QVariantList() << requestId;
    if (!arguments.isEmpty())
    {
        args.append(arguments);
    }
    m_pSocketIOClient->emitMessage("reply", args);
}

PendingReply *CBus::subscribeToSignal(const QString &serviceName, const QString &objectName, const QString &signalName, QObject *target, const char *member, bool sendParametersAsIs)
{
    PendingReply *pendingReply = new PendingReply(this);
    bool isValid = false;
    if (!member || !*member)
    {
        qDebug() << "CBus::subscribeToSignal: member is null or empty";
        pendingReply->setError(cbus::INVALID_ARGUMENT, "member parameter is null or empty");
    }
    else
    {
        isValid = (isnumber(member[0]) && ((member[0] == (QSLOT_CODE + '0')) || (member[0] == (QSIGNAL_CODE + '0'))));
        if (!isValid)
        {
            qDebug() << "CBus::subscribeToSignal: Only signals and slots are valid as targets for a signal subscription.";
            pendingReply->setError(cbus::INVALID_ARGUMENT, "Only signals and slots are valid as targets for a signal subscription.");
        }
        else
        {
            ++member;
            QByteArray normalizedSignature = QMetaObject::normalizedSignature(member);
            int idx = target->metaObject()->indexOfMethod(normalizedSignature);
            if (idx >= 0)
            {
                QMetaMethod mm = target->metaObject()->method(idx);
                m_events.insertMulti(serviceName + "." + objectName + "." + signalName, QPair<EventEntry, bool>(EventEntry(target, mm), sendParametersAsIs));

                QString uniqueId = createUniqueId();
                QJsonObject jo;
                jo["serviceName"] = serviceName;
                jo["objectName"] = objectName;
                jo["targetName"] = signalName;
                QJsonArray ja;
                ja.append(jo);
                ja.append(uniqueId);
                m_pendingReplies.insert(uniqueId, pendingReply);
                m_pSocketIOClient->emitMessage("subscribeToSignal", ja.toVariantList());
            }
            else
            {
                qDebug() << "CBus::subscribeToSignal:" << member << "not found.";
                pendingReply->setError(cbus::INVALID_ARGUMENT, QString("Member ") + member + " not found.");
            }
        }
    }
    return pendingReply;
}

PendingReply *CBus::getObject(const QString &serviceName, const QString &objectName)
{
    PendingReply *pendingReply = new PendingReply;
    Object *pObject = new ObjectProxy(*this, serviceName, objectName);
    pendingReply->setResult(QVariant::fromValue<Object *>(pObject));
    return pendingReply;
}

PendingReply *CBus::subscribeToMethod(const QString &serviceName, const QString &objectName, const QString &methodName, QObject *receiver, const char *member)
{
    bool isValid = false;
    PendingReply *pendingReply = new PendingReply(this);
    if (!member || !*member)
    {
        qDebug() << "CBus::subscribeToMethod: method is null or empty";
        pendingReply->setError(cbus::INVALID_ARGUMENT, "Method is null or empty");
    }
    else
    {
        isValid = (isnumber(member[0]) && ((member[0] == (QSLOT_CODE + '0')) || (member[0] == (QSIGNAL_CODE + '0'))));
        if (!isValid)
        {
            qDebug() << "CBus::subscribeToMethod: Only signals and slots are valid as targets for a method subscription.";
            pendingReply->setError(cbus::INVALID_ARGUMENT, "Only signals and slots are valid as targets for a method subscription");
        }
        else
        {
            ++member;
            QByteArray normalizedSignature = QMetaObject::normalizedSignature(member);
            int idx = receiver->metaObject()->indexOfMethod(normalizedSignature);
            if (idx >= 0)
            {
                QMetaMethod mm = receiver->metaObject()->method(idx);
                QString fullMethodName = serviceName + "." + objectName + "." + methodName;
                if (m_events.contains(fullMethodName))
                {
                    qDebug() << "CBus::subscribeToMethod: There is already a subscription for method" << fullMethodName;
                    pendingReply->setError(cbus::INVALID_ARGUMENT, QString("There is already a subscription for method ").append(fullMethodName));
                }
                else
                {
                    m_events.insert(fullMethodName, QPair<EventEntry, bool>(EventEntry(receiver, mm), false));
                    pendingReply->setResult(true);
                }
            }
            else
            {
                qDebug() << "CBus::subscribeToMethod:" << member << "not found.";
                pendingReply->setError(cbus::INVALID_ARGUMENT, QString("Member ").append(member).append(" not found"));
            }
        }
    }
    return pendingReply;
}

PendingReply *CBus::subscribeToMethod(const QString &serviceName, const QString &objectName, const QString &methodName, QObject *receiver, QMetaMethod member)
{
    PendingReply *pendingReply = new PendingReply(this);
    QString fullMethodName = serviceName + "." + objectName + "." + methodName;
    if (m_events.contains(fullMethodName))
    {
        qDebug() << "CBus::subscribeToMethod: There is already a subscription for method" << fullMethodName;
        pendingReply->setError(cbus::INVALID_ARGUMENT, QString("There is already a subscription for method ").append(fullMethodName));
    }
    else
    {
        m_events.insert(fullMethodName, QPair<EventEntry, bool>(EventEntry(receiver, member), false));
        pendingReply->setResult(true);
    }
    return pendingReply;
}

QStringList reservedEvents = QStringList() << "message" << "connect" << "disconnect" << "open" << "close" << "error" << "retry" << "reconnect";

void CBus::onEventReceived(const QString &message, const QJsonArray arguments)
{
    //qDebug() << "Event received" << message << "with arguments" << arguments;
    if (!reservedEvents.contains(message))
    {
        if ((message != "reply") && (message != "error!"))
        {
            QString messageId;
            if (arguments.size() > 1)
            {
                messageId = arguments[1].toString();
            }
            if (m_events.contains(message))
            {
                typedef QPair<EventEntry, bool> EE;
                QList<EE> values = m_events.values(message);
                Q_FOREACH(EE e, values)
                {
                    EventEntry ee = e.first;
                    const bool sendAsIs = e.second;
                    QMetaMethod mm = ee.second;
                    QObject *target = ee.first;
                    bool ret = false;
                    //arguments[0] contains an array of parameters
                    QJsonArray parameters = arguments[0].toArray();
                    QVariantList args = parameters.toVariantList();
                    QVariantList returnValue = cbus::utility::Call(target, mm, args, sendAsIs, &ret);
                    if (!messageId.isEmpty() && ret)
                    {
                        sendReply(messageId, returnValue);
                    }
                    if (!ret)
                    {
                        qDebug() << "CBus::onEventReceived: Call to" << mm.methodSignature() << "failed.";
                    }
                }
            }
            else
            {
                qDebug() << "No handler found for" << message;
            }
        }
        else if (message == "reply")
        {
            //qDebug() <<  "Received reply with arguments:"<< arguments;
            QVariant argVal = arguments[1].toVariant();
            QVariant args;
            if (argVal.isValid())
            {
                args = argVal;
            }
            Q_EMIT(receivedReply(arguments[0].toString(), args));
        }
        else	//we have an error
        {
            qDebug() << "****************************CBus::receivedEvent: We have an error" << arguments;
            QVariantMap qv = arguments[1].toObject().toVariantMap();
            int errNo = qv["error"].toInt();
            QString errDesc = qv["description"].toString();
            Q_EMIT(receivedError(arguments[0].toString(), errNo, errDesc));
        }
    }
    else
    {
        qDebug() << "Reserved event";
    }
}

QString CBus::createUniqueId()
{
    return m_pSocketIOClient->getSessionId() + QString::number(++m_requestId);
}

void CBus::onMessageReceived(QString message)
{
    qDebug() << "Received message" << message;
}

void CBus::onAckReceived(int messageId, const QJsonArray arguments)
{
    qDebug() << "Received ACK for message with id" << messageId << "Arguments:" << arguments;
}

void CBus::onErrorReceived(QString reason, QString advice)
{
    qDebug() << "Received error" << reason << "with advice" << advice;
}

/*class ServiceWatcher:public QObject
{
public:
    ServiceWatcher(const CBus &cBus, QObject *parent = 0);
    virtual ~ServiceWatcher();

    void addWatchedService(const QString &serviceName);
    void addWatchedServices(const QStringList &serviceNames);
    void removeWatchedService(const QString &serviceName);

Q_SIGNALS:
    void serviceRegistered(const QString &serviceName);
    void serviceUnregistered(const QString &serviceName);

private Q_SLOTS:
    void onServiceRegistered(const QString &serviceName);
    void onServiceUnregistered(const QString &serviceName);

private:
    const CBus &m_cBus;
    QSet<QString> m_watchedServices;

    Q_DISABLE_COPY(ServiceWatcher)
    Q_DECLARE_PRIVATE(ServiceWatcher)
};

ServiceWatcher::ServiceWatcher(const CBus &cBus, QObject *parent) :
    QObject(parent),
    m_cBus(cBus),
    m_watchedServices()
{}

ServiceWatcher::~ServiceWatcher()
{
    //unsubscribe to the signal
}

void ServiceWatcher::addWatchedService(const QString &serviceName)
{
    bool firstService = m_watchedServices.isEmpty();
    m_watchedServices.insert(serviceName);
    if (firstService)
    {
        m_cBus.subscribeToSignal(CBUS_SERVICE, CBUS_OBJECT, "serviceRegistered", this, SLOT(onServiceRegistered(QString)));
    }
}

void ServiceWatcher::addWatchedServices(const QStringList &serviceNames)
{
    bool firstService = m_watchedServices.isEmpty();
    Q_FOREACH(QString serviceName, serviceNames)
    {
        m_watchedServices.insert(serviceName);
    }
    if (firstService)
    {
        m_cBus.subscribeToSignal(CBUS_SERVICE, CBUS_OBJECT, "serviceRegistered", this, SLOT(onServiceRegistered(QString)));
    }
}

void ServiceWatcher::removeWatchedService(const QString &serviceName)
{
    m_watchedServices.remove(serviceName);
    if (m_watchedServices.isEmpty())
    {
        //unsubscribe from signal
    }
}

void ServiceWatcher::onServiceRegistered(const QString &serviceName)
{
    if (m_watchedServices.contains(serviceName))
    {
        Q_EMIT serviceRegistered(serviceName);
    }
}

void ServiceWatcher::onServiceUnregistered(const QString &serviceName)
{
    if (m_watchedServices.contains(serviceName))
    {
        Q_EMIT serviceUnregistered(serviceName);
    }
}*/

void CBus::onConnected(QString endpoint)
{
    m_isConnected = true;
    /*PendingReply *pr = getServices();
    pr->onReady(function(){
                    qDebug() << "CBus::onConnected: Registered services:" << qvariant_cast<QStringList>(pr->getResult());
                });
    subscribeToSignal(CBUS_SERVICE, CBUS_OBJECT, "serviceRegistered", this, SLOT(onServiceRegistered(QString)));
    */
    //qDebug() << "Connected endpoint[" << endpoint << "]";
    Q_EMIT(connected(endpoint));
}

void CBus::onDisconnected(QString endpoint)
{
    qDebug() << QStringLiteral("Disconnect received") << (endpoint.isEmpty() ? QStringLiteral("for whole socket") : (QStringLiteral("for endpoint") + endpoint));
}

void CBus::onReceivedReply(QString messageId, QVariant result)
{
    //qDebug() << "CBus::onReceivedReply: received reply for message with id" << messageId << "arguments:" << result;
    PendingReply *pendingReply = m_pendingReplies.value(messageId, 0);
    if (pendingReply)
    {
        pendingReply->setResult(result);
        m_pendingReplies.remove(messageId);
        //pendingReply->deleteLater();
    }
}

void CBus::onReceivedError(QString messageId, int errNo, QString errDescription)
{
    PendingReply *pendingReply = m_pendingReplies.value(messageId, 0);
    if (pendingReply)
    {
        pendingReply->setError(static_cast<cbus::Error>(errNo), errDescription);
        m_pendingReplies.remove(messageId);
        //pendingReply->deleteLater();
    }
}

void CBus::onServiceRegistered(QString serviceName)
{
    qDebug() << "CBus::onServiceRegistered:" << serviceName << "was registered";
}

Model::Model(QObject *parent) :
    QObject(parent)
{
}
#include <QEvent>
#include <QDynamicPropertyChangeEvent>
bool Model::event(QEvent *pEvent)
{
    if (pEvent->type() == QEvent::DynamicPropertyChange)
    {
        QDynamicPropertyChangeEvent *dpce = static_cast<QDynamicPropertyChangeEvent *>(pEvent);
        dpce->accept();
        qDebug() << "Dynamic property" << dpce->propertyName() << "changed";
        if (!property(dpce->propertyName()).isValid())
        {
            Q_EMIT propertyRemoved(dpce->propertyName());
        }
        else
        {
            Q_EMIT propertyChanged(dpce->propertyName());
        }
        return true;
    }
    return false;
}

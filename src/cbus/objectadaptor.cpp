#include "objectadaptor.h"
#include "dynamicqobject.h"
#include "cbus.h"
#include "callhelper.h"
#include <QDebug>

Service::Service(CBus &cBus, const QString &serviceName, QObject *parent) :
    QObject(parent),
    m_cBus(cBus),
    m_serviceName(serviceName)
{
}

CBus &Service::getCBus() const
{
    return m_cBus;
}

const QString &Service::getServiceName() const
{
    return m_serviceName;
}

void Service::onCBusRegistered()
{
}

/**
 * @brief The SignalDispatcher class maps signals from QObject derived classes to dynamically created Slot objects
 * All signals from an adapted object are enumerated and dynamically mapped to a custom slot that can stream the signals
 * parameters over the cbus
 */
class SignalDispatcher:public DynamicQObject
{
public:
    SignalDispatcher(const QString &serviceName, CBus &cBus, QObject *parent = 0);

    QString getServiceName() const;
    CBus &getCBus() const;

    void emitSignal(const QString &objectName, const QString &signalName, const QVariant &parameters);

protected:
    DynamicSlot *createSlot(const QByteArray &slotName, const QString &objectName, QMetaMethod target);

private:
    QString m_serviceName;
    CBus &m_cBus;
};

class Slot:public DynamicSlot
{
public:
    Slot(const QString &slotName, const QString &objectName, QMetaMethod target, SignalDispatcher *parent);
    void call(QObject *sender, void **arguments);

private:
    QString m_slotName;
    QString m_objectName;
    QMetaMethod m_target;
    SignalDispatcher *m_pSignalDispatcher;
};

Slot::Slot(const QString &slotName, const QString &objectName, QMetaMethod target, SignalDispatcher *parent) :
    DynamicSlot(),
    m_slotName(slotName),
    m_objectName(objectName),
    m_target(target),
    m_pSignalDispatcher(parent)
{
}

void Slot::call(QObject *sender, void **arguments)
{
    Q_UNUSED(sender);
    //qDebug() << "Dynamic slot called from sender:" << sender->objectName();
    //qDebug() << "Arguments of sender:" << m_target.parameterNames();
    QVariant parameters;
    if (m_target.parameterCount() > 1)
    {
        QVariantList parameterList;
        for (int i = 0; i < m_target.parameterCount(); ++i)
        {
            QVariant v(m_target.parameterType(i), arguments[i + 1]);
            //qDebug() << "Parameter" << (i+1) << "=" << v;
            parameterList << v;
        }
        parameters = parameterList;
    }
    else if (m_target.parameterCount() == 1)
    {
        parameters = QVariant(m_target.parameterType(0), arguments[1]);
        //qDebug() << parameters;
    }
    m_pSignalDispatcher->emitSignal(m_objectName, m_target.name(), parameters);
}

SignalDispatcher::SignalDispatcher(const QString &serviceName, CBus &cBus, QObject *parent) :
    DynamicQObject(parent),
    m_serviceName(serviceName),
    m_cBus(cBus)
{}

QString SignalDispatcher::getServiceName() const
{
    return m_serviceName;
}

CBus &SignalDispatcher::getCBus() const
{
    return m_cBus;
}

void SignalDispatcher::emitSignal(const QString &objectName, const QString &signalName, const QVariant &parameters)
{
    m_cBus.emitSignal(m_serviceName, objectName, signalName, parameters);
}

DynamicSlot *SignalDispatcher::createSlot(const QByteArray &slotName, const QString &objectName, QMetaMethod target)
{
    return new Slot(slotName, objectName, target, this);
}

ObjectAdaptor::ObjectAdaptor(CBus &cBus, const QString &serviceName, const QString &objectName, QObject *innerObject, QObject *parent) :
    Object(cBus, serviceName, objectName, parent),
    m_pInnerObject(innerObject),
    m_pSignalDispatcher(0)
{
}

void ObjectAdaptor::onCBusConnected()
{
    m_pSignalDispatcher = new SignalDispatcher(getServiceName(), getCBus(), this);
    connect(this, SIGNAL(ready()), this, SLOT(onReady()));

    //TODO: remove this line
    //getCBus().registerService(getServiceName());

    const QMetaObject *metaObj = m_pInnerObject->metaObject();
    int numMethods = metaObj->methodCount();
    for (int i = 0; i < numMethods; ++i)
    {
        QMetaMethod mm = metaObj->method(i);
        if (mm.methodType() == QMetaMethod::Signal)
        {
            bool res = m_pSignalDispatcher->connectDynamicSlot(getObjectName(), m_pInnerObject, mm);
            if (!res)
            {
                qDebug() << "ObjectAdaptor::onCBusConnected: Connection failed for signal" << mm.name();
            }
        }
        else if (mm.methodType() == QMetaMethod::Slot)
        {
            //TODO: should subscribe to the method and supply this object as target
            //cbus should then call objectadaptor.invoke(fullMethodPath, arguments, expectReturnValue);
            //we will then call doExecute(methodName, arguments, expectReturnValue)
            getCBus().subscribeToMethod(getServiceName(), getObjectName(), mm.name(), m_pInnerObject, mm);
        }
    }
    Q_EMIT ready();
}

QMetaMethod getMethod(QObject *object, const QString &methodName)
{
    const QMetaObject *mo = object->metaObject();
    int numMethods = mo->methodCount();
    QMetaMethod metaMethod;
    for (int i = 0; i < numMethods; ++i)
    {
        QMetaMethod mm = mo->method(i + mo->methodOffset());
        if (QString::fromLatin1(mm.name().constData()) == methodName)
        {
            metaMethod = mm;
            break;
        }
    }
    return metaMethod;
}

PendingReply *ObjectAdaptor::doExecute(const QString &methodName, const QVariant &argument, bool expectReturnValue) const
{
    Q_UNUSED(expectReturnValue)
    //invoke the method with methodName and arguments on the innerobject
    //if no return value is expected then do not store the result in the pending reply
    //if the invocation is not successful, set the pending reply error
    bool success = false;
    PendingReply *pendingReply = new PendingReply;
    //const QMetaObject *mo = m_pInnerObject->metaObject();
    //int idx = mo->indexOfMethod(methodName.toLatin1().constData());
    //if (idx >= 0)
    QMetaMethod metaMethod = getMethod(m_pInnerObject, methodName);
    if (metaMethod.isValid())
    {
        QVariantList returnValue = cbus::utility::Call(m_pInnerObject, metaMethod, argument, false, &success);
        if (!success)
        {
            pendingReply->setError(cbus::METHOD_CALL_FAILED, QString("Call to method ").append(methodName).append(" failed"));
        }
        else
        {
            pendingReply->setResult(returnValue.value(0));
        }
    }
    else
    {
        pendingReply->setError(cbus::METHOD_DOES_NOT_EXIST, QString("Method ").append(methodName).append(" does not exist."));
    }
    return pendingReply;
}

PendingReply *ObjectAdaptor::doExecute(const QString &methodName, const QVariantList &arguments, bool expectReturnValue) const
{
    Q_UNUSED(expectReturnValue)
    bool success = false;
    PendingReply *pendingReply = new PendingReply;
    QMetaMethod metaMethod = getMethod(m_pInnerObject, methodName);
    if (metaMethod.isValid())
    {
        QVariantList returnValue = cbus::utility::Call(m_pInnerObject, metaMethod, arguments, false, &success);
        if (!success)
        {
            pendingReply->setError(cbus::METHOD_CALL_FAILED, QString("Call to method ").append(methodName).append(" failed"));
        }
        else
        {
            pendingReply->setResult(returnValue.value(0));
        }
    }
    else
    {
        pendingReply->setError(cbus::METHOD_DOES_NOT_EXIST, QString("Method ").append(methodName).append(" does not exist."));
    }
    return pendingReply;
}

void ObjectAdaptor::onReady()
{
    //QVariantList args = QVariantList() << QString("Saying hello because cbusadaptor is ready");
    //m_cBus.execute(m_serviceName, m_objectName, "sayHello", args);
    //m_cBus.emitSignal(m_serviceName, m_objectName, "doSayHello", args);

    //args.clear();
    //args << (int)2 << (int)3;
    //m_cBus.execute(m_serviceName, m_objectName, "add", args);
}

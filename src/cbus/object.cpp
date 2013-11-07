#include "object.h"


Object::Object(CBus &cBus, const QString &serviceName, const QString &objectName, QObject *pObject) :
    QObject(pObject),
    m_cBus(cBus),
    m_serviceName(serviceName),
    m_objectName(objectName)
    //m_pObject(pObject)
{
    if (cBus.isConnected())
    {
        metaObject()->invokeMethod(this, "onCBusConnected", Qt::QueuedConnection);
        //onCBusConnected();
    }
    else
    {
        QObject::connect(&cBus, SIGNAL(connected(QString)), this, SLOT(onCBusConnected()));
    }
}

void Object::on(const QString &signalName, QObject *pReceiver, const char *member)
{
    doConnect(signalName, pReceiver, member, false);
}

bool Object::waitForReply(PendingReply *pPendingReply) const
{
    Q_ASSERT_X(pPendingReply, "Object::waitForReply", "pendingReply is 0");
    bool isOK = pPendingReply->isOK();
//	if (isOK)
//	{
//		pPendingReply->waitForResult(5000);
//		isOK = pPendingReply->isOK();
//		if (!isOK)
//		{
//			qDebug() << "Object::waitForReply: Got error:" << pPendingReply->getErrorString();
//		}
//	}
    //delete pPendingReply;
    return isOK;
}

bool Object::call(const QString &methodName, const QVariantList &params) const
{
    PendingReply *pr = doExecute(methodName, params, false);
    return waitForReply(pr);
}

bool Object::call(const QString &methodName, const QVariant &param) const
{
    PendingReply *pr = doExecute(methodName, param, false);
    return waitForReply(pr);
}
bool Object::call(const QString &methodName) const
{
    return call(methodName, QVariant());
}

CBus &Object::getCBus() const
{
    return m_cBus;
}

const QString &Object::getServiceName() const
{
    return m_serviceName;
}

const QString &Object::getObjectName() const
{
    return m_objectName;
}

void Object::onCBusConnected()
{
}

void Object::doConnect(const QString &signalName, QObject *pReceiver, const char *member, bool sendParametersAsIs) const
{
    m_cBus.subscribeToSignal(m_serviceName, m_objectName, signalName, pReceiver, member, sendParametersAsIs);
}

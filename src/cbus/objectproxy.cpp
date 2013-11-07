#include "objectproxy.h"
#include "cbus.h"

ObjectProxy::ObjectProxy(CBus &cBus, const QString &serviceName, const QString &objectName, QObject *parent) :
	Object(cBus, serviceName, objectName, parent)
{
}

PendingReply *ObjectProxy::doExecute(const QString &methodName, const QVariantList &parameters, bool expectReturnValue) const
{
	return getCBus().execute(getServiceName(), getObjectName(), methodName, parameters, expectReturnValue);
}

PendingReply *ObjectProxy::doExecute(const QString &methodName, const QVariant &parameter, bool expectReturnValue) const
{
	return getCBus().execute(getServiceName(), getObjectName(), methodName, parameter, expectReturnValue);
}

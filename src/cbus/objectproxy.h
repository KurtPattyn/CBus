#ifndef OBJECTPROXY_H
#define OBJECTPROXY_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>

#include "cbus.h"
#include "object.h"

class PendingReply;

//ObjectProxy is a unique reference to an object; it can refer to a locally registered object or to a remote object
//in the first case, the object is called directly, in the latter, a network request is made
class ObjectProxy:public Object
{
    Q_OBJECT
public:
    ObjectProxy(CBus &cBus, const QString &serviceName, const QString &objectName, QObject *pObject = 0);

protected:
    virtual PendingReply *doExecute(const QString &methodName, const QVariantList &parameters, bool expectReturnValue) const;
    virtual PendingReply *doExecute(const QString &methodName, const QVariant &parameter, bool expectReturnValue) const;
};

#endif // OBJECTPROXY_H

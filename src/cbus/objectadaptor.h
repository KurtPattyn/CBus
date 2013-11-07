#ifndef CBUSADAPTOR_H
#define CBUSADAPTOR_H

#include <QObject>
#include <QString>
#include "object.h"

class SignalDispatcher;
class CBus;

class Service:public QObject
{
    Q_OBJECT
public:
    Service(CBus &cBus, const QString &serviceName, QObject *parent = 0);

    const QString &getServiceName() const;
    CBus &getCBus() const;

Q_SIGNALS:
    void registered();

private Q_SLOTS:
    void onCBusRegistered();

private:
    CBus &m_cBus;
    QString m_serviceName;
};

class ObjectAdaptor:public Object
{
    Q_OBJECT
public:
    ObjectAdaptor(CBus &cBus, const QString &serviceName, const QString &objectName, QObject *innerObject, QObject *parent = 0);

Q_SIGNALS:
    void ready();

protected:
    void onCBusConnected();
    PendingReply *doExecute(const QString &methodName, const QVariant &arguments, bool expectReturnValue) const;
    PendingReply *doExecute(const QString &methodName, const QVariantList &arguments, bool expectReturnValue) const;

private Q_SLOTS:
    void onReady();

private:
    QObject *m_pInnerObject;
    SignalDispatcher *m_pSignalDispatcher;
};

#endif // CBUSADAPTOR_H

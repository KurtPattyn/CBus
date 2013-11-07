#ifndef SAYHELLOSERVICE_H
#define SAYHELLOSERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>
#include <QDebug>
#include "messagebus.h"
#include "sayhellotest.h"
#include "objectproxy.h"

//using namespace messagebus::utility;
//using namespace messagebus::functor;

class SayHelloService : public QObject
{
	Q_OBJECT
public:
	explicit SayHelloService(MessageBus &messageBus, QObject *parent = 0);

Q_SIGNALS:
	void registered();

public Q_SLOTS:
	void onConnected();
	void onCPUStatus(QVariantMap vm);
	void onCPUStatuses(QVariantList vm);

private Q_SLOTS:
	void onRegistered();

	void onSayHelloReceived(QString message);

private:
	MessageBus &m_messageBus;
	QString m_serviceName;
	SayHelloTest m_sayHelloTest;
};

#endif // SAYHELLOSERVICE_H

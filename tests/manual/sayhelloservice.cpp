#include "sayhelloservice.h"
#include <QJsonObject>
#include <QStringList>
#include <QDebug>

SayHelloService::SayHelloService(MessageBus &messageBus, QObject *parent) :
	QObject(parent),
	m_messageBus(messageBus),
	m_serviceName("com.barco.cirrus.sayhelloservice"),
	m_sayHelloTest()
{
	connect(&m_messageBus, SIGNAL(connected(QString)), this, SLOT(onConnected()));
}

void SayHelloService::onConnected()
{
	PendingReply *pPendingReply = m_messageBus.registerService(m_serviceName);
	connect(pPendingReply, &PendingReply::ready, function()
	{
		PendingReply *pPendingReply = m_messageBus.registerObject(m_serviceName, "sayHelloTest", &m_sayHelloTest);

		//when coming here the messagebus is connected, but the service is not registered yet, and so the signal does not work
		connect(pPendingReply, &PendingReply::ready, this, &SayHelloService::onRegistered);
	});
}

void SayHelloService::onCPUStatus(QVariantMap vm)
{
	qDebug() << "SayHelloService::CPUStatus received:" << vm;
	/*Q_FOREACH(QVariant v, vm)
	{
		qDebug() << v;
	}*/
}

void SayHelloService::onCPUStatuses(QVariantList vm)
{
	qDebug() << "SayHelloService::	CPUStatuses received:" << vm;
}

void SayHelloService::onRegistered()
{
	PendingReply *m_pPendingReply = m_messageBus.getServices();
	/*connect(m_pPendingReply, &PendingReply::ready, function() {
		qDebug() << m_pPendingReply->getResult().toList();
	});*/
	m_pPendingReply->onReady(function()
	{
		qDebug() << m_pPendingReply->getResult().toList();
	});
	PendingReply *pPendingReply = m_messageBus.subscribeToSignal(m_serviceName, "sayHelloTest", "doSayHello", this, SLOT(onSayHelloReceived(QString)));
	/*connect(pPendingReply, &PendingReply::ready, function()
	{
		Q_EMIT m_sayHelloTest.doSayHello("'t Is feest");
	});*/
	pPendingReply->onReady(function()
	{
		Q_EMIT m_sayHelloTest.doSayHello("'t Is feest");
	});
	Q_EMIT registered();
}

void SayHelloService::onSayHelloReceived(QString message)
{
	qDebug() << "SayHelloService::Received sayhello signal from MessageBus; message=" << message;
}

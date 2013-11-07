#include "sayhellotest.h"
#include <QDebug>
#include <QMetaObject>

SayHelloTest::SayHelloTest(QObject *parent) :
	QObject(parent)
{
	setObjectName("sayHelloTest1");
	//connect(this, SIGNAL(doSayHello(QString)), this, SLOT(sayHello(QString)));
}

QString SayHelloTest::sayHello(QString message)
{
	//Q_EMIT doSayHello("Dispatched: " + message);
	qDebug() << "SayHelloTest::sayHello called: I say:" << message;
	//Q_EMIT doSayHello(QString("sayHelloCalledSignal: ") + message);
	return QString("SayHelloTest::sayHello called: I say:").append(message);
}

int SayHelloTest::add(int a, int b)
{
	qDebug() << "SayHelloTest::Called add" << a << "+" << b;
	return (a + b);
}

#include <QTime>
#include <QVariantList>
#include <QVariant>

QVariantList SayHelloTest::getSomeThings()
{
	return QVariantList() << "Coffee" << "Pizza" << 69 << QTime();
}


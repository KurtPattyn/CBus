#ifndef SAYHELLOTEST_H
#define SAYHELLOTEST_H

#include <QObject>
#include <QString>
#include <QVariantList>

class SayHelloTest : public QObject
{
	Q_OBJECT
public:
	explicit SayHelloTest(QObject *parent = 0);

Q_SIGNALS:
	void doSayHello(QString msg);

public Q_SLOTS:
	QString sayHello(QString msg);
	int add(int a, int b);
	QVariantList getSomeThings();
};

#endif // SAYHELLOTEST_H

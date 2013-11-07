#ifndef METHODCALLER_H
#define METHODCALLER_H

#include <QObject>
#include <QMetaMethod>
#include <QVariantList>
#include <QGenericArgument>

class MethodCaller:public QObject
{
	Q_OBJECT
public:
	MethodCaller(const QString &objectName, QObject *targetObject, QMetaMethod method);

public Q_SLOTS:
	QVariant onMethodCalled(QVariantList arguments);

private:
	QString m_objectName;
	QObject *m_pTargetObject;
	QMetaMethod m_method;

	QGenericArgument Arg(int idx, QVariantList arguments);
};

#endif // METHODCALLER_H

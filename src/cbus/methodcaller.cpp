#include "methodcaller.h"
#include <QGenericArgument>
#include <QMetaType>
#include <QMetaMethod>
#include <QVariantList>
#include <QDebug>

MethodCaller::MethodCaller(const QString &objectName, QObject *targetObject, QMetaMethod method) :
	QObject(targetObject),
	m_objectName(objectName),
	m_pTargetObject(targetObject),
	m_method(method)
{
}

QVariant MethodCaller::onMethodCalled(QVariantList arguments)
{
	QVariant returnValue;
	qDebug() << "MethodCaller::onMethodCalled: Method called for object" << m_objectName << "with arguments" << arguments;
	if (m_method.parameterCount() != arguments.length())
	{
		qDebug() << "MethodCaller::onMethodCalled: Invalid number of parameters for call; expected:" << m_method.parameterCount() << "got" << arguments.length();
	}
	else
	{
		if (m_method.returnType() == QMetaType::Void)	//method has no return value
		{
			m_method.invoke(m_pTargetObject,
							Arg(0, arguments),
							Arg(1, arguments),
							Arg(2, arguments),
							Arg(3, arguments),
							Arg(4, arguments),
							Arg(5, arguments),
							Arg(6, arguments),
							Arg(7, arguments),
							Arg(8, arguments));
		}
		else
		{
			void *retVal = QMetaType::create(m_method.returnType());
			if (retVal)
			{
				m_method.invoke(m_pTargetObject,
								QGenericReturnArgument(m_method.typeName(), retVal),
								Arg(0, arguments),
								Arg(1, arguments),
								Arg(2, arguments),
								Arg(3, arguments),
								Arg(4, arguments),
								Arg(5, arguments),
								Arg(6, arguments),
								Arg(7, arguments),
								Arg(8, arguments));
				returnValue = QVariant(m_method.returnType(), retVal);
				QMetaType::destroy(m_method.returnType(), retVal);
			}
			else
			{
				qDebug() << "MethodCaller::onMethodCalled:" << m_method.typeName() << "is a non-supported value type.";
			}
		}
	}
	return returnValue;
}

QGenericArgument MethodCaller::Arg(int idx, QVariantList arguments)
{
	QGenericArgument arg(0);
	if (idx < arguments.length())
	{
		int argType = m_method.parameterType(idx);
		if (arguments.at(idx).canConvert(argType))
		{
			arguments[idx].convert(argType);
			arg = QGenericArgument(QMetaType::typeName(argType), static_cast<const void *>(&arguments[idx]));
		}
	}
	return arg;
}

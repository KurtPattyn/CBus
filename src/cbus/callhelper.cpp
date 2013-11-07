#include "callhelper.h"
#include <QVariant>
#include <QMetaType>
#include <QGenericReturnArgument>
#include <QDebug>

namespace cbus
{
namespace utility
{
#define ARG(method, idx, argumentList) ((idx < argumentList.size()) && (idx < method.parameterCount())) ? Arg(method, idx, argumentList) : QGenericArgument(0)

QGenericArgument Arg(QMetaMethod method, int idx, QVariantList &arguments);

QVariantList Call(QObject *pTarget, QMetaMethod metaMethod, const QVariantList &arguments, bool sendParametersAsIs, bool *success)
{
    QVariantList returnValue;
    bool hasReturnValue = (metaMethod.returnType() != QMetaType::Void);
    QVariantList args = arguments;
    QGenericReturnArgument retVal;
    void *retType = 0;
    if (hasReturnValue)
    {
        retType = QMetaType::create(metaMethod.returnType());
        if (retType)
        {
            retVal = QGenericReturnArgument(metaMethod.typeName(), retType);
        }
        else
        {
            qDebug() << "Call:" << metaMethod.typeName() << "is a non-supported return value type.";
        }
    }

    if (sendParametersAsIs)
    {
        *success = metaMethod.invoke(pTarget,
                                     Qt::DirectConnection,
                                     retVal,
                                     Q_ARG(QVariantList, args));
    }
    else
    {
        *success = metaMethod.invoke(pTarget,
                                     Qt::DirectConnection,
                                     retVal,
                                     ARG(metaMethod, 0, args),
                                     ARG(metaMethod, 1, args),
                                     ARG(metaMethod, 2, args),
                                     ARG(metaMethod, 3, args),
                                     ARG(metaMethod, 4, args),
                                     ARG(metaMethod, 5, args),
                                     ARG(metaMethod, 6, args),
                                     ARG(metaMethod, 7, args),
                                     ARG(metaMethod, 8, args));

    }
    if (*success && hasReturnValue && retType)
    {
        if (metaMethod.returnType() != QMetaType::QVariant)
        {
            returnValue << QVariant(metaMethod.returnType(), retType);
        }
        else
        {
            returnValue << *((QVariant *)retType);
        }
    }
    if (retType)
    {
        QMetaType::destroy(metaMethod.returnType(), retType);
    }
    return returnValue;
}

QVariantList Call(QObject *pTarget, QMetaMethod metaMethod, const QVariant &argument, bool sendParametersAsIs, bool *success)
{
    QVariantList returnValue;
    bool hasReturnValue = (metaMethod.returnType() != QMetaType::Void);
    QGenericReturnArgument retVal;
    void *retType = 0;
    if (hasReturnValue)
    {
        retType = QMetaType::create(metaMethod.returnType());
        if (retType)
        {
            retVal = QGenericReturnArgument(metaMethod.typeName(), retType);
        }
        else
        {
            qDebug() << "Call:" << metaMethod.typeName() << "is a non-supported return value type.";
        }
    }

    if (sendParametersAsIs)
    {
        *success = metaMethod.invoke(pTarget,
                                     Qt::DirectConnection,
                                     retVal,
                                     Q_ARG(QVariant, argument));
    }
    else
    {
        QVariantList args = QVariantList() << argument;
        *success = metaMethod.invoke(pTarget,
                                     Qt::DirectConnection,
                                     retVal,
                                     ARG(metaMethod, 0, args));

    }
    if (*success && hasReturnValue && retType)
    {
        if (metaMethod.returnType() != QMetaType::QVariant)
        {
            returnValue << QVariant(metaMethod.returnType(), retType);
        }
        else
        {
            returnValue << *((QVariant *)retType);
        }
    }
    if (retType)
    {
        QMetaType::destroy(metaMethod.returnType(), retType);
    }
    return returnValue;
}

QGenericArgument Arg(QMetaMethod method, int idx, QVariantList &arguments)
{
    QGenericArgument arg(0);
    if ((idx < arguments.size()) && (idx < method.parameterCount()))
    {
        int argType = method.parameterType(idx);
        QVariant v = arguments[idx];
        bool ok = (argType == QMetaType::QVariant);
        if (!ok)
        {
            ok = arguments[idx].canConvert(argType) && arguments[idx].convert(argType);
        }
        if (ok)
        {
            arg = QGenericArgument(QMetaType::typeName(argType), arguments[idx].constData());
        }
        else
        {
            qDebug() << "Call::Arg: Cannot convert" << v << "to" << QMetaType::typeName(argType) << "in call to method" << method.methodSignature();
        }
    }
    return arg;
}

}	//end namespace utility
}	//end namespace cbus

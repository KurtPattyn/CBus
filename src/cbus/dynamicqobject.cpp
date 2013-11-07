#include "dynamicqobject.h"
#include <QStringList>
#include <QDebug>

bool DynamicQObject::connectDynamicSlot(const QString &objectName, QObject *pSourceObject, QMetaMethod signalMethod)
{
    QByteArray slotName = signalMethod.name().prepend("on").append("(");
    QStringList parameters;
    for (int i = 0, j = signalMethod.parameterCount(); i < j; ++i)
    {
        parameters << QMetaType::typeName(signalMethod.parameterType(i));
    }
    slotName.append(parameters.join(",").toUtf8()).append(")");
    QByteArray theSignal = QMetaObject::normalizedSignature(signalMethod.methodSignature().constData());
    QByteArray theSlot = QMetaObject::normalizedSignature(slotName);
    if (!QMetaObject::checkConnectArgs(theSignal, theSlot))
    {
        return false;
    }

    int signalId = pSourceObject->metaObject()->indexOfSignal(theSignal);
    if (signalId < 0)
    {
        return false;
    }

    int slotId = slotIndices.value(theSlot, -1);
    if (slotId < 0)
    {
        slotId = slotList.size();
        slotIndices[theSlot] = slotId;
        slotList.append(createSlot(theSlot, objectName, signalMethod));
    }

    return QMetaObject::connect(pSourceObject, signalId, this, slotId + metaObject()->methodCount());
}

bool DynamicQObject::connectDynamicSignal(const QString &signalName,
                                          QObject *pReceiver,
                                          char *member)
{
    QByteArray theSlot = QMetaObject::normalizedSignature(member);
    int slotId = pReceiver->metaObject()->indexOfSlot(theSlot);
    if (slotId < 0)
    {
        qDebug() << "DynamicQObject::connectDynamicSignal: slot" << member << "not found";
        return false;
    }

    QMetaMethod slotMethod = pReceiver->metaObject()->method(slotId);

    QByteArray theSignal = signalName.toLatin1();
    theSignal.append("(");
    QStringList parameters;
    for (int i = 0, j = slotMethod.parameterCount(); i < j; ++i)
    {
        parameters << QMetaType::typeName(slotMethod.parameterType(i));
    }
    theSignal.append(parameters.join(",").toUtf8()).append(")");
    if (!QMetaObject::checkConnectArgs(theSignal, theSlot))
    {
        return false;
    }

    int signalId = signalIndices.value(theSignal, -1);
    if (signalId < 0)
    {
        signalId = signalIndices.size();
        signalIndices[theSignal] = signalId;
    }

    return QMetaObject::connect(this, signalId + metaObject()->methodCount(), pReceiver, slotId);
}

int DynamicQObject::qt_metacall(QMetaObject::Call c, int id, void **arguments)
{
    id = QObject::qt_metacall(c, id, arguments);
    if ((id < 0) || (c != QMetaObject::InvokeMetaMethod))
    {
        return id;
    }
    Q_ASSERT(id < slotList.size());

    slotList[id]->call(sender(), arguments);
    return -1;
}

bool DynamicQObject::emitDynamicSignal(const QString &signalName, void **arguments)
{
    //QByteArray theSignal = QMetaObject::normalizedSignature(signal);
    int signalId = signalIndices.value(signalName.toLatin1(), -1);
    if (signalId >= 0)
    {
        QMetaObject::activate(this, metaObject(), signalId + metaObject()->methodCount(), arguments);
        return true;
    }
    else
    {
        return false;
    }
}

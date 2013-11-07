#ifndef DYNAMICQOBJECT_H
#define DYNAMICQOBJECT_H

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QMetaMethod>
#include <QObject>

class DynamicSlot
{
public:
    virtual void call(QObject *sender, void **arguments) = 0;
};

class DynamicQObject: public QObject
{
public:
    DynamicQObject(QObject *parent = 0) : QObject(parent) { }

    virtual int qt_metacall(QMetaObject::Call c, int id, void **arguments);

    bool emitDynamicSignal(const QString &signal, void **arguments);
    //connect the signal represented by signalMethod from the given object and objectName to this dynamic object
    bool connectDynamicSlot(const QString &objectName, QObject *pSourceObject, QMetaMethod signalMethod);
    bool connectDynamicSignal(const QString &signalName,
                              QObject *pReceiver, char *member);

protected:
    virtual DynamicSlot *createSlot(const QByteArray &slotName, const QString &objectName, QMetaMethod caller) = 0;

private:
    QHash<QByteArray, int> slotIndices;
    QList<DynamicSlot *> slotList;
    QHash<QByteArray, int> signalIndices;
};

#endif

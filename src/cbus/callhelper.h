#ifndef CALLHELPER_H
#define CALLHELPER_H

#include <QVariantList>
#include <QMetaMethod>
#include <QObject>
#include <QGenericArgument>

namespace cbus
{
namespace utility
{
QVariantList Call(QObject *pTarget, QMetaMethod metaMethod, const QVariantList &arguments, bool sendParametersAsIs, bool *success);
QVariantList Call(QObject *pTarget, QMetaMethod metaMethod, const QVariant &argument, bool sendParametersAsIs, bool *success);
}	//end namespace utility
}	//end namespace cbus

#endif // CALLHELPER_H

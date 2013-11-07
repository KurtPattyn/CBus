#ifndef OBJECT_H
#define OBJECT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include "functor.h"
#include "utility.h"
#include "cbus.h"
#include "pendingreply.h"

class Callback:public QObject
{
    Q_OBJECT
public:
    inline Callback(QObject *parent = 0) :
        QObject(parent)
    {}

Q_SIGNALS:
    void signalEmitted(QVariantList parameters);
};

class Object:public QObject
{
    Q_OBJECT

private:
    template <typename...Args>
    struct IsVariadicCall
    {
        enum { value = cbus::functor::FunctionChecker<typename cbus::utility::ListLast<cbus::utility::List<Args...>>::Value>::isFunction ? (sizeof...(Args) > 2) : (sizeof...(Args) > 1) };
    };
    template<typename Function>
    bool waitForReply(PendingReply *pPendingReply, Function f) const
    {
        bool isOK = pPendingReply->isOK();
        if (isOK)
        {
            pPendingReply->onReady(function()
            {
                cbus::functor::Functor<Function>(f).call(pPendingReply->getResult());
                //pPendingReply->deleteLater();
            });
            pPendingReply->onError([&isOK, pPendingReply]()
            {
                isOK = false;
                qDebug() << "Object::waitForReply: Got error:" << pPendingReply->getErrorString();
                //delete pPendingReply;
            });
        }
        else
        {
            delete pPendingReply;
        }
//		pPendingReply->waitForResult(1000);
//		bool isOK = pPendingReply->isOK();
//		if (isOK)
//		{
//			cbus::functor::Functor<Function>(f).call(pPendingReply->getResult());
//		}
//		else
//		{
//			qDebug() << "Object::waitForReply: Got error:" << pPendingReply->getErrorString();
//		}
        return isOK;
    }

    template <typename...Args>
    class ArgumentsHelper
    {
        typedef typename cbus::utility::ListLast<cbus::utility::List<Args...>>::Value CallbackType;

        template<typename ArgType, typename FirstArg>
        ArgType getLast(FirstArg &&first)
        {
            return first;
        }

        template<typename ArgType, typename FirstArg, typename ...RemainingArgs>
        ArgType getLast(FirstArg &&, RemainingArgs &&...remaining)
        {
            return getLast<ArgType>(remaining...);
        }

    public:
        enum { HasCallback = cbus::functor::FunctionChecker<CallbackType>::isFunction };

        ArgumentsHelper(Args &...args) :
            m_size(sizeof...(Args)),
            m_arguments(unroll(args...)),
            m_callback(getLast<CallbackType>(args...))
        {
        }

        inline int getSize() const
        {
            return m_size;
        }

        inline bool hasCallback() const
        {
            return HasCallback;
        }

        inline QVariantList getArguments() const
        {
            return m_arguments;
        }

        inline CallbackType getCallback() const
        {
            static_assert(cbus::functor::FunctionChecker<CallbackType>::isFunction, "Cannot retrieve callback, because the argument list contains no callback function");
            return m_callback;
        }

    private:
        const int m_size;
        QVariantList m_arguments;
        CallbackType m_callback;

        inline void doUnroll(QVariantList &) const {}

        template<typename LastArg>
        typename cbus::utility::enableIf<!cbus::functor::FunctionChecker<LastArg>::isFunction, void>::Type
        doUnroll(QVariantList &argumentList, const LastArg &lastArg) const
        {
            //if last element is not a function, it will be put in the list
            argumentList.append(QVariant(lastArg));
        }

        template<typename LastArg>
        typename cbus::utility::enableIf<cbus::functor::FunctionChecker<LastArg>::isFunction, void>::Type
        doUnroll(QVariantList &, const LastArg &) const
        {
            //if last element is a function, it will not be put in the list
        }

        template<typename FirstArg, typename ...RemainingArgs>
        void doUnroll(QVariantList &argumentList, const FirstArg &firstArg, const RemainingArgs &...remainingArgs) const
        {
            argumentList.append(QVariant(firstArg));
            doUnroll(argumentList, remainingArgs...);
        }

        template<typename...Params>
        QVariantList unroll(const Params &...args) const
        {
            QVariantList arguments;
            doUnroll(arguments, args...);
            return arguments;
        }
    };

public:
    Object(CBus &cBus, const QString &serviceName, const QString &objectName, QObject *parent = 0);

    template <typename Func>
    void on(const QString &signalName, Func f)
    {
        Callback *pCallback = new Callback(this);
        doConnect(signalName, pCallback, SIGNAL(signalEmitted(QVariantList)), true);
        QObject::connect(pCallback, &Callback::signalEmitted, function(QVariantList list) {
            cbus::functor::Functor<Func>(f).call(list);
        });
    }
    void on(const QString &signalName, QObject *pReceiver, const char *member);

    template <typename Func>
    bool call(const QString &methodName, const QVariantList &params, Func f) const
    {
        Q_ASSERT_X(cbus::functor::Functor<Func>(f).isValid(), "ObjectReference::call", "Supplied callback function is not valid");
        PendingReply *pr = doExecute(methodName, params, true);
        return waitForReply(pr, f);
    }

    bool call(const QString &methodName, const QVariantList &params) const;

    template <typename Func>
    bool call(const QString &methodName, const QVariant &param, Func f) const
    {
        Q_ASSERT_X(cbus::functor::Functor<Func>(f).isValid(), "ObjectReference::call", "Supplied callback function is not valid");
        PendingReply *pr = doExecute(methodName, param, true);
        return waitForReply(pr, f);
    }
    bool call(const QString &methodName, const QVariant &param) const;

    template <typename Func>
    typename cbus::utility::enableIf<cbus::functor::FunctionChecker<Func>::isFunction, bool>::Type
    call(const QString &methodName, Func f) const
    {
        Q_ASSERT_X(cbus::functor::Functor<Func>(f).isValid(), "ObjectReference::call", "Supplied callback function is not valid");
        return call<Func>(methodName, QVariant(), f);
    }

    bool call(const QString &methodName) const;

    template <typename...Args>
    typename cbus::utility::enableIf<IsVariadicCall<Args...>::value && ArgumentsHelper<Args...>::HasCallback, bool>::Type
    call(const QString &methodName, Args &&...args) const
    {
        ArgumentsHelper<Args ...> argums(args...);
        return call(methodName, argums.getArguments(), argums.getCallback());
    }

    template <typename...Args>
    typename cbus::utility::enableIf<IsVariadicCall<Args...>::value && !ArgumentsHelper<Args...>::HasCallback, bool>::Type
    call(const QString &methodName, Args &&...args) const
    {
        ArgumentsHelper<Args ...> argums(args...);
        return call(methodName, argums.getArguments());
    }

    CBus &getCBus() const;
    const QString &getServiceName() const;
    const QString &getObjectName() const;

protected:
    virtual PendingReply *doExecute(const QString &methodName, const QVariantList &arguments, bool expectReturnValue) const = 0;
    virtual PendingReply *doExecute(const QString &methodName, const QVariant &arguments, bool expectReturnValue) const = 0;

Q_SIGNALS:
    //void onSignalReceived(QVariantList args);

protected Q_SLOTS:
    virtual void onCBusConnected();

private:
    CBus &m_cBus;
    QString m_serviceName;
    QString m_objectName;

    void doConnect(const QString &signalName, QObject *pReceiver, const char *member, bool sendParametersAsIs = false) const;
    bool waitForReply(PendingReply *pPendingReply) const;
};

Q_DECLARE_METATYPE(Object *)

#endif // OBJECT_H

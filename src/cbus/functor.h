#ifndef FUNCTOR_H
#define FUNCTOR_H

#include "utility.h"
#include <QVariantList>
#include <QDebug>

namespace cbus
{
namespace functor
{
template<typename Func>
struct FunctionTrait
{
    enum { ArgumentCount = FunctionTrait<decltype(&Func::operator())>::ArgumentCount };
    typedef typename FunctionTrait<decltype(&Func::operator())>::Arguments Arguments;
    typedef typename FunctionTrait<decltype(&Func::operator())>::ReturnType ReturnType;
    typedef typename FunctionTrait<decltype(&Func::operator())>::ObjectType ObjectType;
    typedef Func FunctionType;
};

template <typename Obj, typename Ret, typename...Args>
struct FunctionTrait<Ret (Obj::*)(Args...)>
{
    enum { ArgumentCount = sizeof...(Args) };
    typedef utility::List<Args...> Arguments;
    typedef Ret ReturnType;
    typedef Obj ObjectType;
    typedef Ret (Obj::*FunctionType)(Args...);
};

template <typename Obj, typename Ret, typename...Args>
struct FunctionTrait<Ret (Obj::*)(Args...) const>
{
    enum { ArgumentCount = sizeof...(Args) };
    typedef utility::List<Args...> Arguments;
    typedef Ret ReturnType;
    typedef Obj ObjectType;
    typedef Ret (Obj::*FunctionType)(Args...) const;
};

template <typename Ret, typename...Args>
struct FunctionTrait<Ret (*)(Args...)>
{
    enum { ArgumentCount = sizeof...(Args) };
    typedef utility::List<Args...> Arguments;
    typedef Ret ReturnType;
    typedef Ret (*FunctionType)(Args...);
    typedef void ObjectType;
};

template <typename Ret, typename...Args>
struct FunctionTrait<Ret (&)(Args...)>
{
    enum { ArgumentCount = sizeof...(Args) };
    typedef utility::List<Args...> Arguments;
    typedef Ret ReturnType;
    typedef Ret (&FunctionType)(Args...);
    typedef void ObjectType;
};

template<typename Function>
struct FunctionChecker
{
    template <typename Func> static char test(decltype(&Func::operator()));
    template <typename Func> static long test(...);

    enum
    {
        isFunction = sizeof(test<Function>(0)) == sizeof(char),
        isLambda = isFunction && true,
        isStaticFunction = false,
        isMemberFunction = false,
        isConstFunction = false
    };
};

template<typename Obj, typename Ret, typename...Args>
struct FunctionChecker<Ret (Obj::*)(Args...)>
{
    enum
    {
        isFunction = true,
        isLambda = false,
        isStaticFunction = false,
        isMemberFunction = true,
        isConstFunction = false
    };
};

template<typename Obj, typename Ret, typename...Args>
struct FunctionChecker<Ret (Obj::*)(Args...) const>
{
    enum
    {
        isFunction = true,
        isLambda = false,
        isStaticFunction = false,
        isMemberFunction = true,
        isConstFunction = true
    };
};

template<typename Ret, typename...Args>
struct FunctionChecker<Ret (*)(Args...)>
{
    enum
    {
        isFunction = true,
        isLambda = false,
        isStaticFunction = true,
        isMemberFunction = false,
        isConstFunction = false
    };
};

template<typename Ret, typename...Args>
struct FunctionChecker<Ret (&)(Args...)>
{
    enum
    {
        isFunction = true,
        isLambda = false,
        isStaticFunction = true,
        isMemberFunction = false,
        isConstFunction = false
    };
};

template<typename Function>
struct Functor
{
private:
    typedef FunctionTrait<Function> FunctionTrait;
    typedef FunctionChecker<Function> FunctionChecker;

public:
    static_assert(FunctionChecker::isFunction, "Functor is constructed with non-function type.");

    enum
    {
        ArgumentCount = FunctionTrait::ArgumentCount,
        isLambda = FunctionChecker::isLambda,
        isMemberFunction = FunctionChecker::isMemberFunction,
        isStaticFunction = FunctionChecker::isStaticFunction,
        isConstFunction = FunctionChecker::isConstFunction
    };
    typedef typename FunctionTrait::Arguments Arguments;
    typedef typename FunctionTrait::ReturnType ReturnType;
    typedef typename FunctionTrait::FunctionType FunctionType;

    template<int index>
    struct Argument
    {
        typedef typename utility::ListAt<Arguments, index>::Value Value;
    };

    Functor() :
        m_functor(0),
        m_pObject(0)
    {}

    Functor(FunctionType f) :
        m_functor(f),
        m_pObject(0)
    {
        static_assert(!isMemberFunction, "Cannot initialize an object functor without an object pointer.");
    }

    Functor(typename FunctionTrait::ObjectType *obj, FunctionType f) :
        m_functor(f),
        m_pObject(obj)
    {
        static_assert(isMemberFunction, "Cannot initialize a non-object functor with an object pointer.");
    }

    bool isValid() const
    {
        //qDebug() << "isMemberFunction?"<< isMemberFunction << "valid functor?" << (((int *)*(void **)&m_functor) != 0);
        return (!isMemberFunction || (m_pObject != 0)) && (((int *)*(void **)&m_functor) != 0);
        //return (isMemberFunction && m_pObject != 0) && (m_functor != 0);
    }

    ReturnType call(const QVariantList &arguments)
    {
        //TODO: check if argument types match
        if (arguments.length() != ArgumentCount)
        {
            qDebug() << "Functor::call: Number of parameters does not match!";
            return ReturnType();
        }
        else
        {
            return callHelper(typename utility::MakeIndexes<Arguments>::Type(), arguments);
        }
    }

    ReturnType call(const QVariant &argument)
    {
        return call(QVariantList() << argument);
    }

    //the check sizeof >= 0 is necessary to have a dependency in enableIf to a template argument
    //otherwise SFINAE does not work
    template<typename...FunctionArgs>
    typename utility::enableIf<isMemberFunction && (sizeof...(FunctionArgs) >= 0), ReturnType>::Type
    call(FunctionArgs...args)
    {
        static_assert(ArgumentCount == sizeof...(args), "Invalid number of arguments supplied to function call.");
        ReturnType retVal = ReturnType();
        if (m_pObject)
        {
            retVal = (m_pObject->*m_functor)(args...);
        }
        return retVal;
    }

    //the check sizeof >= 0 is necessary to have a dependency in enableIf to a template argument
    //otherwise SFINAE does not work
    template<typename...FunctionArgs>
    typename utility::enableIf<!isMemberFunction && (sizeof...(FunctionArgs) >= 0), ReturnType>::Type
    call(FunctionArgs...args)
    {
        static_assert(ArgumentCount == sizeof...(args), "Invalid number of arguments supplied to function call.");
        return m_functor(args...);
    }

    ReturnType operator()(const QVariantList &arguments)
    {
        return call(arguments);
    }

    template<typename...FunctionArgs>
    ReturnType operator()(FunctionArgs...args)
    {
        return call(args...);
    }

private:
    FunctionType m_functor;
    typename FunctionTrait::ObjectType *m_pObject;
    bool m_isValid;

    template<int index, typename ArgList>
    struct IsCompatible
    {
        static bool isCompatible(const QVariantList &vl)
        {
            bool isCompat = QMetaType::QVariant == qMetaTypeId<typename ArgList::Car>();
            if (!isCompat)
            {
                QVariant v = vl.at(vl.size() - index);
                const char *typeName = v.typeName();
                isCompat = v.canConvert<typename ArgList::Car>() && v.convert(qMetaTypeId<typename ArgList::Car>());
                if (!isCompat)
                {
                    qDebug() << "Functor::IsCompatible:isCompatible" << QMetaType::typeName(qMetaTypeId<typename ArgList::Car>()) << "and" << typeName << "are not compatible";
                }
            }
            return isCompat && IsCompatible<index - 1, typename ArgList::Cdr>::isCompatible(vl);
        }
    };

    template<typename ArgList>
    struct IsCompatible<0, ArgList>
    {
        static bool isCompatible(const QVariantList &)
        {
            return true;
        }
    };

    inline bool isCompatible(const QVariantList &arguments)
    {
        bool compatible = false;
        compatible = (ArgumentCount == arguments.size());
        if (compatible)
        {
            compatible = IsCompatible<ArgumentCount, Arguments>::isCompatible(arguments);
        }
        else
        {
            qDebug() << "Functor::isCompatible: Invalid number of arguments supplied.";
        }
        return compatible;
    }

    template<int ...Indexes>
    inline ReturnType callHelper(utility::IndexTuple<Indexes...>, const QVariantList &arguments)
    {
        if (isCompatible(arguments))
        {
            return m_functor(qvariant_cast<typename Argument<Indexes>::Value>(arguments.at(Indexes))...);
        }
        else
        {
            return ReturnType();
        }
    }

};

} //end namespace functor
} //end namespace cbus
#endif // FUNCTOR_H

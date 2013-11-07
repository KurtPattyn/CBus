#ifndef UTILITY_H
#define UTILITY_H

namespace cbus
{
namespace utility
{
template<bool B, typename T = void>
struct enableIf
{};

template<typename T>
struct enableIf<true, T>
{
    typedef T Type;
};

template <typename...>
struct List
{
    enum { size = 0 };
};

template <typename Head, typename... Tail>
struct List<Head, Tail...>
{
    typedef Head Car;
    typedef List<Tail...> Cdr;
    enum { size = 1 + sizeof...(Tail) };
};

template <typename L, int N>
struct ListAt
{
    typedef typename ListAt<typename L::Cdr, N - 1>::Value Value;
};

template <typename L>
struct ListAt<L, 0>
{
    typedef typename L::Car Value;
};

template <typename L>
struct ListLast
{
    typedef typename ListAt<L, L::size - 1>::Value Value;
};

template<int...> struct IndexTuple{};

template<int i, typename IdxTuple, typename... Types>
struct MakeIndexesImpl;

template<int i, int... Indexes, typename T, typename ... Types>
struct MakeIndexesImpl<i, IndexTuple<Indexes...>, T, Types...>
{
    typedef typename MakeIndexesImpl<i + 1, IndexTuple<Indexes..., i>, Types...>::Type Type;
};

template<int i, int... Indexes>
struct MakeIndexesImpl<i, IndexTuple<Indexes...> >
{
    typedef IndexTuple<Indexes...> Type;
};

template<typename ... Types>
struct MakeIndexes : MakeIndexesImpl<0, IndexTuple<>, Types...>
{};
} //end namespace utility
} //end namespace cbus
#endif // UTILITY_H

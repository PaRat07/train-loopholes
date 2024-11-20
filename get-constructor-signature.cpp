#include "injector.h"

#include <print>
#include <utility>


template<typename NotTo, typename Uniquefy>
struct Caster {
    template<typename T, auto =
        Injector<
            std::conditional_t<std::is_same_v<T, NotTo>, TypeList<NotTo, void>, Uniquefy>{},
            TypeList<T>{}
        >{}
    > requires (!std::is_same_v<NotTo, T>)
    constexpr operator T();
};

template<typename T>
constexpr void InvocableWith(const T&) {}

template<typename ToTest, std::size_t ArgCnt>
consteval auto GetTypesForKnownAmount() {
    return [] <std::size_t... Inds> (std::index_sequence<Inds...>) {
        static_assert(requires {
            InvocableWith<ToTest>({ Caster<ToTest, TypeList<ToTest, Wrapper<Inds>>>{}... });
        });
        return TypeList<
                typename decltype([] <std::size_t Ind> (std::index_sequence<Ind>) {
                    return Magic(Getter<TypeList<ToTest, Wrapper<Inds>>{}>{});
                } (std::index_sequence<Inds>{}))::Type...
            >();
    } (std::make_index_sequence<ArgCnt>());
}

template<typename NotTo>
struct SimpleCaster {
    template <typename T> requires (!std::is_same_v<std::decay_t<NotTo>, std::decay_t<T>>)
    constexpr operator T&&();

    template <typename T> requires (!std::is_same_v<std::decay_t<NotTo>, std::decay_t<T>>)
    constexpr operator T&();
};




// USER CODE

struct T {
    T() = delete;
};

struct ToAsk {
    ToAsk(T) {}
};

template<typename... Ts>
void PrintTypes(TypeList<Ts...>) {
    std::println(__PRETTY_FUNCTION__);
}



int main() {
    // auto _ = Injector<TypeList<ToAsk, Wrapper<0>>{}, TypeList<ToAsk, TypeList<ToAsk, Wrapper<0>>>{}>{};
    // [] (ToAsk&&) {} ({ Caster<ToAsk, TypeList<ToAsk, Wrapper<0>>>{} });
    // auto _ = Injector<TypeList<ToAsk, Wrapper<0>>{}, TypeList<int, TypeList<ToAsk, Wrapper<0>>>{}>{};
    // ToAsk _ = { Caster<ToAsk, TypeList<ToAsk, Wrapper<0>>>{} };
    // static_assert(requires {
    //     InvocableWith<ToAsk>({Caster<ToAsk, Wrapper<0>>{}});
    // });
    // PrintTypes(Magic(Getter<Wrapper<0>{}>{}));
    PrintTypes(GetTypesForKnownAmount<ToAsk, 1>());
}
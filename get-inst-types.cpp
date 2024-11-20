#include <injector>

#include <print>
#include <utility>




template<typename Uniquefy>
struct Caster {
    template<typename T, auto = Injector<Uniquefy{}, TypeList<Uniquefy>{}>{}>
    operator T&();

    template<typename T, auto = Injector<Uniquefy{}, TypeList<Uniquefy>{}>{}>
    operator T&&();
};

template<typename ToTest, std::size_t ArgCnt>
consteval auto GetTypes() {
    return [] <std::size_t... Inds> (std::index_sequence<Inds...>) {
        std::ignore = ToTest(Caster<Wrapper<Inds>>{}...);

        return TypeList<
                typename decltype([] <std::size_t Ind> (std::index_sequence<Ind>) {
                    return Magic(Getter<Wrapper<Ind>{}>{});
                } (std::index_sequence<Inds>{}))::Type...
            >();
    } (std::make_index_sequence<ArgCnt>());
}

struct ToAsk {
    std::unique_ptr<int> _;

    ToAsk(int) {}
};

template<typename... Ts>
void PrintTypes(TypeList<Ts...>) {
    std::println(__PRETTY_FUNCTION__);
}


template<typename NotTo>
struct SimpleCaster {
    template<typename T> requires (!std::is_same_v<NotTo, T>)
    operator T&() { return *(T*)nullptr; }

    // template<typename T>
    // operator T&&() { return *reinterpret_cast<T*>(nullptr); }
};

int main() {
    ToAsk(SimpleCaster<ToAsk>{});
    // PrintTypes(GetTypes<ToAsk, 1>());
}
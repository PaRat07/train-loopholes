#include <print>
#include <utility>

#include "injector.h"


namespace impl {
template<typename Key, typename Uniquefy, std::size_t Ans = 0>
consteval std::size_t GetSetSize() {
  if constexpr (requires (Uniquefy) { Magic(Getter<TypeList<Key, Wrapper<Ans>>{}>{}); }) {
    return GetSetSize<Key, Uniquefy, Ans + 1>();
  } else {
    return Ans;
  }
}


template<typename Key, typename Value>
consteval void AddToSet() {
  if constexpr (!requires { Magic(Getter<TypeList<Key, Value>{}>{}); }) {
    std::ignore = Injector<TypeList<Key, Value>{}, GetSetSize<Key, Value>()>{};
    std::ignore = Injector<TypeList<Key, Wrapper<GetSetSize<Key, Value>()>>{}, TypeList<Value>{}>{};
  }
}


template<typename Key, std::size_t Ind>
consteval auto GetFromSet() {
  return Magic(Getter<TypeList<Key, Wrapper<Ind>>{}>{});
}


template<typename ForT>
struct InstancesSaver {
  template<typename T, auto = (AddToSet<ForT, std::decay_t<T>>(), 0)>
  constexpr void Method(T);
};
} // namespace impl


template<typename FuncT>
consteval auto GetInstantiatedTypes(FuncT func) {
  static_assert(requires { func.template operator()<impl::InstancesSaver<FuncT>>(); });
  return [] <std::size_t... Inds> (std::index_sequence<Inds...>) {
    return impl::TypeList<typename decltype(impl::GetFromSet<FuncT, Inds>())::Type...>{};
  } (std::make_index_sequence<impl::GetSetSize<FuncT, FuncT>()>{});
}


template<typename... Ts>
void PrintTypes(impl::TypeList<Ts...>) {
  std::println(__PRETTY_FUNCTION__);
}

constexpr int ConstexprFunc(auto x) {
  x.Method(1);
  x.Method('a');
  x.Method(x);
  return 52;
}

int main() {
  PrintTypes(GetInstantiatedTypes([]<typename T> {
    ConstexprFunc(T{});
  }));
}
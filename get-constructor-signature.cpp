#include "injector.h"

#include <print>
#include <utility>

namespace impl {
template<typename NotTo, typename Uniquefy>
struct Caster {
  template<typename T, auto =
  Injector<
      std::conditional_t<std::is_same_v<T, NotTo>, TypeList<NotTo, void>, Uniquefy>{},
      TypeList<T>{}
  >{}
  >
  requires (!std::is_same_v<NotTo, T>)
  constexpr operator T();
};

template<typename T>
constexpr void InvocableWith(const T &) {}

template<typename ToTest, std::size_t ArgCnt>
consteval auto GetTypesForKnownAmount() {
  return []<std::size_t... Inds>(std::index_sequence<Inds...>) {
    static_assert(requires {
      InvocableWith<ToTest>({Caster<ToTest, TypeList<ToTest, Wrapper<Inds>>>{}...});
    });
    return TypeList<
        typename decltype([]<std::size_t Ind>(std::index_sequence<Ind>) {
          return Magic(Getter<TypeList<ToTest, Wrapper<Inds>>{}>{});
        }(std::index_sequence<Inds>{}))::Type...
    >();
  }(std::make_index_sequence<ArgCnt>());
}

template<typename NotTo>
struct SimpleCaster {
  template<typename T>
  requires (!std::is_same_v<std::decay_t<NotTo>, std::decay_t<T>>)
  constexpr operator T &&();

  template<typename T>
  requires (!std::is_same_v<std::decay_t<NotTo>, std::decay_t<T>>)
  constexpr operator T &();
};

template<typename ToTest, std::size_t CurAns = 0, std::size_t... AmountHolder>
consteval std::size_t GetTypesAmount() {
  if constexpr (!requires {
    InvocableWith<ToTest>({(AmountHolder, SimpleCaster<ToTest>{})...});
  }) {
    return GetTypesAmount<ToTest, CurAns + 1, AmountHolder..., 0>();
  } else {
    return CurAns;
  }
}
} // namespace impl

template<typename ToTest>
consteval auto GetConstructorTypes() {
  static constexpr std::size_t args_amount = impl::GetTypesAmount<ToTest>();
  return impl::GetTypesForKnownAmount<ToTest, args_amount>();
}




// USER CODE

struct T {
  T() = delete;
};

struct ToAsk {
  ToAsk(T) {}
};

template<typename... Ts>
void PrintTypes(impl::TypeList<Ts...>) {
  std::println(__PRETTY_FUNCTION__);
}

int main() {
  // can print something like: void PrintTypes(impl::TypeList<Ts...>) [Ts = <T>]
  PrintTypes(GetConstructorTypes<ToAsk>());
}
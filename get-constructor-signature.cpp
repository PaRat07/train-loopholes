#include "injector.h"

#include <print>
#include <vector>
#include <ranges>
#include <utility>
#include <expected>
#include <optional>
#include <algorithm>

template<typename>
struct CasterTag {};

namespace impl {
template<typename LT, typename RT>
struct ImplMergeLists;

template<typename... LTs, typename... RTs>
struct ImplMergeLists<impl::TypeList<LTs...>, impl::TypeList<RTs...>> {
  using type = impl::TypeList<LTs..., RTs...>;
};

template<typename LT, typename RT>
using kMergeLists = typename ImplMergeLists<LT, RT>::type;

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
      InvocableWith<ToTest>({ Caster<ToTest, CasterTag<TypeList<ToTest, Wrapper<Inds>>>>{}... });
    });
    return TypeList<
        typename decltype([]<std::size_t Ind>(std::index_sequence<Ind>) {
          return Magic(Getter<CasterTag<TypeList<ToTest, Wrapper<Inds>>>{}>{});
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

template<typename Key, typename Val>
consteval std::size_t OrderOfKey() {
  return Magic(Getter<impl::TypeList<Key, Val>{}>{});
}

template<typename T, T... Vals>
auto ForEach(std::integer_sequence<T, Vals...>, auto func, auto merger) {
  return merger(func(Vals)...);
}

struct TypeIdTag {};

} // namespace impl

template<typename T, auto = (impl::AddToSet<impl::TypeIdTag, T>(), 0)>
constexpr std::size_t kTypeId = impl::OrderOfKey<impl::TypeIdTag, T>();

template<std::size_t Ind>
using kTypeById = typename decltype(impl::GetFromSet<impl::TypeIdTag, Ind>())::Type;

template<typename T>
constexpr auto kIndsOfTypesInList = [] <typename... Ts> (impl::TypeList<Ts...>) {
  return std::index_sequence<kTypeId<Ts>...>{};
} (T{});

template<typename In, typename What>
constexpr bool kIsSubList = [] <std::size_t... InIs, std::size_t... WhatIs> (std::index_sequence<InIs...>, std::index_sequence<WhatIs...>) {
  std::array<std::size_t, sizeof...(InIs)> inis_sorted = { InIs... };
  std::array<std::size_t, sizeof...(WhatIs)> whatis_sorted = { WhatIs... };
  std::ranges::sort(inis_sorted);
  std::ranges::sort(whatis_sorted);
  return std::ranges::includes(inis_sorted, whatis_sorted);
} (kIndsOfTypesInList<In>, kIndsOfTypesInList<What>);

template<auto In, auto What>
constexpr bool kIsSubListByObjects = kIsSubList<std::decay_t<decltype(In)>, std::decay_t<decltype(What)>>;

template<typename ToTest>
consteval auto GetConstructorTypes() {
  static constexpr std::size_t args_amount = impl::GetTypesAmount<ToTest>();
  return impl::GetTypesForKnownAmount<ToTest, args_amount>();
}

template<std::size_t TsCnt>
struct TypeInfo {
  std::size_t id;
  std::array<bool, TsCnt> depend_on;
};


template<typename... Ts>
consteval auto GetSortedTypes() {
  static_assert(requires { (kTypeId<Ts>, ...); });
  static constexpr std::tuple types_depths = {
    GetConstructorTypes<Ts>()...
  };
  return [] <std::size_t CurInd = 0, typename... HaveTypes> (this auto self) {
    if constexpr (CurInd != sizeof...(Ts)) {
      static constexpr std::size_t ind_of_first_can = [] <std::size_t cur_ind = 0> (this auto self_inside) {
        // static_assert((impl::TypeList<HaveTypes...>{}, 52, decltype(std::get<cur_ind>(types_depths)){}, sizeof...(HaveTypes) != 4 || cur_ind != 0));
        if constexpr (!kIsSubListByObjects<impl::TypeList<HaveTypes...>{}, std::get<cur_ind>(types_depths)> ||
                      kIsSubList<impl::TypeList<HaveTypes...>, impl::TypeList<Ts...[cur_ind]>>) {
          return self_inside.template operator()<cur_ind + 1>();
        } else {
          return cur_ind;
        }
      } ();
      return impl::kMergeLists<
                impl::TypeList<Ts...[ind_of_first_can]>,
                decltype(self.template operator()<CurInd + 1, HaveTypes..., Ts...[ind_of_first_can]>())
      >{};
    } else {
      return impl::TypeList<>{};
    }
  } ();
}


// USER CODE

struct T {
  T(int) {}
};

struct Z {
  Z(double) {}
};

struct ToAsk {
  ToAsk(const T&, const Z&) {}
};

template<typename... Ts>
void PrintTypes(impl::TypeList<Ts...>) {
  std::println(__PRETTY_FUNCTION__);
}

int main() {
  // can print something like: void PrintTypes(impl::TypeList<Ts...>) [Ts = <T>]
  // PrintTypes(GetConstructorTypes<T>());
  PrintTypes(GetSortedTypes<int, ToAsk, T, Z, double>());
}
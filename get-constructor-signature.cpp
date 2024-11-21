#include "injector.h"

#include <print>
#include <vector>
#include <ranges>
#include <utility>
#include <expected>
#include <optional>

template<typename>
struct CasterTag {};

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

template<typename ToTest>
consteval auto GetConstructorTypes() {
  static constexpr std::size_t args_amount = impl::GetTypesAmount<ToTest>();
  return impl::GetTypesForKnownAmount<ToTest, args_amount>();
}

struct TypeInfo {
  std::size_t id;
  std::vector<std::size_t> dependencies;
};


consteval bool HasCycles(std::size_t from,
                         const std::vector<std::vector<std::size_t>> &graph,
                         std::vector<bool> &used,
                         std::vector<bool> &processed,
                         std::vector<std::size_t> &ans) {
  if (used[from]) {
    return false;
  } else if (!processed[from]) {
    return true;
  }
  used[from] = true;
  processed[from] = false;
  for (std::size_t i : graph[from]) {
    if (HasCycles(i, graph, used, processed, ans)) {
      return true;
    }
  }
  processed[from] = true;
  ans.push_back(from);
  return false;
}

template<typename... Ts>
consteval std::array<TypeInfo, sizeof...(Ts)> GetTypeInfo() {
  return {
    TypeInfo {
      .id = kTypeId<Ts>,
      .dependencies = [] <typename... Dependencies> (impl::TypeList<Dependencies...>) {
        return std::vector<std::size_t> { kTypeId<Dependencies>... };
      } (GetConstructorTypes<Ts>())
  }...
};
}

template<std::size_t TsCnt>
std::optional<std::array<std::size_t, TsCnt>> TopoligicallySort(std::array<TypeInfo, TsCnt> ts_info) {
  std::vector<std::vector<std::size_t>> graph(ts_info.size());
  for (auto &[ind, val] : ts_info) {
    graph[ind] = val;
  }
  std::vector<bool> used(graph.size(), false);
  std::vector<bool> processed(graph.size(), true);
  std::vector<std::size_t> ans;
  for (std::size_t i : std::views::iota(0uz, graph.size())) {
    if (HasCycles(0, graph, used, processed, ans)) {
      return std::nullopt;
    }
  }
  std::array<std::size_t, TsCnt> ans_arr{};
  std::ranges::copy(ans, ans_arr.begin());

  return ans_arr;
}

template<typename... Types>
consteval auto GetSortedTypes() {
  static constexpr auto maybe_sorted_inds = TopoligicallySort(GetTypeInfo<Types...>());
  static_assert(maybe_sorted_inds.has_value(), "Cyclic dependency detected");
  static constexpr auto sorted_inds = *maybe_sorted_inds;
  // return sorted_inds;
  return [] <std::size_t... Inds> (std::index_sequence<Inds...>) {
    return impl::TypeList<Types...[sorted_inds[Inds]]...>{};
  } (std::make_index_sequence<sizeof...(Types)>{});
}

// USER CODE

struct T {
  T(int) {}
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
  std::print("{}", TopoligicallySort<3>(GetTypeInfo<int, T, ToAsk>).value());
}
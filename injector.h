#pragma once
#include <cstddef>

namespace impl {
template<auto>
struct Wrapper {};

template<template<typename> typename>
struct TemplWrapper {};

template<typename... Ts>
struct TypeList {
  template<std::size_t TInd>
  using IthType = Ts...[TInd];
};

template<typename T>
struct TypeList<T> {
  using Type = T;
};

template<typename... Ts>
consteval auto operator==(const TypeList<Ts...> &, const TypeList<Ts...> &) -> bool {
  return true;
};

template<typename... Ts, typename... TTs>
consteval auto operator==(const TypeList<Ts...> &, const TypeList<TTs...> &) -> bool {
  return false;
};

template<auto I>
struct Getter {
  friend constexpr auto Magic(Getter<I>);
};

template<auto I, auto Value>
struct Injector {
  friend constexpr auto Magic(Getter<I>) { return Value; };
};
} // namespace impl
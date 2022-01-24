
#ifndef _TRI_LIST_CONCEPTS_H_
#define _TRI_LIST_CONCEPTS_H_

#include <concepts>
#include <variant>
#include <ranges>

template <typename T1, typename T2, typename T3>
class tri_list;

// Concept that a modifier applied on the tri_list must satisfy.
template <typename F, typename T>
concept modifier = requires (F f, T t) {
  requires std::invocable<F, T>;
  {f(t)} -> std::convertible_to<T>;
};

// Implementation provided to students.
template <typename TriList, typename T1, typename T2, typename T3>
concept is_tri_list_valid = requires (TriList l) {
  {l} -> std::convertible_to<tri_list<T1, T2, T3>>;
  {l} -> std::ranges::viewable_range;
  {*l.begin()} -> std::same_as<std::variant<T1, T2, T3>>;
  {l.template range_over<T1>()} -> std::ranges::view;
};

#endif  // _TRI_LIST_CONCEPTS_H_

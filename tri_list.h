
#ifndef _TRI_LIST_H_
#define _TRI_LIST_H_

#include <functional>
#include <initializer_list>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <variant>
#include <concepts>

#include "tri_list_concepts.h"

// We are required to call tri_list methods only when the type parameter is one
// of the T{1,2,3} but not 0 nor more than 1. This concept makes sure of that.
template <typename T, typename T1, typename T2, typename T3>
concept one_type =
  (std::same_as<T, T1> && !std::same_as<T, T2> && !std::same_as<T, T3>) ||
  (std::same_as<T, T3> && !std::same_as<T, T2> && !std::same_as<T, T1>) ||
  (std::same_as<T, T2> && !std::same_as<T, T1> && !std::same_as<T, T3>);

// Compose two modifiers into a single one. Works just like standard function
// composition in maths ie. if the result of compose(f2, f1) would be applied on
// some x then it would evaluate to f2(f1(x)).
template <typename T, modifier<T> F1, modifier<T> F2>
inline auto compose(F2&& f2, F1&& f1)
{
  using namespace std::placeholders;
  return std::bind(std::move(f2), std::bind(std::move(f1), _1));
}

// Identity function for any type.
template <typename T>
inline T identity(const T& t)
{
  return t;
}

// A a collection that may store elements of 3 types: T1, T2 and T3.
template <typename T1, typename T2, typename T3>
class tri_list {
  // An alias for the variants stored by the list.
  using var_t = std::variant<T1, T2, T3>;

  // Alias for the basic modifier type for elements of type T. Useful for
  // accessing the tuple in which I keep the three modifiers.
  template <typename T>
  using mod_type = std::function<T(const T&)>;

  // Contents will be stored in a vector of variants.
  std::vector<var_t> contents;

  // Modifier for each type is kept here. Note that I use the previously defined
  // mod_type<T> alias as I will continue to use it with std::get on this tuple.
  std::tuple<mod_type<T1>, mod_type<T2>, mod_type<T3>> mods = {
    identity<T1>, identity<T2>, identity<T3>
  };

  // Accessing the type T's current modifier. It returns a reference so that
  // the result can be further modified. That's also why it cannot be const.
  template <typename T>
  mod_type<T>& get_mod()
  {
    return std::get<mod_type<T>>(mods);
  }

  // Same function but for const access (so that eg. range_over can be const).
  template <typename T>
  const mod_type<T>& get_mod() const
  {
    return std::get<mod_type<T>>(mods);
  }

public:
  // Constructor for an empty list.
  tri_list() : contents{} {}

  // Make a tri_list out of initialiser list of elements of types T1, T2 and T3.
  tri_list(std::initializer_list<var_t> init) : contents{init} {}

  // Add a new element of type T to the list.
  template <typename T>
  void push_back(const T& t) requires one_type<T, T1, T2, T3>
  {
    contents.push_back(t);
  }

  // Get a view of all of the elements of type T stored in the list.
  template <typename T>
  auto range_over() const requires one_type<T, T1, T2, T3>
  {
    return contents
      | std::views::filter(std::holds_alternative<T, T1, T2, T3>)
      | std::views::transform([this] (const var_t& v) -> T {
        return get_mod<T>()(std::get<T>(v));
      });
  }

  // Modify all elements of type T with a modifier m. It updates the appropriate
  // modifier from the mods tuple by composing its previous value with m.
  template <typename T, modifier<T> F>
  void modify_only(F m = F{}) requires one_type<T, T1, T2, T3>
  {
    auto& mod = get_mod<T>();
    mod = compose<T>(m, mod);
  }

  // Undo all modifications that were done on elements of type T. Do it by
  // overwriting the modifier for type T with identity<T>.
  template <typename T>
  void reset() requires one_type<T, T1, T2, T3>
  {
    get_mod<T>() = identity<T>;
  }

private:
  // An iterator, necessary for the begin() and end() methods. Based on the
  // contents vector's iterator but applies modifiers when dereferenced. Only
  // methods necessary for the input_iterator concept have been implemented.
  class tri_iterator : public std::vector<var_t>::iterator {
    const tri_list* tl;

  public:
    using value_type = var_t;

    tri_iterator(typename std::vector<var_t>::iterator it, const tri_list* tl)
      : std::vector<var_t>::iterator{it}, tl{tl} {}

    // Must be default-initialisable.
    tri_iterator() : std::vector<var_t>::iterator{}, tl{nullptr} {}

    // As in the base class iterator but apply a modifier upon visit.
    var_t operator*() const
    {
      var_t& elt = std::vector<var_t>::iterator::operator*();
      return std::visit([this] <typename T> (const T& t) -> var_t {
        const auto& mod = tl->get_mod<T>();
        return mod(t);
      }, elt);
    }

    tri_iterator& operator++()
    {
      std::vector<var_t>::iterator::operator++();
      return *this;
    }

    tri_iterator operator++(int)
    {
      tri_iterator tmp{*this};
      std::vector<var_t>::iterator::operator++();
      return tmp;
    }
  };

public:
  // Get the iterators.
  tri_iterator begin()
  {
    return tri_iterator{contents.begin(), this};
  }

  tri_iterator end()
  {
    return tri_iterator{contents.end(), this};
  }
};

#endif  // _TRI_LIST_H_

#ifndef _TRI_LIST_H_
#define _TRI_LIST_H_

#include <functional>
#include <numeric>
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
// of the T{1,2,3} but not 0 nor more than 1. This concept makes sure of that as
// converting anything into a variant with repeating types is ill-formed.
template <typename T, typename T1, typename T2, typename T3>
concept variantable = requires (T x) {
  {std::variant<T1, T2, T3>{x}};
};

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

// A collection that may store elements of 3 types: T1, T2 and T3.
template <typename T1, typename T2, typename T3>
class tri_list {
  // A shorthand for the variants stored by the list.
  using var_t = std::variant<T1, T2, T3>;

  // Alias for the lost of modifiers for elements of type T. Useful for
  // accessing the tuple in which the three lists are kept.
  template <typename T>
  using modlist_t = std::vector<std::function<T(const T&)>>;

  // List of modifiers for each type is kept here. Note the use of the
  // previously defined modlist_t<T> type as it will be used with std::get on
  // this tuple which greatly facilitates accessing these lists of modifiers.
  std::tuple<modlist_t<T1>, modlist_t<T2>, modlist_t<T3>> mods = {{}, {}, {}};

  // Accessing the type T's current list of modifiers. It returns a reference so
  // that it can be further modified. That's also why it cannot be const.
  template <typename T>
  modlist_t<T>& get_mods()
  {
    return std::get<modlist_t<T>>(mods);
  }

  // Same function but for const access (so that eg. range_over can be const).
  template <typename T>
  const modlist_t<T>& get_mods() const
  {
    return std::get<modlist_t<T>>(mods);
  }

  // Get a modifier for type T representing the composition of all modifiers
  // applied on tri list's elements of this type.
  template <typename T>
  std::function<T(const T&)> compose_mods() const
  {
    return std::accumulate(get_mods<T>().cbegin(), get_mods<T>().cend(),
      static_cast<std::function<T(const T&)>>(identity<T>),
      [] (auto&& f1, auto&& f2) { return compose<T>(f2, f1); });
  }

  // Contents of the list will be stored in a vector of variants.
  std::vector<var_t> contents;

public:
  // Constructor for an empty list.
  tri_list() : contents{} {}

  // Make a tri_list out of elements of types T1, T2 and T3.
  tri_list(std::initializer_list<var_t> init) : contents{init} {}

  // Add a new element of type T to the list.
  template <variantable<T1, T2, T3> T>
  void push_back(const T& t)
  {
    contents.push_back(t);
  }

  // Get a view of all of the elements of type T stored in the list with their
  // modifications (which are to be applied lazily).
  template <variantable<T1, T2, T3> T>
  auto range_over() const
  {
    return contents
      | std::views::filter(std::holds_alternative<T, T1, T2, T3>)
      | std::views::transform([this] (const var_t& v) -> T {
        return compose_mods<T>()(std::get<T>(v));
      });
  }

  // Lazily modify all elements of type T with a modifier m. It appends m to the
  // T's modifiers list. The modifiers will get actually composed and applied
  // upon each visitation of an element of this type.
  template <variantable<T1, T2, T3> T, modifier<T> F>
  void modify_only(F m = F{})
  {
    get_mods<T>().push_back(m);
  }

  // Get rid of all modifications that were applied on elements of type T.
  template <variantable<T1, T2, T3> T>
  void reset()
  {
    get_mods<T>() = {};
  }

private:
  // An iterator, necessary for the begin() and end() methods. Based on the
  // contents' vector iterator but applies modifiers when dereferenced. Only
  // methods necessary for the input_iterator concept have been implemented.
  class tri_iterator : public std::vector<var_t>::const_iterator {
    using base_it_t = typename std::vector<var_t>::const_iterator;

    // Keeping the tri_list this iterator refers to in order to use the freshest
    // versions of its modifiers.
    const tri_list* tl;

  public:
    using value_type = var_t;

    tri_iterator(base_it_t it, const tri_list* tl)
      : base_it_t{it}, tl{tl} {}

    // Must be default-initialisable.
    tri_iterator() : base_it_t{}, tl{nullptr} {}

    // As in the base class iterator but apply a modifier upon visit.
    var_t operator*() const
    {
      const var_t& elt = base_it_t::operator*();
      return std::visit([this] <typename T> (const T& t) -> var_t {
        return tl->compose_mods<T>()(t);
      }, elt);
    }

    tri_iterator& operator++()
    {
      base_it_t::operator++();
      return *this;
    }

    tri_iterator operator++(int)
    {
      tri_iterator tmp{*this};
      base_it_t::operator++();
      return tmp;
    }
  };

public:
  // Get appropriate iterators.
  tri_iterator begin() const
  {
    return tri_iterator{contents.cbegin(), this};
  }

  tri_iterator end() const
  {
    return tri_iterator{contents.cend(), this};
  }
};

#endif  // _TRI_LIST_H_

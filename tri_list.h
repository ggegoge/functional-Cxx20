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
  return std::bind(std::forward<F2>(f2), std::bind(std::forward<F1>(f1), _1));
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

  // Type of an identity function T -> T.
  template <typename T>
  using id_fn_t = std::function<T(const T&)>;

  // Alias for the lost of modifiers for elements of type T. Useful for
  // accessing the tuple in which the three lists are kept.
  template <typename T>
  using modlist_t = std::vector<id_fn_t<T>>;

  // List of modifiers for each type is kept here. Note the use of the
  // previously defined modlist_t<T> type as it will be used with std::get on
  // this tuple which greatly facilitates accessing these lists of modifiers.
  // It's mutable so that each modifier gets composed only once.
  mutable std::tuple<modlist_t<T1>, modlist_t<T2>, modlist_t<T3>> mods = {
    {}, {}, {}};

  // Accessing the type T's current list of modifiers. It returns a reference so
  // that it can be further modified. It may be const as the tuple is mutable.
  template <typename T>
  modlist_t<T>& get_mods() const
  {
    return std::get<modlist_t<T>>(mods);
  }

  // Get a modifier for type T representing the composition of all modifiers
  // applied on tri list's elements of this type. It then replaces the T's list
  // of modifiers with a one element list with the composed result compressing
  // the list (each modifier will be composed only once).
  template <typename T>
  id_fn_t<T> compose_mods() const
  {
    get_mods<T>() = {
      std::accumulate(get_mods<T>().cbegin(), get_mods<T>().cend(),
        static_cast<id_fn_t<T>>(identity<T>),
        [] (auto&& f1, auto&& f2) {
          return compose<T>(std::move(f2), std::move(f1));
        })
    };

    return get_mods<T>().front();
  }

  // Contents of the list will be stored in a vector of variants.
  std::vector<var_t> contents;

  // This function unpacks a variant, applies the modifiers on it and then
  // packs it into a variant once again.
  id_fn_t<var_t> var_modifier = [this] (const var_t& v) -> var_t {
    return std::visit([this] <typename T> (const T& t) -> var_t {
        return compose_mods<T>()(t);
      }, v);
  };

  // Type for a view over variants holding modified values from the list.
  // It is troubling it is done with decltype but I do not know the proper
  // type of such a view (how to write it manually).
  using var_view_t = decltype(contents | std::views::transform(var_modifier));

  // Keep a view with modifed list's contents.
  var_view_t modified = contents | std::views::transform(var_modifier);

public:
  // Constructor for an empty list.
  tri_list() {}

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
    return modified
      | std::views::filter(std::holds_alternative<T, T1, T2, T3>)
      | std::views::transform([] (const var_t& v) { return std::get<T>(v); });
  }

  // Lazily modify all elements of type T with a modifier m. It appends m to the
  // T's modifiers list. The modifiers will get actually composed and applied
  // upon a visitation of an element of this type.
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

  // Get begin and end of the modified list's view.
  auto begin() const
  {
    return std::ranges::begin(modified);
  }

  auto end() const
  {
    return std::ranges::end(modified);
  }
};

#endif  // _TRI_LIST_H_


#ifndef _TRI_LIST_H_
#define _TRI_LIST_H_

#include <functional>
#include <initializer_list>
#include <ranges>
#include <tuple>
#include <type_traits>
#include <vector>
#include <variant>
#include <concepts>

#include "tri_list_concepts.h"

// moze overkill? czy zamiast
// template< typename T>
// wewn klasy tri_list robić
// template <my_type<T1, T2, T3> T>?
template <typename T, typename T1, typename T2, typename T3>
concept my_type = std::same_as<T, T1> || std::same_as<T, T2>
                  || std::same_as<T, T3>;


// Compose two modifiers into a single one. Works just like standard function
// composition in maths ie. if the result of compose(f2, f1) would be applied
// on some x then it would evaluate to f2(f1(x)).
template <typename T, modifier<T> F1, modifier<T> F2>
inline auto compose(F2 f2, F1 f1)
{
  return [f1, f2] (T e) { return f2(f1(e)); };
}


// Identity function for any type.
template <typename T>
inline T identity(T e)
{
  return e;
}


// A a collection that may store elements of 3 types: T1, T2 and T3.
template <typename T1, typename T2, typename T3>
class tri_list {
  // An alias for the variants stored by the list.
  using var_t = std::variant<T1, T2, T3>;

  // Alias for the basic modifier for a given type.
  template <typename T>
  using mod_type = std::function<T(T)>;

  // Contents will be store in a vector of variants.
  std::vector<var_t> contents;

  // Modifier for each type is stored in this tuple.
  std::tuple<mod_type<T1>, mod_type<T2>, mod_type<T3>> mods = {
    identity<T1>, identity<T2>, identity<T3>
  };

  // Accessing the type T's current modifier. It returns a reference so that
  // the result can be further modified.
  template <typename T>
  mod_type<T>& get_mod()
  {
    return std::get<mod_type<T>>(mods);
  }

public:
  // Constructor for an empty list.
  tri_list() : contents() {}

  // Make a list out of given arguments in a initialiser list (all may have
  // different types).
  tri_list(std::initializer_list<var_t> init) : contents(init) {}

  // Add a new element of type T.
  template <typename T>
  void push_back(const T& t)
  {
    contents.push_back(var_t(t));
  }

  // Get a view of all of the elements of type T stored in the list.
  template <typename T>
  auto range_over()
  {
    auto filter = [] <typename E> (E) {
      return std::same_as<E, T>;
    };

    auto visit_filter = [filter] (const var_t & v) {
      return std::visit(filter, v);
    };

    auto mod = get_mod<T>();

    return contents
      | std::views::filter(visit_filter)
      | std::views::transform([mod] (const var_t & v) {
        return mod(std::get<T>(v));
      });
  }

  // Modify all elements of type T with a modifier m.
  template <typename T, modifier<T> F>
  void modify_only(F m = F{})
  {
    auto& mod = get_mod<T>();
    mod = compose<T>(m, mod);
  }

  // Undo all modifications that were done on elements of type T.
  template <typename T>
  void reset()
  {
    get_mod<T>() = identity<T>;
  }

private:
  // An iterator, necessary for the begin() and end() methods.
  struct tri_iterator : public std::vector<var_t>::iterator {
    using value_type = var_t;

    // UWAGA: chciałem tutaj referencję, ale wtedy miałem błąd, że
    // ill formed assignemnt operator... Zostawić pointer (dość ułatwia?)
    // czy jednak trzymać się referencji i nadpisywać coś?
    // potrzebuję obiektu "rodzica" by znać modifier...
    tri_list* tl;

    tri_iterator(typename std::vector<var_t>::iterator it, tri_list* tl)
      : std::vector<var_t>::iterator(it), tl(tl) {}

    tri_iterator() : std::vector<var_t>::iterator(), tl() {}

    var_t operator*() const
    {
      var_t& elt = std::vector<var_t>::iterator::operator*();
      return std::visit([this] <typename T> (T e) {
          auto mod = tl->get_mod<T>();
          return var_t(mod(e));
        }, elt);
    }

    var_t* operator->() const
    {
      return &operator*();
    }

    tri_iterator& operator++()
    {
      std::vector<var_t>::iterator::operator++();
      return *this;
    }

    tri_iterator& operator--()
    {
      std::vector<var_t>::iterator::operator--();
      return *this;
    }

    tri_iterator operator++(int)
    {
      tri_iterator tmp(*this);
      std::vector<var_t>::iterator::operator++();
      return tmp;
    }

    tri_iterator operator--(int)
    {
      tri_iterator tmp(*this);
      std::vector<var_t>::iterator::operator--();
      return tmp;
    }
  };

public:

  tri_iterator begin()
  {
    return tri_iterator(contents.begin(), this);
  }

  tri_iterator end()
  {
    return tri_iterator(contents.end(), this);
  }
};

#endif  // _TRI_LIST_H_

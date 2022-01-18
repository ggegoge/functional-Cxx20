
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

// moze overkill?
template <typename T, typename T1, typename T2, typename T3>
concept my_type = std::same_as<T, T1> || std::same_as<T, T2>
                  || std::same_as<T, T3>;


template <typename T, modifier<T> F1, modifier<T> F2>
inline auto compose(F2 f2, F1 f1)
{
  return [f1, f2] (T e) { return f2(f1(e)); };
}

template <typename T>
inline std::function<T(T)> identity()
{
  return [] (T e) { return e; };
}

template <typename T1, typename T2, typename T3>
class tri_list
{
  using var_t = std::variant<T1, T2, T3>;

  template <typename T>
  using mod_type = std::function<T(T)>;
  
  std::vector<var_t> contents;

  std::tuple<mod_type<T1>, mod_type<T2>, mod_type<T3>>
  mods = {
    identity<T1>(), identity<T2>(), identity<T3>()
  };

  template <typename T>
  mod_type<T>& get_mod()
  {
    return std::get<mod_type<T>>(mods);
  }
  
public:
  tri_list() : contents() {}

  tri_list(std::initializer_list<var_t> init) : contents(init) {}

  template <my_type<T1, T2, T3> T>
  // template <typename T>
  void push_back(const T& t)
  {
    contents.push_back(var_t(t));
  }

  template <my_type<T1, T2, T3> T>
  // template <typename T>
  auto range_over()
  {
    auto filter = [] <typename E> (E) {
      return std::same_as<E, T>;
    };

    auto visit_filter = [filter] (const var_t & v) {
      return std::visit(filter, v);
    };

    auto mod = get_mod<T>();
    
    return contents | std::views::filter(visit_filter)
      | std::views::transform([mod] (const var_t & v) {
        return mod(std::get<T>(v));
    });
  }

  template <typename T, modifier<T> F>
  void modify_only(F m = F{})
  {
    auto& mod = get_mod<T>();
    mod = compose<T>(m, mod);
  }

  template <typename T>
  void reset()
  {
    get_mod<T>() = identity<T>();
  }
};

#endif  // _TRI_LIST_H_

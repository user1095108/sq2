#ifndef SQ2_ITERATOR_HPP
# define SQ2_ITERATOR_HPP
# pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "sqlite3.h"

namespace sq2
{

namespace detail
{

inline decltype(auto) get(auto&& s) noexcept
  requires(std::is_pointer_v<std::remove_reference_t<decltype(s)>>)
{
  return std::forward<decltype(s)>(s);
}

inline decltype(auto) get(auto&& s) noexcept(noexcept(s.get()))
  requires(requires{s.get();})
{
  return s.get();
}

}

template <typename> struct tag{};

template <int I, typename T>
inline T user_deref(sqlite3_stmt* const s, tag<T>) noexcept
  requires(std::is_floating_point_v<T>)
{
  return sqlite3_column_double(s, I);
}

template <int I, typename T>
inline T user_deref(sqlite3_stmt* const s, tag<T>) noexcept
  requires(std::is_integral_v<T>)
{
  return sqlite3_column_int64(s, I);
}

template <int I, typename T>
inline T user_deref(sqlite3_stmt* const s, tag<T>) noexcept
  requires(std::is_same_v<T, char const*>)
{
  return reinterpret_cast<char const*>(sqlite3_column_text(s, I));
}

template <int I, typename T>
inline T user_deref(sqlite3_stmt* const s, tag<T>)
  requires(std::is_same_v<T, std::string>)
{
  return T(
    reinterpret_cast<char const*>(sqlite3_column_text(s, I)),
    sqlite3_column_bytes(s, I)
  );
}

template <int I, typename T>
inline T user_deref(sqlite3_stmt* const s, tag<T>) noexcept
  requires(std::is_same_v<T, std::string_view>)
{
  return T(
    reinterpret_cast<char const*>(sqlite3_column_text(s, I)),
    sqlite3_column_bytes(s, I)
  );
}

template <int I, typename ...A>
  requires(bool(sizeof...(A)) && !(std::is_reference_v<A> || ...))
class iterator
{
  sqlite3_stmt* s_;

public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::intmax_t;

  using value_type = std::conditional_t<
      (sizeof...(A) > 1),
      std::tuple<A...>,
      std::tuple_element_t<0, std::tuple<A...>>
    >;
  using reference = value_type const&;

public:
  iterator() = default;

  iterator(auto&& s) noexcept(noexcept(detail::get(s)))
    requires(!std::is_same_v<iterator, std::remove_cvref_t<decltype(s)>>):
    s_{detail::get(s)}
  {
    ++*this;
  }

  iterator(iterator const&) = default;
  iterator(iterator&&) = default;

  //
  iterator& operator=(iterator const&) = default;
  iterator& operator=(iterator&&) = default;

  //
  bool operator==(iterator const& o) const noexcept = default;

  // increment, decrement
  auto& operator++() noexcept
  {
    if (auto const r(sqlite3_step(s_)); SQLITE_ROW != r)
    {
      assert(SQLITE_DONE == r);
      s_ = {};
    }

    return *this;
  }

  void operator++(int) noexcept { ++*this; }

  // member access
  value_type operator*() const
  {
    if constexpr(sizeof...(A) > 1)
    {
      return [&]<int ...J>(std::integer_sequence<int, J...>)
        {
          return std::tuple<A...>{
              user_deref<I + J>(
                s_,
                tag<std::tuple_element_t<J, std::tuple<A...>>>{}
              )...
            };
        }(std::make_integer_sequence<int, sizeof...(A)>());
    }
    else
      return user_deref<0>(s_, tag<value_type>{});
  }
};

template <int I, typename ...A>
  requires(bool(sizeof...(A)) && !(std::is_reference_v<A> || ...))
class offset_range
{
  sqlite3_stmt* s_;

public:
  using iterator = sq2::iterator<I, A...>;

public:
  offset_range() = default;

  offset_range(auto&& s) noexcept
    requires(!std::is_same_v<offset_range, std::remove_cvref_t<decltype(s)>>):
    s_{detail::get(s)}
  {
  }

  offset_range(offset_range const&) = default;
  offset_range(offset_range&&) = default;

  //
  offset_range& operator=(offset_range const&) = default;
  offset_range& operator=(offset_range&&) = default;

  //
  bool operator==(offset_range const&) const = default;

  //
  auto begin() const noexcept { return iterator(s_); }
  auto end() const noexcept { return iterator(); }

  //
  auto clear_bindings() const noexcept { return sqlite3_clear_bindings(s_); }
  auto reset() const noexcept { return sqlite3_reset(s_); }
};

template <typename ...A> using range = offset_range<0, A...>;

}

#endif // SQ2_ITERATOR_HPP

#ifndef SQ2_ITERATOR_HPP
# define SQ2_ITERATOR_HPP
# pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <optional>
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
  requires(std::is_same_v<std::remove_cvref_t<decltype(s)>, sqlite3_stmt*>)
{
  return std::forward<decltype(s)>(s);
}

inline decltype(auto) get(auto&& s) noexcept(noexcept(s.get()))
  requires(requires{s.get();})
{
  return s.get();
}

}

template <int ...I>
using i = std::integer_sequence<int, I...>;

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

template <typename Tuple, int ...I>
class iterator
{
  template <typename, int ...> friend class iterator;

  sqlite3_stmt* s_;

public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::intmax_t;

  using value_type = std::conditional_t<
      (std::tuple_size_v<Tuple> > 1),
      Tuple,
      std::tuple_element_t<0, Tuple>
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
  bool operator==(iterator const&) const = default;

  template <int ...J>
  bool operator==(iterator<Tuple, J...> const& o) const noexcept
  {
    return s_ == o.s_;
  }

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

  void operator++(int) noexcept(noexcept(++*this)) { ++*this; }

  // member access
  auto operator*() const
  {
    if constexpr(sizeof...(I) > 1)
    {
      return std::tuple{
          user_deref<I>(
            s_,
            tag<std::tuple_element_t<I, Tuple>>{}
          )...
        };
    }
    else if constexpr(1 == sizeof...(I))
      return user_deref<(I, ...)>(s_, tag<value_type>{});
    else if constexpr(std::tuple_size_v<Tuple> > 1)
    {
      return [&]<int ...J>(std::integer_sequence<int, J...>)
        {
          return std::tuple{
              user_deref<J>(
                s_,
                tag<std::tuple_element_t<J, Tuple>>{}
              )...
            };
        }(std::make_integer_sequence<int, std::tuple_size_v<Tuple>>());
    }
    else
      return user_deref<0>(s_, tag<value_type>{});
  }

  //
  auto reset() noexcept
  {
    auto const r(sqlite3_reset(s_)); ++*this; return r;
  }
};

template <typename Tuple, int ...I>
class range
{
  template <typename, int ...> friend class range;

  sqlite3_stmt* s_;

public:
  range() = default;

  range(auto&& s) noexcept
    requires(!std::is_same_v<range, std::remove_cvref_t<decltype(s)>>):
    s_{detail::get(s)}
  {
  }

  range(range const&) = default;
  range(range&&) = default;

  //
  range& operator=(range const&) = default;
  range& operator=(range&&) = default;

  //
  bool operator==(range const&) const = default;

  template <int ...J>
  bool operator==(range<Tuple, J...> const& o) const noexcept
  {
    return s_ == o.s_;
  }

  //
  template <int ...J>
  auto begin() const noexcept { return iterator<Tuple, J...>(s_); }
  auto begin() const noexcept { return iterator<Tuple, I...>(s_); }
  auto end() const noexcept { return iterator<Tuple, I...>(); }

  //
  auto reset() const noexcept { return sqlite3_reset(s_); }
};

template <typename ...A>
  requires((sizeof...(A) > 0) && !(std::is_reference_v<A> || ...))
auto make_range(auto&& s) noexcept
{
  return range<std::tuple<A...>>(std::forward<decltype(s)>(s));
}

template <typename ...A, int ...I>
  requires((sizeof...(A) > 0) && !(std::is_reference_v<A> || ...))
auto make_range(auto&& s, std::integer_sequence<int, I...>) noexcept
{
  return range<std::tuple<A...>, I...>(std::forward<decltype(s)>(s));
}

}

#endif // SQ2_ITERATOR_HPP

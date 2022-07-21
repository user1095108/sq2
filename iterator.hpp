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

template <typename ...A>
class iterator
{
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::uintmax_t;

  using value_type = std::tuple<A...>;
  using pointer = value_type*;
  using reference = value_type&;

  sqlite3_stmt* s_;

public:
  iterator() = default;

  iterator(auto&& s) noexcept
    requires(
      !std::is_same_v<iterator, std::remove_cvref_t<decltype(s)>>
    )
  {
    if constexpr(requires{s.get();})
    {
      s_ = s.get();
    }
    else
    {
      s_ = s;
    }

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
    if (SQLITE_ROW != sqlite3_step(s_))
    {
      s_ = {};
    }

    return *this;
  }

  // member access
  auto operator*() const noexcept
  {
    using result_t = std::conditional_t<
      (sizeof...(A) > 1),
      std::tuple<A...>,
      std::tuple_element_t<0, std::tuple<A...>>
    >;

    return [&]<auto ...I>(std::index_sequence<I...>) noexcept
      {
        return result_t{
          [&]() noexcept
          {
            if constexpr(
              std::is_floating_point_v<
                std::tuple_element_t<I, std::tuple<A...>>
              >
            )
            {
              return sqlite3_column_double(s_, I);
            }
            else if constexpr(
              std::is_integral_v<
                std::tuple_element_t<I, std::tuple<A...>>
              >
            )
            {
              return sqlite3_column_int(s_, I);
            }
            else if constexpr(
              std::is_same_v<
                std::tuple_element_t<I, std::tuple<A...>>,
                std::string_view
              >
            )
            {
              return std::string_view(
                reinterpret_cast<char const*>(sqlite3_column_text(s_, I)),
                sqlite3_column_bytes(s_, I)
              );
            }
          }()...
        };
      }(std::make_index_sequence<sizeof...(A)>());
  }
};

template <typename ...A>
class range
{
  sqlite3_stmt* const s_;

public:
  range(auto&& s) noexcept requires(requires{s.get();}):
    s_{s.get()}
  {
  }

  range(sqlite3_stmt* const s) noexcept:
    s_{s}
  {
  }

  range(range const&) = default;
  range(range&&) = default;

  //
  range& operator=(range const&) = default;
  range& operator=(range&&) = default;

  //
  bool operator==(range const&) const noexcept = default;

  //
  auto begin() const noexcept { return iterator<A...>(s_); }
  auto end() const noexcept { return iterator<A...>(); }

  //
  auto reset() const noexcept { return sqlite3_reset(s_); }
};

}

#endif // SQ2_ITERATOR_HPP

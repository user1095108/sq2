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

template <int I, typename ...A>
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

    return [&]<auto ...J>(std::index_sequence<J...>) noexcept -> result_t
      {
        return {
          [&]() noexcept
          {
            if constexpr(
              std::is_floating_point_v<
                std::tuple_element_t<J, std::tuple<A...>>
              >
            )
            {
              return sqlite3_column_double(s_, I + J);
            }
            else if constexpr(
              std::is_integral_v<
                std::tuple_element_t<J, std::tuple<A...>>
              >
            )
            {
              return sqlite3_column_int64(s_, I + J);
            }
            else if constexpr(
              std::is_same_v<
                std::tuple_element_t<J, std::tuple<A...>>,
                std::string_view
              >
            )
            {
              return std::string_view(
                reinterpret_cast<char const*>(sqlite3_column_text(s_, I + J)),
                sqlite3_column_bytes(s_, I + J)
              );
            }
          }()...
        };
      }(std::make_index_sequence<sizeof...(A)>());
  }
};

template <int I, typename ...A>
class offset_range
{
  sqlite3_stmt* const s_;

public:
  offset_range(auto&& s) noexcept requires(requires{s.get();}):
    s_{s.get()}
  {
  }

  offset_range(sqlite3_stmt* const s) noexcept:
    s_{s}
  {
  }

  offset_range(offset_range const&) = default;
  offset_range(offset_range&&) = default;

  //
  offset_range& operator=(offset_range const&) = default;
  offset_range& operator=(offset_range&&) = default;

  //
  bool operator==(offset_range const&) const noexcept = default;

  //
  auto begin() const noexcept { return iterator<I, A...>(s_); }
  auto end() const noexcept { return iterator<I, A...>(); }

  //
  auto clear_bindings() const noexcept { return sqlite3_clear_bindings(s_); }
  auto reset() const noexcept { return sqlite3_reset(s_); }
};

template <typename ...A>
class range : public offset_range<0, A...>
{
public:
  using offset_range<0, A...>::offset_range;

  using offset_range<0, A...>::operator=;
  using offset_range<0, A...>::operator==;
};

}

#endif // SQ2_ITERATOR_HPP

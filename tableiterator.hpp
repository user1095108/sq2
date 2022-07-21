#ifndef SQ2_TABLEITERATOR_HPP
# define SQ2_TABLEITERATOR_HPP
# pragma once

#include "sqlite3.h"

namespace sq2
{

template <typename ...A>
class tableiterator
{
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;

  using value_type = std::tuple<A...>;
  using pointer = value_type*;
  using reference = value_type&;

  sqlite3_stmt* s_;

public:
  tableiterator() = default;

  tableiterator(auto&& s) noexcept
  {
    if constexpr(requires{s.get();})
    {
      s_ = s.get();
    }
    else
    {
      s_ = s;
    }

    if (SQLITE_ROW != sqlite3_step(s_))
    {
      s_ = {};
    }
  }

  tableiterator(tableiterator const&) = default;
  tableiterator(tableiterator&&) = default;

  //
  tableiterator& operator=(tableiterator const&) = default;
  tableiterator& operator=(tableiterator&&) = default;

  //
  bool operator==(tableiterator const& o) const noexcept = default;

  // increment, decrement
  auto& operator++() noexcept
  {
    if (auto const r(sqlite3_step(s_)); SQLITE_ROW != r)
    {
      s_ = {};
    }

    return *this;
  }

  // member access
  auto operator*() const noexcept
  {
    std::tuple<A...> t;

    gnr::apply(
      [&](auto&& ...a) noexcept
      {
        int i{};

        (
          [&](auto& a) noexcept
          {
            if constexpr(
              std::is_floating_point_v<
                std::remove_cvref_t<decltype(a)>
              >
            )
            {
              a = sqlite3_column_double(s_, i);
            }
            else if constexpr(
              std::is_integral_v<
                std::remove_cvref_t<decltype(a)>
              >
            )
            {
              a = sqlite3_column_int(s_, i);
            }
            else if constexpr(
              std::is_same_v<
                std::remove_cvref_t<decltype(a)>,
                std::string_view
              >
            )
            {
              a = std::string_view(
                reinterpret_cast<char const*>(sqlite3_column_text(s_, i)),
                sqlite3_column_bytes(s_, i)
              );
            }

            ++i;
          }(std::forward<decltype(a)>(a)),
          ...
        );
      },
      t
    );

    if constexpr(sizeof...(A) == 1)
    {
      return std::get<0>(t);
    }
    else
    {
      return t;
    }
  }
};

template <typename ...A>
struct range
{
  tableiterator<A...> b_;

  range(auto&& s) noexcept:
    b_(std::forward<decltype(s)>(s))
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
  auto& begin() const noexcept { return b_; }
  auto end() const noexcept { return decltype(b_)(); }
};

}

#endif // SQ2_TABLEITERATOR_HPP

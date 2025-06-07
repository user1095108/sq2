#ifndef SQ2_HPP
# define SQ2_HPP
# pragma once

#include "iterator.hpp"

namespace sq2
{

namespace detail
{

struct sqlite3_db_deleter final
{
  void operator()(sqlite3* const p) const noexcept { sqlite3_close_v2(p); }
};

struct sqlite3_stmt_deleter final
{
  void operator()(sqlite3_stmt* const p) const noexcept
  {
    sqlite3_finalize(p);
  }
};

}

using shared_db_t = std::shared_ptr<sqlite3>;

using unique_db_t = std::unique_ptr<
  sqlite3,
  detail::sqlite3_db_deleter
>;

using shared_stmt_t = std::shared_ptr<sqlite3_stmt>;

using unique_stmt_t = std::unique_ptr<
  sqlite3_stmt,
  detail::sqlite3_stmt_deleter
>;

template <int I>
inline auto user_bind(auto&& s, auto&& a) noexcept
  requires(std::is_same_v<std::remove_cvref_t<decltype(a)>, std::nullopt_t> ||
    std::is_same_v<std::remove_cvref_t<decltype(a)>, std::nullptr_t>)
{
  return sqlite3_bind_null(detail::get(s), I);
}

template <int I>
inline auto user_bind(auto&& s, auto&& a) noexcept
  requires(std::is_floating_point_v<std::remove_reference_t<decltype(a)>>)
{
  return sqlite3_bind_double(detail::get(s), I, a);
}

template <int I>
inline auto user_bind(auto&& s, auto&& a) noexcept
  requires(std::is_integral_v<std::remove_reference_t<decltype(a)>>)
{
  return sqlite3_bind_int64(detail::get(s), I, a);
}

template <int I, std::size_t N>
inline auto user_bind(auto&& s, char (&a)[N]) noexcept
{
  return sqlite3_bind_text64(detail::get(s), I, a, N - 1, SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

template <int I, std::size_t N>
inline auto user_bind(auto&& s, char const (&a)[N]) noexcept
{
  return sqlite3_bind_text64(detail::get(s), I, a, N - 1, SQLITE_STATIC,
    SQLITE_UTF8);
}

template <int I>
inline auto user_bind(auto&& s, auto&& a) noexcept
  requires(std::is_same_v<std::remove_reference_t<decltype(a)>, char*>)
{
  return sqlite3_bind_text64(detail::get(s), I, a, -1, SQLITE_TRANSIENT,
    SQLITE_UTF8);
}

template <int I>
inline auto user_bind(auto&& s, auto&& a) noexcept
  requires(std::is_same_v<std::remove_reference_t<decltype(a)>, char const*>)
{
  return sqlite3_bind_text64(detail::get(s), I, a, -1, SQLITE_STATIC,
    SQLITE_UTF8);
}

template <int I>
inline auto user_bind(auto&& s, auto&& a) noexcept
  requires(std::is_same_v<std::remove_cvref_t<decltype(a)>, std::string> ||
    std::is_same_v<std::remove_cvref_t<decltype(a)>, std::string_view>)
{
  return sqlite3_bind_text64(detail::get(s), I, a.data(), a.size(),
    SQLITE_TRANSIENT, SQLITE_UTF8);
}

template <int ...I> requires(sizeof...(I) > 0)
inline auto bind(auto&& s, auto&& ...a) noexcept
  requires(sizeof...(a) == sizeof...(I))
{
  int r;

  (
    [&]() noexcept -> bool
    {
      return (r = user_bind<I>(detail::get(s), std::forward<decltype(a)>(a)));
    }() || ...
  );

  return r;
}

template <int I>
inline auto bind(auto&& s, auto&& ...a) noexcept
  requires(sizeof...(a) > 1)
{
  return [&]<int ...J>(std::integer_sequence<int, J...>) noexcept
    {
      return bind<I + J...>(std::forward<decltype(s)>(s),
        std::forward<decltype(a)>(a)...);
    }(std::make_integer_sequence<int, sizeof...(a)>());
}

inline auto bind(auto&& s, auto&& ...a) noexcept
  requires(sizeof...(a) > 0)
{
  return bind<1>(std::forward<decltype(s)>(s),
    std::forward<decltype(a)>(a)...);
}

//
inline auto changes(auto&& db) noexcept
{
  return sqlite3_changes64(detail::get(db));
}

inline auto clear_bindings(auto&& s) noexcept
{
  return sqlite3_clear_bindings(detail::get(s));
}

inline auto column_count(auto&& s) noexcept
{
  return sqlite3_column_count(detail::get(s));
}

inline auto reset(auto&& s) noexcept { return sqlite3_reset(detail::get(s)); }
inline auto step(auto&& s) noexcept { return sqlite3_step(detail::get(s)); }

//
namespace detail
{

struct maker
{
  std::string_view const s_;

  auto exec(auto&& db) && noexcept
  {
    return sqlite3_exec(detail::get(db), s_.data(), {}, {}, {});
  }

  auto open_shared(int const fl = {}, char const* const zvfs = {}) && noexcept
  {
    sqlite3* db;

    return SQLITE_OK == sqlite3_open_v2(s_.data(), &db, fl, zvfs) ?
      shared_db_t(db, detail::sqlite3_db_deleter()) :
      (detail::sqlite3_db_deleter()(db), shared_db_t());
  }

  auto open_unique(int const fl = {}, char const* const zvfs = {}) && noexcept
  {
    sqlite3* db;

    return SQLITE_OK == sqlite3_open_v2(s_.data(), &db, fl, zvfs) ?
      unique_db_t(db) :
      (detail::sqlite3_db_deleter()(db), unique_db_t());
  }

  auto shared(auto&& db, unsigned const fl = {}) && noexcept
  {
    sqlite3_stmt* s;

    auto const r(sqlite3_prepare_v3(detail::get(db), s_.data(), s_.size(),
      fl, &s, {}));
    assert(SQLITE_OK == r);

    return SQLITE_OK == r ?
      shared_stmt_t(s, detail::sqlite3_stmt_deleter()) :
      shared_stmt_t();
  }

  auto unique(auto&& db, unsigned const fl = {}) && noexcept
  {
    sqlite3_stmt* s;

    auto const r(sqlite3_prepare_v3(detail::get(db), s_.data(), s_.size(),
      fl, &s, {}));
    assert(SQLITE_OK == r);

    return SQLITE_OK == r ? unique_stmt_t(s) : unique_stmt_t();
  }
};

}

namespace literals
{

inline auto operator ""_sq2(char const* const s, std::size_t const N)
  noexcept
{
  return detail::maker{{s, N}};
}

}

}

#endif // SQ2_HPP

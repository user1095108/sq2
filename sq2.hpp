#ifndef SQ2_HPP
# define SQ2_HPP
# pragma once

#include <cassert>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

#include "generic/invoke.hpp"

#include "tableiterator.hpp"

namespace sq2
{

namespace detail
{

struct sqlite3_db_deleter
{
  void operator()(sqlite3* const p) const noexcept
  {
    sqlite3_close_v2(p);
  }
};

struct sqlite3_stmt_deleter
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

namespace detail
{

struct maker
{
  std::string_view const s_;

  auto exec(auto&& db) && noexcept
  {
    if constexpr(requires{db.get();})
    {
      return sqlite3_exec(db.get(), s_.data(), nullptr, nullptr, nullptr);
    }
    else
    {
      return sqlite3_exec(db, s_.data(), nullptr, nullptr, nullptr);
    }
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

    if constexpr(requires{db.get();})
    {
      auto const r(sqlite3_prepare_v3(db.get(), s_.data(), s_.size(),
        fl, &s, {}));
      assert(SQLITE_OK == r);

      return SQLITE_OK == r ?
        shared_stmt_t(s, detail::sqlite3_stmt_deleter()) :
        shared_stmt_t();
    }
    else
    {
      auto const r(sqlite3_prepare_v3(db, s_.data(), s_.size(), fl, &s, {}));
      assert(SQLITE_OK == r);

      return SQLITE_OK == r ?
        shared_stmt_t(s, detail::sqlite3_stmt_deleter()) :
        shared_stmt_t();
    }
  }

  auto unique(auto&& db, unsigned const fl = {}) && noexcept
  {
    sqlite3_stmt* s;

    if constexpr(requires{db.get();})
    {
      auto const r(sqlite3_prepare_v3(db.get(), s_.data(), s_.size(), fl, &s,
        {}));
      assert(SQLITE_OK == r);

      return SQLITE_OK == r ? unique_stmt_t(s) : unique_stmt_t();
    }
    else
    {
      auto const r(sqlite3_prepare_v3(db, s_.data(), s_.size(), fl, &s, {}));
      assert(SQLITE_OK == r);

      return SQLITE_OK == r ? unique_stmt_t(s) : unique_stmt_t();
    }
  }
};

}

inline auto exec(auto&& stmt) noexcept
{
  if constexpr(requires{stmt.get();})
  {
    return sqlite3_step(stmt.get());
  }
  else
  {
    return sqlite3_step(stmt);
  }
}

inline auto exec(auto&& stmt, auto&& ...a) noexcept
{
  int i{}, r;

  sqlite3_stmt* s;

  if constexpr(requires{stmt.get();})
  {
    s = stmt.get();
  }
  else
  {
    s = stmt;
  }

  if (gnr::invoke_cond(
      [&](auto&& a) noexcept -> bool
      {
        ++i;

        if constexpr(
          std::is_same_v<
            std::remove_cvref_t<decltype(a)>,
            nullptr_t
          >
        )
        {
          return r = sqlite3_bind_null(s, i);
        }
        else if constexpr(
          std::is_floating_point_v<
            std::remove_cvref_t<decltype(a)>
          >
        )
        {
          return r = sqlite3_bind_double(s, i, a);
        }
        else if constexpr(
          std::is_same_v<
            std::remove_cvref_t<decltype(a)>,
            char*
          > ||
          std::is_same_v<
            std::remove_cvref_t<decltype(a)>,
            char const*
          >
        )
        {
          return r =
            sqlite3_bind_text64(s, i, a, -1, SQLITE_STATIC, SQLITE_UTF8);
        }
      },
      std::forward<decltype(a)>(a)...
    )
  )
  {
    return r;
  }
  else
  {
    return sqlite3_step(s);
  }
}

inline auto reset(auto&& s) noexcept
{
  if constexpr(requires{s.get();})
  {
    return sqlite3_reset(s.get());
  }
  else
  {
    return sqlite3_reset(s);
  }
}

inline auto rexec(auto&& s, auto&& ...a) noexcept
{
  return
    reset(std::forward<decltype(s)>(s)) ||
    exec(
      std::forward<decltype(s)>(s),
      std::forward<decltype(a)>(a)...
    );
}

namespace literals
{

inline auto operator "" _sq2(char const* const s, std::size_t const N)
  noexcept
{
  return detail::maker{{s, N}};
}

}

}

#endif // SQ2_HPP

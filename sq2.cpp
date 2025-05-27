#include <iostream>
#include <list>
#include <ranges>

#include "sq2.hpp"

using namespace sq2::literals;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

//////////////////////////////////////////////////////////////////////////////
namespace sq2
{

template <int I>
inline auto user_deref(sqlite3_stmt* const s, tag<char const*>) noexcept
{
  return reinterpret_cast<char const*>(sqlite3_column_text(s, I));
}

}

//////////////////////////////////////////////////////////////////////////////
void print_tuple(auto const& t)
{
  [&]<auto ...I>(std::index_sequence<I...>)
  {
    (
      [&]()
      {
        if constexpr(I)
        {
          std::cout << ", " << std::get<I>(t);
        }
        else
        {
          std::cout << std::get<I>(t);
        }
      }(),
      ...
    );

    std::cout << '\n';
  }
  (
    std::make_index_sequence<
      std::tuple_size_v<std::remove_cvref_t<decltype(t)>>
    >()
  );
}

//////////////////////////////////////////////////////////////////////////////
int main()
{
  auto const db(":memory:"_sq2.open_unique(
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    )
  );

  {
    auto s("SELECT ?/?"_sq2.unique(db));
    sq2::bind(s, 1., 3);
    std::cout << *sq2::range<double>(s).begin() << std::endl;

    s = "SELECT ?"_sq2.unique(db);
    sq2::bind(s, "test");
    std::cout << *sq2::range<std::string_view>(s).begin() << std::endl;
  }

//"DROP TABLE IF EXISTS COMPANY;"
  "CREATE TABLE COMPANY("
  "NAME           TEXT    NOT NULL,"
  "AGE            INT     NOT NULL,"
  "ADDRESS        CHAR(50),"
  "SALARY         REAL);"
  "INSERT INTO COMPANY (NAME,AGE,ADDRESS,SALARY)"
  "VALUES('Paul', 32, 'California', 20000.00);"
  "INSERT INTO COMPANY (NAME,AGE,ADDRESS,SALARY)"
  "VALUES('Allen', 25, 'Texas', 15000.00 );"
  "INSERT INTO COMPANY (NAME,AGE,ADDRESS,SALARY)"
  "VALUES('Teddy', 23, 'Norway', 20000.00 );"
  "INSERT INTO COMPANY (NAME,AGE,ADDRESS,SALARY)"
  "VALUES('Mark', 25, 'Rich-Mond ', 65000.00)"_sq2.exec(db);

  {
    auto const s("SELECT * FROM COMPANY"_sq2.unique(db));

    sq2::range<std::string, int, std::string, double> const r(s);
    std::cout << std::distance(r.begin(), {}) << std::endl;

    for (auto const& t: r) print_tuple(t);
    for (auto const& t: std::list<decltype(*r.begin())>{r.begin(), {}} |
      std::views::reverse) print_tuple(t);
  }

  {
    auto const s(
      "WITH RECURSIVE\n"
      "xaxis(x)AS(VALUES(-2.0)UNION ALL SELECT x+0.05 FROM xaxis WHERE x<1.2),"
      "yaxis(y)AS(VALUES(-1.0)UNION ALL SELECT y+0.1 FROM yaxis WHERE y<1.0),"
      "m(iter,cx,cy,x,y)AS("
      "SELECT 0,x,y,0.0,0.0 FROM xaxis,yaxis\n"
      "UNION ALL\n"
      "SELECT iter+1,cx,cy,x*x-y*y+cx,2.0*x*y+cy FROM m\n"
      "WHERE(x*x+y*y)<4.0 AND iter<28"
      "),"
      "m2(iter,cx,cy)AS("
      "SELECT max(iter),cx,cy FROM m GROUP BY cx,cy"
      "),"
      "a(t)AS("
      "SELECT group_concat(substr(' .+*#',1+min(iter/7,4),1),'')"
      "FROM m2 GROUP BY cy"
      ")"
      "SELECT group_concat(rtrim(t),x'0a')FROM a"_sq2.unique(db)
    );

    std::cout << *sq2::range<char const*>(s).begin() << std::endl;
  }

  return 0;
}

#include <iostream>

#include "sq2.hpp"

using namespace sq2::literals;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

//////////////////////////////////////////////////////////////////////////////
void print_tuple(auto&& t) noexcept
{
  [&]<auto ...I>(std::index_sequence<I...>) noexcept
  {
    (
      [&]()
      {
        if constexpr(I)
        {
          std::cout << ", ";
        }

        std::cout << std::get<I>(t);
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
  auto const db("example.db"_sq2.open_unique(
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    )
  );

  {
    auto s("SELECT ?/?"_sq2.unique(db));
    sq2::bind(s, 1., 3);
    std::cout << *sq2::range<double>(s).begin() << std::endl;

    s = "SELECT ?"_sq2.unique(db);
    sq2::bind(s, "lol");
    std::cout << *sq2::range<std::string_view>(s).begin() << std::endl;
  }

  "DROP TABLE IF EXISTS COMPANY;"
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

    sq2::range<std::string_view, int, std::string_view, double> const r(s);

    for (auto&& t: r) print_tuple(t);

    r.reset();
    std::cout << std::distance(r.begin(), r.end()) << std::endl;
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

    std::cout << *sq2::range<std::string_view>(s).begin() << std::endl;
  }

  return 0;
}

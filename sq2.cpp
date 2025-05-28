#include <iostream>
#include <list>
#include <ranges>

#include "sq2.hpp"

using namespace sq2::literals;
using namespace std::literals::string_literals;
using namespace std::literals::string_view_literals;

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
    auto s("SELECT * FROM COMPANY"_sq2.unique(db));

    sq2::range<std::string, int, std::string, double> r(s);
    std::cout << std::distance(r.begin(), {}) << std::endl;

    for (auto const& t: r) print_tuple(t);

    r = s = "SELECT *,ROW_NUMBER()OVER()AS rn\n"
      "FROM COMPANY ORDER BY rn DESC"_sq2.unique(db);
    for (auto const& t: r) print_tuple(t);
    //for (auto const& t: std::list<decltype(*r.begin())>{r.begin(), {}} |
    //  std::views::reverse) print_tuple(t);
  }

  {
    auto const s(
      "WITH RECURSIVE\n"
      "xaxis(x)AS(VALUES(-1.3)UNION ALL SELECT x+0.033 FROM xaxis WHERE x<1.3),"
      "yaxis(y)AS(VALUES(-1.15)UNION ALL SELECT y+0.096 FROM yaxis WHERE y<1.15),"
      "m(iter,cx,cy,x,y)AS("
      "SELECT 0,x,y,x,y FROM xaxis,yaxis\n"
      "UNION ALL\n"
      "SELECT iter+1,cx,cy,x*x-y*y+0.0,2.0*x*y-0.8 FROM m\n"
      "WHERE(x*x+y*y)<4.0 AND iter<28"
      ")"
      "SELECT group_concat(line, x'0a')FROM("
      "SELECT group_concat(ch, '') AS line FROM("
      "SELECT cy,substr(' .+*#', 1 + min(max(iter)/7, 4), 1) AS ch FROM m\n"
      "GROUP BY cx,cy ORDER BY cy DESC,cx ASC)"
      "GROUP BY cy)"_sq2.unique(db));

    std::cout << *sq2::range<std::string_view>(s).begin() << std::endl;
  }

  {
    auto const s(
      "WITH RECURSIVE\n"
      "fern(iter, x, y) AS ("
      "SELECT 0, 0.0, 0.0\n"
      "UNION ALL\n"
      "SELECT\n"
      "  iter + 1,\n"
      "  CASE\n"
      "    WHEN r < 0.01 THEN 0.0\n"
      "    WHEN r < 0.86 THEN 0.85 * x + 0.04 * y\n"
      "    WHEN r < 0.93 THEN 0.20 * x - 0.26 * y\n"
      "    ELSE -0.15 * x + 0.28 * y\n"
      "  END,\n"
      "  CASE\n"
      "    WHEN r < 0.01 THEN 0.16 * y\n"
      "    WHEN r < 0.86 THEN -0.04 * x + 0.85 * y + 1.6\n"
      "    WHEN r < 0.93 THEN 0.23 * x + 0.22 * y + 1.6\n"
      "    ELSE 0.26 * x + 0.24 * y + 0.44\n"
      "  END\n"
      "FROM fern, (SELECT abs(random() % 100) / 100.0 AS r)\n"
      "WHERE iter < 10000),"
      "scaled AS ("
      "SELECT\n"
      "  ROUND((x + 2.1820) / (2.6558 + 2.1820) * 79) AS x,"
      "  ROUND((9.9983 - y) / (9.9983 - 0.0) * 24) AS y\n"
      "FROM fern),"
      "dedup AS (SELECT DISTINCT x,y FROM scaled),\n"
      "xseq(x) AS ("
      "  SELECT 0\n"
      "  UNION ALL\n"
      "  SELECT x + 1 FROM xseq WHERE x < 79\n"
      "),"
      "yseq(y) AS ("
      "  SELECT 0\n"
      "  UNION ALL\n"
      "  SELECT y + 1 FROM yseq WHERE y < 24\n"
      "),"
      "grid AS ("
      "  SELECT\n"
      "    yseq.y,\n"
      "    xseq.x,\n"
      "    CASE\n"
      "      WHEN EXISTS (\n"
      "        SELECT 1 FROM dedup d\n"
      "        WHERE d.x = xseq.x AND d.y = yseq.y\n"
      "      ) THEN '#'\n"
      "      ELSE ' '\n"
      "    END AS ch\n"
      "  FROM yseq CROSS JOIN xseq\n"
      "),"
      "ordered_grid AS ("
      "  SELECT y, group_concat(ch, '') AS line\n"
      "  FROM(SELECT * FROM grid ORDER BY y, x)\n"
      "  GROUP BY y\n"
      ")"
      "SELECT group_concat(line, '\n') FROM ordered_grid"_sq2.unique(db));

    std::cout << *sq2::range<std::string_view>(s).begin() << std::endl;
  }


  return 0;
}

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#else
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include <iostream>
#include <list>
#include <ranges>

#include "sq2.hpp"

using namespace sq2::literals;
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
  int w, h;

  {
    #if defined(_WIN32)
      auto const handle(GetStdHandle(STD_OUTPUT_HANDLE));

      if (DWORD mode; GetConsoleMode(handle, &mode))
      {
        SetConsoleMode(handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
      }

      CONSOLE_SCREEN_BUFFER_INFO csbi;
      GetConsoleScreenBufferInfo(handle, &csbi);
      w = csbi.srWindow.Right - csbi.srWindow.Left + 1;
      h = csbi.srWindow.Bottom - csbi.srWindow.Top;
    #else
      struct winsize ws;
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

      w = ws.ws_col;
      h = ws.ws_row - 1;
    #endif
  }

  auto const db(":memory:"_sq2.open_unique(SQLITE_OPEN_READWRITE |
    SQLITE_OPEN_CREATE));

  {
    auto s("SELECT ?/?"_sq2.unique(db));
    sq2::bind(s, 1., 3);
    std::cout << *sq2::make_range<double>(s).begin() << std::endl;

    s = "SELECT ?"_sq2.unique(db);
    sq2::bind(s, "test");
    print_tuple(*sq2::make_range<std::string_view>(s).begin<0, 0, 0>());

    s = "SELECT random() & 9223372036854775807"_sq2.unique(db);

    auto j(sq2::make_range<std::string_view>(s).begin());

    for (unsigned i{}; i != 10; ++i, j.reset()) std::cout << *j << '\n';
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

    auto r(sq2::make_range<std::string_view, int, std::string_view, double>(s, sq2::i<2, 0>{}));
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
      "xaxis(x)AS(VALUES(-1.3)UNION ALL SELECT x+?1 FROM xaxis WHERE x<1.3),"
      "yaxis(y)AS(VALUES(-1.15)UNION ALL SELECT y+?2 FROM yaxis WHERE y<1.15),"
      "m(iter,cx,cy,x,y)AS("
      "SELECT 0,x,y,x,y FROM xaxis,yaxis\n"
      "UNION ALL\n"
      "SELECT iter+1,cx,cy,x*x-y*y+0.0,2.0*x*y-0.8 FROM m\n"
      "WHERE(x*x+y*y)<4.0 AND iter<28"
      ")"
      "SELECT group_concat(line, x'0d')FROM("
      "SELECT group_concat(ch, '') AS line FROM("
      "SELECT cy,substr(' .+*#', 1 + min(max(iter)/7, 4), 1) AS ch FROM m\n"
      "GROUP BY cx,cy ORDER BY cx)"
      "GROUP BY cy ORDER BY cy DESC)"_sq2.unique(db));

    sq2::bind<1, 2>(s, 2.6 / w, 2.3 / h);

    std::cout << *sq2::make_range<std::string_view>(s).begin() << std::endl;
  }

  {
    auto const s(
      "WITH RECURSIVE\n"
      "fern(iter, x, y) AS ("
      "SELECT 0, 0.0, 0.0\n"
      "UNION ALL\n"
      "SELECT\n"
      "  iter + 1,"
      "  CASE"
      "    WHEN r < 0.01 THEN 0.0"
      "    WHEN r < 0.86 THEN 0.85 * x + 0.04 * y"
      "    WHEN r < 0.93 THEN 0.20 * x - 0.26 * y"
      "    ELSE -0.15 * x + 0.28 * y"
      "  END,"
      "  CASE"
      "    WHEN r < 0.01 THEN 0.16 * y"
      "    WHEN r < 0.86 THEN -0.04 * x + 0.85 * y + 1.6"
      "    WHEN r < 0.93 THEN 0.23 * x + 0.22 * y + 1.6"
      "    ELSE 0.26 * x + 0.24 * y + 0.44"
      "  END\n"
      "FROM fern, (SELECT (random() & 9223372036854775807) / 9223372036854775807.0 r)"
      "WHERE iter < 10000),"
      "scaled(x, y) AS ("
      "SELECT"
      "  ROUND((x + 2.1820) / (2.6558 + 2.1820) * (?1 - 1)),"
      "  ROUND((9.9983 - y) / (9.9983 - 0.0) * (?2 - 1))"
      "FROM fern),"
      "dedup AS (SELECT DISTINCT x,y FROM scaled),"
      "xseq(x) AS (VALUES(0) UNION ALL SELECT x + 1 FROM xseq WHERE x < ?1 - 1),"
      "yseq(y) AS (VALUES(0) UNION ALL SELECT y + 1 FROM yseq WHERE y < ?2 - 1),"
      "grid AS ("
      "  SELECT"
      "    xseq.x,"
      "    yseq.y,"
      "    CASE"
      "      WHEN EXISTS ("
      "        SELECT 1 FROM dedup d"
      "        WHERE d.x = xseq.x AND d.y = yseq.y"
      "      ) THEN '#'"
      "      ELSE ' '"
      "    END AS ch"
      "  FROM xseq CROSS JOIN yseq"
      ")"
      "SELECT group_concat(line, '') FROM ("
      "  SELECT group_concat(ch, '') AS line"
      "  FROM(SELECT * FROM grid ORDER BY x, y)"
      "  GROUP BY y"
      ")"_sq2.unique(db));

    sq2::bind(s, w, h);

    std::cout << *sq2::make_range<std::string_view>(s).begin() << std::endl;
  }

  return 0;
}

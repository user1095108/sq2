#include <iostream>
#include <string_view>
#include <thread>

#include "sq2.hpp"

using namespace sq2::literals;

//////////////////////////////////////////////////////////////////////////////
int main()
{
  auto const db(":memory:"_sq2.open_unique(SQLITE_OPEN_READWRITE |
    SQLITE_OPEN_CREATE));

  auto s(
    "CREATE TABLE fb("
    "  x INTEGER,"
    "  y INTEGER,"
    "  v REAL,"
    "  PRIMARY KEY (x, y))"_sq2.unique(db)
  );

  sq2::step(s);

  s =
    "WITH RECURSIVE"
    "  xseq(x) AS (VALUES(0) UNION ALL SELECT x + 1 FROM xseq WHERE x < ?1 - 1),"
    "  yseq(y) AS (VALUES(0) UNION ALL SELECT y + 1 FROM yseq WHERE y < ?2 - 1),"
    "  grid(x, y, v) AS ("
    "    SELECT xseq.x, yseq.y, .0"
    "    FROM xseq CROSS JOIN yseq"
    "  )"
    "INSERT INTO fb SELECT * FROM grid"_sq2.unique(db);

  sq2::bind(s, 80, 25);
  sq2::step(s);

  for (;;)
  {
    s =
      "WITH RECURSIVE"
      "  xseq(x) AS (VALUES(0) UNION ALL SELECT x + 1 FROM xseq WHERE x < ?1 - 1),"
      "  grid(x, y, v) AS ("
      "    SELECT xseq.x, ?2 - 1, .8 + .2 * abs(random() / 9223372036854775807.0)"
      "    FROM xseq"
      "  )"
      "REPLACE INTO fb SELECT * from grid;"_sq2.unique(db);

    sq2::bind(s, 80, 25);
    sq2::step(s);

    s =
      "WITH RECURSIVE"
      "  xseq(x) AS (VALUES(0) UNION ALL SELECT x + 1 FROM xseq WHERE x < ?1 - 1),"
      "  yseq(y) AS (VALUES(0) UNION ALL SELECT y + 1 FROM yseq WHERE y < ?2 - 2),"
      "  grid(x, y, v) AS ("
      "    SELECT xseq.x, yseq.y,"
      "      MAX(MIN(("
      "        (SELECT v FROM fb WHERE fb.x=(xseq.x-1+?1) % ?1 AND fb.y=(yseq.y+1) % ?2) +"
      "        (SELECT v FROM fb WHERE fb.x=xseq.x AND fb.y=(yseq.y+1) % ?2) +"
      "        (SELECT v FROM fb WHERE fb.x=(xseq.x+1) % ?1 AND fb.y=(yseq.y+1) % ?2) +"
      "        (SELECT v FROM fb WHERE fb.x=xseq.x AND fb.y=(yseq.y+2) % ?2)"
      "      ) / (4.45 + 1.7 * (random() / 9223372036854775807.0)), 1), 0)"
      "    FROM xseq CROSS JOIN yseq"
      "  )"
      "REPLACE INTO fb SELECT * from grid;"_sq2.unique(db);

    sq2::bind(s, 80, 25);
    sq2::step(s);

    s =
      "SELECT group_concat(line, x'0a')FROM("
      "SELECT group_concat(ch, '') AS line FROM("
      "SELECT x,y,substr(' .:+*#', 1 + round(5 * v * v), 1) AS ch FROM fb)"
      "GROUP BY y ORDER BY y)"_sq2.unique(db);

    std::cout << *sq2::make_range<std::string_view>(s).begin();
    std::cout << "\x0d\x1b[24A" << std::flush;

    std::this_thread::sleep_for(std::chrono::milliseconds(33));
  }

  return 0;
}

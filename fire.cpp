#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#else
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h> // for STDOUT_FILENO
#endif

#include <iostream>
#include <string_view>
#include <thread>

#include "sq2.hpp"

using namespace sq2::literals;
using namespace std::literals::chrono_literals;

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
      h = csbi.srWindow.Bottom - csbi.srWindow.Top + 2;
    #else
      struct winsize ws;
      ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);

      w = ws.ws_col;
      h = ws.ws_row + 1;
    #endif
  }

  auto const db(":memory:"_sq2.open_unique(SQLITE_OPEN_READWRITE |
    SQLITE_OPEN_CREATE));

  {
    auto s(
      "CREATE TABLE fb("
      "  x INTEGER NOT NULL,"
      "  y INTEGER NOT NULL,"
      "  v REAL NOT NULL,"
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

    sq2::bind(s, w, h);
    sq2::step(s);
  }

  auto const replace_bottom(
    "WITH RECURSIVE"
    "  xseq(x) AS (VALUES(0) UNION ALL SELECT x + 1 FROM xseq WHERE x < ?1 - 1),"
    "  grid(x, y, v) AS ("
    "    SELECT xseq.x, ?2 - 1, min(max((random() & 9007199254740991) / 9007199254740991.0, .5), 1.)"
    "    FROM xseq"
    "  )"
    "REPLACE INTO fb SELECT * from grid;"_sq2.unique(db));

  sq2::bind(replace_bottom, w, h);

  auto const propagate(
    "WITH RECURSIVE"
    "  xseq(x) AS (VALUES(0) UNION ALL SELECT x + 1 FROM xseq WHERE x < ?1 - 1),"
    "  yseq(y) AS (VALUES(0) UNION ALL SELECT y + 1 FROM yseq WHERE y < ?2 - 2),"
    "  grid(x, y, v) AS ("
    "    SELECT xseq.x, yseq.y,"
    "      ("
    "        0.4 * (SELECT v FROM fb WHERE fb.x=(xseq.x-1+?1) % ?1 AND fb.y=(yseq.y+1) % ?2) +"
    "        2.2 * (SELECT v FROM fb WHERE fb.x=xseq.x AND fb.y=(yseq.y+1) % ?2) +"
    "        0.4 * (SELECT v FROM fb WHERE fb.x=(xseq.x+1) % ?1 AND fb.y=(yseq.y+1) % ?2) +"
    "        1 * (SELECT v FROM fb WHERE fb.x=xseq.x AND fb.y=(yseq.y+2) % ?2)"
    "      ) / (4.45 + 1.7 * (2 * ((random() & 9007199254740991) / 9007199254740991.0) - 1))"
    "    FROM xseq CROSS JOIN yseq"
    "  )"
    "REPLACE INTO fb SELECT * from grid;"_sq2.unique(db));

  sq2::bind(propagate, w, h);

  auto const render(
    "SELECT group_concat(line, '') FROM("
    "SELECT group_concat(ch, '' ORDER BY x) AS line FROM("
    "SELECT x,y,"
    "CASE WHEN y=?1-1 THEN '' ELSE substr(' ░▒▓█', 1 + round(4 * pow(max(min(v, 1.0), .0), 1.1)), 1) END AS ch FROM fb)"
    "GROUP BY y ORDER BY y)"_sq2.unique(db));

  sq2::bind(render, h);

  auto i(sq2::make_range<std::string_view>(render).begin());

  auto const reset_screen("\x0d\x1b[" + std::to_string(h - 2) + 'A');

  for (;;)
  {
    auto const frame_start(std::chrono::steady_clock::now());

    sq2::step(replace_bottom, propagate);

    std::cout << *i << reset_screen;

    i.reset();
    sq2::reset(replace_bottom, propagate);

    std::this_thread::sleep_until(frame_start + 33ms);
  }

  return 0;
}

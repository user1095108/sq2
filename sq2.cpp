#include <iostream>
#include <ranges>

#include "sq2.hpp"

using namespace sq2::literals;

//////////////////////////////////////////////////////////////////////////////
int main()
{
  auto const db("example.db"_sq2.open_unique(
      SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE
    )
  );

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
    auto const s1("SELECT * FROM COMPANY"_sq2.unique(db));

    for (auto&& t:
      sq2::range<std::string_view, int, std::string_view, double>(s1))
    {
      std::cout <<
        std::get<0>(t) << " " <<
        std::get<1>(t) << " " <<
        std::get<2>(t) << " " << 
        std::get<3>(t) << std::endl;
    }
  }

  return 0;
}

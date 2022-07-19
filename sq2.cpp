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
  "ID INT PRIMARY KEY     NOT NULL,"
  "NAME           TEXT    NOT NULL,"
  "AGE            INT     NOT NULL,"
  "ADDRESS        CHAR(50),"
  "SALARY         REAL);"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(1, 'Paul', 32, 'California', 20000.00);"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(2, 'Allen', 25, 'Texas', 15000.00 );"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(3, 'Teddy', 23, 'Norway', 20000.00 );"
  "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)"
  "VALUES(4, 'Mark', 25, 'Rich-Mond ', 65000.00)"_sq2.exec(db);

  return 0;
}

cmake_minimum_required(VERSION 3.1)

project(hurmadb-tests)

enable_testing()
add_test(NAME PgSqlServerTest
         COMMAND py.test -s -v ${CMAKE_SOURCE_DIR}/tests/PgSqlServerTest/PgSqlServerTest.py)

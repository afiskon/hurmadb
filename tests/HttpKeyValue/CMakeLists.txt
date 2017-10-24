cmake_minimum_required(VERSION 3.1)

project(hurmadb-tests)

add_test(NAME HttpKeyValue
         COMMAND py.test -s -v ${CMAKE_SOURCE_DIR}/tests/HttpKeyValue/TestHttpKeyValue.py)

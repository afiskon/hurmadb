cmake_minimum_required(VERSION 3.1)

project(hurmadb)

option(USE_GCOV "Create a GCov-enabled build." OFF)
if (USE_GCOV)
  set(GCC_COVERAGE_COMPILE_FLAGS "-fprofile-arcs -ftest-coverage")
  set(GCC_COVERAGE_LINK_FLAGS    "-lgcov")
endif()

set( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )
set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} ${GCC_COVERAGE_LINK_FLAGS}" )

find_package(PythonInterp REQUIRED)

find_library(PTHREAD_LIBRARY pthread)
find_library(PCRE_LIBRARY pcre)

include_directories(include)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror")

add_executable(hurmadb src/Main.cpp src/Socket.cpp
  src/Storage.cpp src/InMemoryStorage.cpp
  src/HttpServer.cpp src/HttpRequest.cpp src/HttpResponse.cpp)

target_link_libraries(hurmadb ${PTHREAD_LIBRARY} ${PCRE_LIBRARY})

enable_testing()
add_test(NAME python_tests
         COMMAND py.test -s ${CMAKE_SOURCE_DIR}/tests/run.py)

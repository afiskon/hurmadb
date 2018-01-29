# HurmaDB

Work in progress.

# How to build, test and run

Install all dependencies:

```
sudo apt-get install cmake g++ libpcre3-dev librocksdb-dev python3-pip \
  lcov wrk doxygen clang-format
```

Install and configure virtualenvwrapper:

```
sudo pip3 install virtualenv virtualenvwrapper
echo 'export VIRTUALENVWRAPPER_PYTHON=/usr/bin/python3' >> ~/.bashrc
echo 'source /usr/local/bin/virtualenvwrapper.sh' >> ~/.bashrc
bash
```

Create a virtualenv:

```
mkvirtualenv hurmadb
workon hurmadb
```

Install all Python dependencies into the virtualenv:

```
pip install -r requirements.txt
```

Build HurmaDB:

```
git clone https://github.com/afiskon/hurmadb.git
cd hurmadb
git submodule init
git submodule update
mkdir build
cd build
cmake ..
make -j2
```

Build the documentation:

```
doxygen
firefox doxygen/html/index.html
```

Run tests:

```
make test
```

Run a benchmark:

```
./hurmadb -h 8080
curl -XPUT -d '{"Some":"RandomData123"}' localhost:8080/v1/kv/123 -D - -o -
wrk -t10 -c10 -d10s http://localhost:8080/v1/kv/123
```

Run under Valgrind:

```
valgrind ./hurmadb -h 8080

workon hurmadb
HURMADB_HTTP_PORT=8080 make test
curl -XPUT http://localhost:8080/v1/_stop

# make sure the result is something like:

...
==14339== LEAK SUMMARY:
==14339==    definitely lost: 0 bytes in 0 blocks
==14339==    indirectly lost: 0 bytes in 0 blocks
==14339==      possibly lost: 0 bytes in 0 blocks
==14339==    still reachable: 739 bytes in 8 blocks <--- C++ runtime, ignore!
==14339==         suppressed: 0 bytes in 0 blocks
==14339== Rerun with --leak-check=full to see details of leaked memory
==14339==
==14339== For counts of detected and suppressed errors, rerun with: -v
==14339== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```

Run a static analysis:

```
cppcheck ./src
cd build-clang
scan-build -o ./report make
```

Create a code coverage report:

```
./code-coverage.sh
```

# API
The HurmaDB API is described [here](API.md).

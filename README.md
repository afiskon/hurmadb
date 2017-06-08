# HurmaDB

Work in progress.

# How to build, test and run

Install all dependencies:

```
sudo apt-get install cmake g++ libpcre3-dev librocksdb-dev python3-pip lcov wrk
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
mkdir build
cd build
cmake ..
make -j2
```

Run tests:

```
make test
```

Run a benchmark:

```
./hurmadb 8080
curl -XPUT -d 'SomeRandomData123' localhost:8080/v1/kv/123 -D - -o -
wrk -t10 -c10 -d10s http://localhost:8080/v1/kv/123
```

Run under Valgrind:

```
valgrind ./hurmadb 8080
HURMADB_PORT=8080 make test

# make sure result is:
...
ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
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

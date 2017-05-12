# HurmaDB

Work in progress.

# How to build, test and run

Install dependencies required to run tests:

```
mkvirtualenv hurmadb
pip install -r requirements.txt
```

Build HurmaDB:

```
mkdir build
cd build
cmake -G Ninja ..
# for production environment:
# cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
ninja
```

Run tests:

```
ninja test
```

Run a benchmark:

```
./hurmadb 8080
curl -XPUT -d 'SomeRandomData123' localhost:8080/v1/kv/123 -D - -o -
wrk -t10 -c10 -d10s http://localhost:8080/v1/kv/123
```

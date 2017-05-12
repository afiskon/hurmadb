# HurmaDB

Work in progress.

How to build, test and run:

```
# Install dependencies required to run tests

mkvirtualenv hurmadb
pip install -r requirements.txt

# Build HurmaDB

mkdir build
cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
ninja

# Run test

ninja test

# Run HurmaDB

./hurmadb 8080

# Run a benchmark

curl -XPUT -d 'SomeRandomData123' localhost:8080/v1/kv/123 -D - -o -
wrk -t10 -c10 -d10s http://localhost:8080/v1/kv/123
```



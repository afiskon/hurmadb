#!/bin/sh

set -e
rm -r cov || true
mkdir cov
cd cov
cmake -DUSE_GCOV=ON ..
make
# make test
echo ">>> Starting HurmaDB"
./hurmadb -h 8080 &
sleep 2 # just to be safe
echo ">>> Running tests..."
HURMADB_HTTP_PORT=8080 make test
echo ">>> Stopping HurmaDB"
curl -XPUT http://localhost:8080/v1/_stop
sleep 5 # give some time to write .gcda files
lcov --directory . --capture --output-file summary.info
mkdir report
genhtml -o ./report summary.info
echo 'DONE! See ./cov/report/index.html'

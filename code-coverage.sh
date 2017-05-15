#!/bin/sh

set -e
rm -r cov || true
mkdir cov
cd cov
cmake -DUSE_GCOV=ON ..
make
make test
lcov --directory . --capture --output-file summary.info
mkdir report
genhtml -o ./report summary.info
echo 'DONE! See ./cov/report/index.html'

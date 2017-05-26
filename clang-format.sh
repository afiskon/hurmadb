#!/bin/sh

set -e

find ./{src,include}/ -type f | xargs clang-format --style=file -i

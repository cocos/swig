#!/bin/bash

# exit this script if any commmand fails
set -e



DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

sudo apt install libpcre2-dev cmake bison

cmake .. -DCMAKE_INSTALL_PREFIX="$DIR/install" -DCMAKE_C_FLAGS="-DPCRE2_STATIC" -DCMAKE_CXX_FLAGS="-DPCRE2_STATIC" -DPCRE2_INCLUDE_DIR=/usr/include -DPCRE2_LIBRARY=/usr/lib/x86_64-linux-gnu/libpcre2-8.a -DBISON_EXECUTABLE=/usr/bin/bison && cmake --build . --config Release --target install
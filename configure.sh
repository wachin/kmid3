#!/bin/bash
mkdir -p build
pushd build
cmake .. -DCMAKE_INSTALL_PREFIX=`kde-config --prefix` -DCMAKE_BUILD_TYPE=release
popd

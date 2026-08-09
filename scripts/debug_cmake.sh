#!/bin/bash
rm -rf build_debug
mkdir build_debug
cd build_debug
cmake .. -DCMAKE_BUILD_TYPE=Debug > ../cmake_log.txt 2>&1
echo "Exit Code: $?" >> ../cmake_log.txt

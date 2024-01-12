#!/bin/bash

cmake -B ../out -S . -G "Unix Makefiles" 
if [ $? -ne 0 ]; then
    echo "CMake configuration failed."
    exit 1
fi

cmake --build ../out --target run_unittests
#!/bin/bash

g++ -fPIC -shared -o libvim-no-escape.so \
    -O3 \
    src/first.cpp \
    -std=c++11 \
    -lX11 -ldl -lXi \
    -fdiagnostics-color=always 2>&1 | less

if [ ${PIPESTATUS[0]} -eq 0 ] ; then
    LD_PRELOAD="./libvim-no-escape.so" gvim
fi


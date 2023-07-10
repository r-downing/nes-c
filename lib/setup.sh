#!/bin/bash

git submodule update --recursive --init

cd cpputest
emcmake cmake .
emmake make

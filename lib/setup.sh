#!/bin/bash

mkdir -p cpputest/cpputest_build
cd cpputest/cpputest_build
emcmake cmake ..
emmake make

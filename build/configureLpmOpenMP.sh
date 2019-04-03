#!/bin/bash

rm -rf CMakeFiles/ CMakeCache.txt

SRC_ROOT=$HOME/lpmkokkos
EXTRA_ARGS=$1
export KO=$HOME/kokkos-openmp/gcc-4.8.5
export OMPI_CXX=g++

cmake \
-D CMAKE_BUILD_TYPE:STRING="RelWithDebInfo" \
-D CMAKE_CXX_FLAGS="-g -fopenmp" \
-D Kokkos_ROOT=$KO \
-D CMAKE_VERBOSE_MAKEFILE:BOOL=OFF \
$EXTRA_ARGS \
$SRC_ROOT

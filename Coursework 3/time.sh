#!/bin/bash

# make sure all is compiled
mingw32-make clean
mingw32-make all -j8

p=bin/gr1714/

echo "baseline"
echo ""
time (bin/make_world.exe 5000 0.1 1 | bin/step_world.exe 0.1 1000 1 > /dev/null)
echo ""

echo "baseline tbb"
echo ""
time (bin/make_world.exe 5000 0.1 1 | ${p}step_world_tbb.exe 0.1 1000 1 > /dev/null)
echo ""

echo "baseline opencl"
echo ""
time (bin/make_world.exe 5000 0.1 1 | ${p}step_world_v3_opencl.exe 0.1 1000 1 > /dev/null)
echo ""

echo "double buffered opencl"
echo ""
time (bin/make_world.exe 5000 0.1 1 | ${p}step_world_v4_double_buffered.exe 0.1 1000 1 > /dev/null)
echo ""

echo "packed properties opencl" # default is GTX 1050
echo ""
time (bin/make_world.exe 5000 0.1 1 | ${p}step_world_v5_packed_properties.exe 0.1 1000 1 > /dev/null)
echo ""

export HPCE_SELECT_PLATFORM=1 # CPU
export HPCE_SELECT_DEVICE=1 # i7 7700HQ
echo "packed properties opencl CPU"
echo ""
time (bin/make_world.exe 5000 0.1 1 | ${p}step_world_v5_packed_properties.exe 0.1 1000 1 > /dev/null)
echo ""

export HPCE_SELECT_DEVICE=0 # integrated graphics
echo "packed properties opencl integrated graphics"
echo ""
time (bin/make_world.exe 5000 0.1 1 | ${p}step_world_v5_packed_properties.exe 0.1 1000 1 > /dev/null)
echo ""
`cmake -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" .. && make -j8`
`cmake -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" .. && make -j8 &&  echo "" && ./make_world.exe | .//step_world_v3_opencl.exe > /dev/null`

    cd build && cmake -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" .. && make -j8 && echo "" && cd .. && ./build/make_world.exe | ./build/step_world_v3_opencl.exe > /dev/null

    cd bin && cmake -G "MinGW Makefiles" -DCMAKE_SH="CMAKE_SH-NOTFOUND" .. && make -j8 && echo "" && cd .. && bin/make_world.exe | bin/step_world_v3_opencl.exe > /dev/null

    make clean && make all -j8

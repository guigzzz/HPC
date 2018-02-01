    make tools -j8 \
    && cat w/random1024.bin | bin/run_network w/n512_00.bin:par_for_naive > w/random1024_par_for_naive.out \
    && cat w/random1024.bin | bin/run_network w/n512_00.bin:simple > w/random1024_simple.out \
    && diff w/random1024_par_for_naive.out w/random1024_simple.out
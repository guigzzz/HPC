#!/bin/bash

mingw32-make tools -j8 && echo ""

algs="simple par_for_atomic clustered par_for_clustered"

for alg in $algs
do
    echo $alg

    str=""
    for i in $(seq -f "%02g" 0 1)
    do
        str="$str w/n512_$i.bin:$alg"
    done
    time (cat w/random16M.bin | pv | bin/run_network $str > /dev/null)
done
#!/bin/bash
mingw32-make tools -j8 && echo ""

sizes="16 128 512 1024 4096"

algs="simple par_for_naive par_for_atomic clustered par_for_clustered"
echo ""
for size in $sizes
do
    echo "****************************"
    echo "layer size: $size"
    bin/generate_sparse_layer $size $size > w/layer_${size}_$size.bin
    dsize=$(bc <<< "16384*$size")
    echo "data size: $dsize"
    cat /dev/urandom | head -c $dsize > w/random${dsize}M.bin
    for alg in $algs
    do
        echo "algorithm: $alg"
        time (cat w/random${dsize}M.bin | bin/run_network w/layer_${size}_$size.bin:$alg > /dev/null)
    done
    echo "****************************"
    echo ""
done
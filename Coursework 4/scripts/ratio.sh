#!/bin/bash
mingw32-make tools -j8 && echo ""

outputs="16 128 512 1024 4096 16384"
input="1024"
algs="simple par_for_naive par_for_atomic clustered par_for_clustered"
echo ""
cat /dev/urandom | head -c 16777216 > w/random16M.bin
for o_size in $outputs
do
    echo "****************************"
    echo "input size: $o_size"
    bin/generate_sparse_layer $input $o_size > w/layer_${input}_$o_size.bin
    for alg in $algs
    do
        echo "algorithm: $alg"
        time (cat w/random16M.bin | bin/run_network w/layer_${input}_$o_size.bin:$alg > /dev/null)
    done
    echo "****************************"
    echo ""
done
#!/bin/bash

mingw32-make tools -j8

algs="par_for_naive par_for_atomic clustered par_for_clustered"
echo ""
num_test=100
test_size=1024

cat /dev/urandom | head -c $test_size > w/random$test_size.bin
bin/generate_sparse_layer 512 512 > w/n512_00.bin

for alg in $algs
do
    different=false
    cat w/random$test_size.bin | bin/run_network w/n512_00.bin:simple > /tmp/random${test_size}_simple.out 2> /dev/null
    for i in $(seq 0 $num_test)
    do
        cat w/random$test_size.bin | bin/run_network w/n512_00.bin:$alg > /tmp/random${test_size}_$alg.out 2> /dev/null
        res=$(diff /tmp/random${test_size}_$alg.out /tmp/random${test_size}_simple.out)
        # echo $res
        if [ "$res" != "" ] 
        then
            different=true
        fi
    done

    if [ "$different" = true ]
    then
        echo "FAIL: In at least one of $num_test tests $alg output different to reference"
    else
        echo "PASS: In $num_test tests, $alg output identical to reference"
    fi
done
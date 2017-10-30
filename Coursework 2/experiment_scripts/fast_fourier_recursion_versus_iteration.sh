#!/bin/bash
recur_ks=(1 1 64 64)
loop_ks=(1 128 1 128)
algo="hpce.gr1714.fast_fourier_transform_combined"
echo "recursion, loop, algo, allowedP, trueP, n, sentinel, time, " > dump_0_header.csv
for index in ${!recur_ks[*]}; 
do 
    rk=${recur_ks[$index]}
    lk=${loop_ks[$index]}
    echo "${rk}, ${lk}"
    export HPCE_FFT_RECURSION_K=$rk
    export HPCE_FFT_LOOP_K=$lk
    bin/time_fourier_transform ${algo} 0 30 "${rk} - ${lk}, " > dump_${rk}_${lk}_${algo}.csv
done

cat dump_*.csv > results/fast_fourier_recursion_versus_iteration.csv
rm dump_*
#!/bin/bash

KS="1 2 4 8 16 32 64"
echo "K, algo, allowedP, trueP, n, sentinel, time, " > dump_0_header.csv
algo="hpce.gr1714.fast_fourier_transform_taskgroup"
echo $algo
for K in $KS; 
do
    # Select the specific value of K and export to other programs
    export HPCE_FFT_RECURSION_K=${K}
    # Run the program with the chosen K, and save to dump_K.csv
    bin/time_fourier_transform ${algo} 0 30 "${HPCE_FFT_RECURSION_K}, " > dump_${HPCE_FFT_RECURSION_K}_${algo}.csv
done

cat dump_*.csv > fast_fourier_time_vs_recursion_k.csv
rm dump_*.csv
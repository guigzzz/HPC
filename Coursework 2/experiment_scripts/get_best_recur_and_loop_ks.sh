#!/bin/bash

recur_ks="1 2 4 8 16 32 64 "
loop_ks="16 "


echo "recursion, loop, algo, allowedP, trueP, n, sentinel, time, " > dump_0_header.csv
algo="hpce.gr1714.fast_fourier_transform_combined"
echo $algo
for rk in $recur_ks; 
do
    for lk in $loop_ks
    do
        # Select the specific value of K and export to other programs
        export HPCE_FFT_RECURSION_K=$rk
        export HPCE_FFT_LOOP_K=$lk
        # Run the program with the chosen K, and save to dump_K.csv
        bin/time_fourier_transform ${algo} 0 30 "${rk}, ${lk}, " > dump_${rk}_${lk}_${algo}.csv
    done
done

cat dump_*.csv > results/best_var_recur_k.csv
rm dump_*


recur_ks="16 "
loop_ks="1 2 4 8 16 32 64 128 256 512 1024"


echo "recursion, loop, algo, allowedP, trueP, n, sentinel, time, " > dump_0_header.csv
algo="hpce.gr1714.fast_fourier_transform_combined"
echo $algo
for rk in $recur_ks; 
do
    for lk in $loop_ks
    do
        # Select the specific value of K and export to other programs
        export HPCE_FFT_RECURSION_K=$rk
        export HPCE_FFT_LOOP_K=$lk
        # Run the program with the chosen K, and save to dump_K.csv
        bin/time_fourier_transform ${algo} 0 30 "${rk}, ${lk}, " > dump_${rk}_${lk}_${algo}.csv
    done
done

cat dump_*.csv > resultsbest_var_loop_k.csv
rm dump_*

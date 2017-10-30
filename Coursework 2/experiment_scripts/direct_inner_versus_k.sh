#!/bin/bash
KS=$(seq 1 16)

echo "K, algo, allowedP, trueP, n, sentinel, time, " > dump_0_header.csv

algo="hpce.gr1714.direct_fourier_transform_parfor_inner"
echo $algo
for K in $KS; 
do
    # Select the specific value of K and export to other programs
    export HPCE_DIRECT_INNER_K=${K}
    # Run the program with the chosen K, and save to dump_K.csv
    bin/time_fourier_transform ${algo} 0 30 "${HPCE_DIRECT_INNER_K}, " > dump_${HPCE_DIRECT_INNER_K}_${algo}.csv
done

cat dump_*.csv > results/direct_inner_versus_k.csv
rm dump_*.csv
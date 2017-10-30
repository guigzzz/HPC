#!/bin/bash
PS=$(seq 2 2 8)

echo "p, algo, allowedP, trueP, n, sentinel, time, " > dump_0_header.csv

algo="hpce.gr1714.direct_fourier_transform_parfor_outer"
echo $algo
for p in $PS; 
do
    bin/time_fourier_transform ${algo} ${p} 30 "${p}, " > dump_${p}_${algo}.csv
done

bin/time_fourier_transform "hpce.direct_fourier_transform" 0 30 "none, " > dump_ref.csv

cat dump_*.csv > direct_outer_time_versus_p.csv
# rm dump_*.csv
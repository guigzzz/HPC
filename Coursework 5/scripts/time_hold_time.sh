#!/bin/bash
make -j8
puzzle=${1:-hold_time}
echo "Using puzzle $puzzle..."

scales="50 100 150 200 500 1000 5000 10000 25000 50000 75000 100000 200000 300000 400000 500000"
#scales="50 100 150 200 500 1000"


for scale in $scales
do
    echo "scale: $scale"
    bin/create_puzzle_input "$puzzle" $scale 0 > w/hold_time_$scale.in
    echo ""
    echo "new"
    time cat w/hold_time_$scale.in | bin/execute_puzzle 0 0 > w/hold_time_$scale.out
    echo ""
    #echo "ref"
    #time cat w/hold_time_$scale.in | bin/execute_puzzle 1 0 > w/hold_time_$scale.ref.out
    #bin/compare_puzzle_output w/hold_time_$scale.in w/hold_time_$scale.ref.out w/hold_time_$scale.out 2
done

#!/bin/bash
make -j8
echo ""
scales=$(python -c 'for i in range(5, 100): print(int(50 * (i ** 2.5)))')

for scale in $scales
do
    echo ""
    echo "==> scale: $scale"
    bin/create_puzzle_input hold_time $scale 0 > w/hold_time_$scale.in
    echo ""
    cat w/hold_time_$scale.in | bin/execute_puzzle 0 2 > w/hold_time_$scale.out
    echo ""
    cat w/hold_time_$scale.in | bin/execute_puzzle 1 2 > w/hold_time_$scale.ref.out
    bin/compare_puzzle_output w/hold_time_$scale.in w/hold_time_$scale.ref.out w/hold_time_$scale.out 2
done

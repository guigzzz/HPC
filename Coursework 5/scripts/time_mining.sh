#!/bin/bash
make -j8
echo ""
scales=$(python -c 'for i in range(10, 100): print(int(10 * (i ** 1.5)))')

for scale in $scales
do
    echo ""
    echo "==> scale: $scale"
    bin/create_puzzle_input mining $scale 0 > w/mining_$scale.in
    echo ""
    cat w/mining_$scale.in | bin/execute_puzzle 0 2 > w/mining_$scale.out
    echo ""
    cat w/mining_$scale.in | bin/execute_puzzle 1 2 > w/mining_$scale.ref.out
    bin/compare_puzzle_output w/mining_$scale.in w/mining_$scale.ref.out w/mining_$scale.out 2
done
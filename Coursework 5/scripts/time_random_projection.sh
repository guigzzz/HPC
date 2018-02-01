#!/bin/bash
make -j8
echo ""
scales=$(python -c 'for i in range(10, 100): print(int(10 * (i ** 1.2)))')

for scale in $scales
do
    echo ""
    echo "==> scale: $scale"
    bin/create_puzzle_input random_projection $scale 0 > w/random_projection_$scale.in
    echo ""
    cat w/random_projection_$scale.in | bin/execute_puzzle 0 2 > w/random_projection_$scale.out
    echo ""
    cat w/random_projection_$scale.in | bin/execute_puzzle 1 2 > w/random_projection_$scale.ref.out
    bin/compare_puzzle_output w/random_projection_$scale.in w/random_projection_$scale.ref.out w/random_projection_$scale.out 2
done
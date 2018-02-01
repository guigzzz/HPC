#!/bin/bash
make -j8
echo ""
scales=$(python -c 'for i in range(3, 100): print(int(10 * (i ** 2)))')

for scale in $scales
do
    echo ""
    echo "==> scale: $scale"
    bin/create_puzzle_input edit_distance $scale 0 > w/edit_distance_$scale.in
    echo ""
    cat w/edit_distance_$scale.in | bin/execute_puzzle 0 2 > w/edit_distance_$scale.out
    echo ""
    cat w/edit_distance_$scale.in | bin/execute_puzzle 1 2 > w/edit_distance_$scale.ref.out
    bin/compare_puzzle_output w/edit_distance_$scale.in w/edit_distance_$scale.ref.out w/edit_distance_$scale.out 2
done

#!/bin/bash
make -j8
echo ""
scales=$(python -c 'for i in range(5, 100): print(int(10 * (i ** 1.5)))')

for scale in $scales
do
    echo ""
    echo "==> scale: $scale"
    bin/create_puzzle_input heat_world $scale 0 > w/heat_world_$scale.in
    echo ""
    cat w/heat_world_$scale.in | bin/execute_puzzle 0 2 > w/heat_world_$scale.out
    echo ""
    cat w/heat_world_$scale.in | bin/execute_puzzle 1 2 > w/heat_world_$scale.ref.out
    bin/compare_puzzle_output w/heat_world_$scale.in w/heat_world_$scale.ref.out w/heat_world_$scale.out 2
done

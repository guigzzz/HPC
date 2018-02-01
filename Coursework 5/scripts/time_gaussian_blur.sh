#!/bin/bash
make -j8
echo ""
scales=$(python -c 'for i in range(5, 100): print(int(5 * (i ** 2)))')
export HPCE_EM_GAUSS_THRESH=100000

for scale in $scales
do
    echo ""
    echo "==> scale: $scale"
    bin/run_puzzle_custom gaussian_blur gaussian_blur_GPU $scale 2
done

export HPCE_EM_GAUSS_THRESH=300

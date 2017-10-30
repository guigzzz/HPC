## Baseline Command
`time bin/julia -width 512 -height 512 -max-iter 512 -animation -max-frames 1 -engine reference > /dev/null`

## T1: Max-Iter
`for i in $(seq 0 5); do time bin/julia -width 512 -height 512 -max-iter $(echo '10^'$i | bc) -engine reference > /dev/null; done`

## T2: Dimension
`for i in $(seq 0 2 12); do time bin/julia -width $(echo '2^'$i | bc) -height $(echo '2^'$i | bc) -max-iter 512 -engine reference > /dev/null; done`

## T3: Width
`for i in $(seq 0 2 16); do time bin/julia -width $(echo '2^'$i | bc) -height 512 -max-iter 512 -engine reference > /dev/null; done`

## T4: Height
`for i in $(seq 0 2 16); do time bin/julia -width 512 -height $(echo '2^'$i | bc) -max-iter 512 -engine reference > /dev/null; done`

## T5: Max-frames
`for i in $(seq 1 4 29); do time bin/julia -width 512 -height 512 -max-iter 512 -animation -max-frames $i -engine reference > /dev/null; done`
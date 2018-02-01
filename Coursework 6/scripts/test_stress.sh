#!/bin/bash
make all -j8
scales=$(python -c 'for i in range(1, 20): print(int(10 * (i ** 2)))')

swrite_ref=$(grep -rn drivers/driver_stress_ref.cpp -e "swrite_Basic=0")
swrite_tbb=$(grep -rn drivers/driver_stress.cpp -e "swrite_Basic=0")

if [ -z "$swrite_ref" ]
then
    echo "ref implementation has swrite set to true"
elif [ -z "$swrite_tbb" ]
then
    echo "tbb implementation has swrite set to true"
else
    for scale in $scales
    do 
        echo  "scale: $scale"
        ./bin/stress_ref_std $scale > /tmp/stress_ref$scale.txt
        ./bin/stress_std $scale > /tmp/stress_tbb$scale.txt
        diff /tmp/stress_ref$scale.txt /tmp/stress_tbb$scale.txt
        echo ""
    done
fi
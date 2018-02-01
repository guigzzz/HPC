#!/bin/bash
make all -j4

swrite_ref=$(grep -rn drivers/driver_certify_ref.cpp -e "swrite_Basic=0")
swrite_tbb=$(grep -rn drivers/driver_certify.cpp -e "swrite_Basic=0")

if [ -z "$swrite_ref" ]
then
    echo "ref implementation has swrite set to true"
elif [ -z "$swrite_tbb" ]
then
    echo "tbb implementation has swrite set to true"
else
    ./bin/certify_ref_std > /tmp/certify_ref.txt
    ./bin/certify_std > /tmp/certify_tbb.txt
    diff /tmp/certify_ref.txt /tmp/certify_tbb.txt
fi

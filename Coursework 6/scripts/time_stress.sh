#!/bin/zsh
make all -j8

seq_start=${1:-1}
seq_end=${2:-1000}

swrite_ref=$(grep -rn drivers/driver_stress_ref.cpp -e "swrite_Basic=0")
swrite_tbb=$(grep -rn drivers/driver_stress.cpp -e "swrite_Basic=0")

if [ -z "$swrite_ref" ]
then
    echo "ref implementation has swrite set to true"
elif [ -z "$swrite_tbb" ]
then
    echo "tbb implementation has swrite set to true"
else
    rm -f full_log.txt summary_log.txt
    touch full_log.txt summary_log.txt
    for scale in `seq $seq_start $seq_end`
    do 
        out=$(./bin/stress_tbb_std $scale)
        chosenN=$(echo $out | grep "ChosenN.*$" | grep -o "[^ ]*$")
        timeUsed=$(echo $out | grep "TimeUsed.*$" | grep -o "[^ ]*$")
        score=$(echo $out | grep "Score.*$" | grep -o "[^ ]*$")

        # Log
        echo $out >> full_log.txt
        echo "$scale, $chosenN, $timeUsed, $score"
        echo "$scale, $chosenN, $timeUsed, $score" >> summary_log.txt
    done
fi

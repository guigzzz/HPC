#!/bin/bash

# make sure all is compiled
mingw32-make clean > /dev/null 2> /dev/null
mingw32-make all -j8 > /dev/null 2> /dev/null

# update id here
id=gr1714
p=bin/$id/
progs=$(ls ${p})

test_dim=10
num_time_steps=1000
prop_constant=0.1
dt=0.1

bin/make_world.exe $test_dim $prop_constant \
| bin/step_world.exe $dt $num_time_steps > /tmp/step_world.txt 2> /dev/null

for prog in $progs
do
    bin/make_world.exe $test_dim $prop_constant \
    | ${p}$prog $dt $num_time_steps > /tmp/$prog.txt 2> /dev/null
done

# round(){
#     echo $(printf %.$2f $(echo "scale=$2;(((10^$2)*$1)+0.5)/(10^$2)" | bc))
# }; 

num_round=4


for prog in $progs
do
    comm -3 <(sort /tmp/step_world.txt)  <(sort /tmp/$prog.txt) > /tmp/${prog}_diff.txt
    # comm -23 <(sort /tmp/step_world.txt)  <(sort /tmp/${prog}.txt) > /tmp/${prog}_diff2.txt
    if [ "$(cat /tmp/${prog}_diff.txt)" == "" ]
        then
            echo "test for ${prog}: PASS"
        else
            # ref_rounded=$(for i in $(cat /tmp/${prog}_diff1.txt); do echo $(round $i $num_round); done)
            # impl_rounded=$(for i in $(cat /tmp/${prog}_diff2.txt); do echo $(round $i $num_round); done)
            # echo $ref_rounded
            # echo $impl_rounded
            # if [ ref_rounded == impl_rounded ]
            #     then
            #         echo "test for ${prog}: PASS"
            #     else
            #         echo "test for ${prog}: FAIL"
            #         echo "differences:"
            #         echo ""
            #         cat /tmp/${prog}_diff1.txt
            #         echo ""
            #         cat /tmp/${prog}_diff2.txt
            #         echo ""
            # fi
            echo "test for ${prog}: FAIL"
            echo "differences:"
            echo ""
            cat /tmp/${prog}_diff.txt
            echo ""
    fi
done
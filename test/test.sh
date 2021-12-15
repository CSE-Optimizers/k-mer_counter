#!/bin/bash

ITER=$1
echo "">time.txt

for k in {15..31..2}
do
    for i in {15..6..-3}
    do
        for j in $(seq $ITER);
        do
            echo $k>>time.txt
            echo $i>>time.txt
            { time mpirun -np $i --hosts D-1,D-2,D-3,D-4,D-5,D-6,D-8,D-10,D-11,D-13,D-14,D-15,D-16,D-17,D-18 ./test/multiple-hashmaps/k-mer_counter/kmer_counter.out vesca.fastq $k ; } 2>>time.txt
            echo "">>time.txt
        done
    done
done
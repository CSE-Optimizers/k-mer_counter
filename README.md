MPI should be installed in the system before compiling.


Configure the AltMakefile with correct paths for boost and [sparsehash](https://github.com/sparsehash/sparsehash)

----
----
## Compile

for distributor tool

```make -f AltMakefile kmer_distributor```


for counting tool

```make -f AltMakefile kmer_counter```

for query tool

```make -f AltMakefile finalizer```


----
----

## Run

----

### for distributor tool

```mpirun <mpi args> ./kmer_distributor.out <data file> <output folder>```

*example*

```mpirun -np 16 --hostfile hosts.txt ./kmer_distributor.out vesca.fastq /home/user1/tempfolder/```

---

### for counting tool

```mpirun <mpi args> ./kmer_counter.out <encoded file> <k> <output folder>```

*example*

```mpirun -np 16 --hostfile hosts.txt ./kmer_counter.out /home/user1/tempfolder/encoded.bin 28 /home/user1/tempfolder/```

---


### for querying tool

```mpirun <mpi args> ./finalizer.out <k> <query file> <no of queries> <output folder>```

*example*

```mpirun -np 16 --hostfile hosts.txt ./finalizer.out 28 query.txt 10 /home/user1/tempfolder/```

Example format of query kmer file (for `k`=5)


> AAAAA <br/> AAAAC <br/> ACGTT <br/> AAAAT <br/> TTTTG <br/> TTTTT

For example if `no of queries` = 4, then only the first 4 kmers will be queried for count.

---


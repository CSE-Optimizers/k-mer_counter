MPI should be installed in the system before compiling.

----
Compile with

```make```

----

Run command

```mpirun ./kmer_counter.out path/to/datafile k_value path/to/output/folder/```

Example

```mpirun -np 16 --hostfile hostfile.txt ./test/k-mer_counter_master_file_read/kmer_counter.out vesca.fastq 28 /home/damika/Documents/test_results/data/```


----
----

Finalizer tool

Compile with

```make finalizer```

----

Run command

```mpirun ./finalizer.out k_value path/to/file_with_query_kmers no_of_queries_to_run```

Note: Use same number of processors for counter and finalizer.

Example format of query kmer file (for `kmer_size`=5)


> AAAAA <br/> AAAAC <br/> ACGTT <br/> AAAAT <br/> TTTTG <br/> TTTTT

For example if `no_of_queries_to_run` = 5, then only the first 5 kmers will be queried for count.



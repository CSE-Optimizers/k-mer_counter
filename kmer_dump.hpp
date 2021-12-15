#ifndef KMER_DUMP_H
#define KMER_DUMP_H

#include <iostream>
#include <string>
#include "utils.hpp"
using std::cout;
using std::endl;
// using tr1::hash;  // or __gnu_cxx::hash, or maybe tr1::hash, depending on your OS
using std::hash;
using std::string;
// using namespace std;

#define DIRECTORY_SEP '/'
#define MAX_PARTITIONS 10

void mergeHashmap(custom_dense_hash_map newHashMap, int partition, string base_path);
void mergeArrayToHashmap(uint64_t *dataArray, int dataArrayLength, int partition, string base_path);
void dumpHashmap(custom_dense_hash_map hashMap, int partition, int partitionFileCounts[], string base_path);
void loadHashMap(custom_dense_hash_map *hashMap, int partition, int file_index, string base_path);
void saveHashMap(custom_dense_hash_map *hashMap, int partition, string base_path);
#endif
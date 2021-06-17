#ifndef KMER_DUMP_H
#define KMER_DUMP_H

#include <iostream>
#include <string>
#include <sparsehash/dense_hash_map>

using google::dense_hash_map; // namespace where class lives by default
using std::cout;
using std::endl;
// using tr1::hash;  // or __gnu_cxx::hash, or maybe tr1::hash, depending on your OS
using std::hash;
using std::string;
// using namespace std;

#define DIRECTORY_SEP '/'
#define MAX_PARTITIONS 10

void mergeHashmap(dense_hash_map<uint64_t, uint64_t> newHashMap, int partition, string base_path);
void mergeArrayToHashmap(uint64_t *dataArray, int dataArrayLength, int partition, string base_path);
void dumpHashmap(dense_hash_map<uint64_t, uint64_t> hashMap, int partition, int partitionFileCounts[], string base_path);
void loadHashMap(dense_hash_map<uint64_t, uint64_t> *hashMap, int partition, string base_path);
void saveHashMap(dense_hash_map<uint64_t, uint64_t> *hashMap, int partition, string base_path);
#endif
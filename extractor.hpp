#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <sparsehash/dense_hash_map>

void countKmersFromBuffer(int &kmer_size, char *read_buffer, google::dense_hash_map<uint64_t, uint64_t> *counts);

#endif
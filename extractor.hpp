#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include <sparsehash/dense_hash_map>
#include <stddef.h>

static inline __attribute__((always_inline)) size_t getCharacterEncoding(const char character);
void countKmersFromBuffer(std::size_t &kmer_size, char* read_buffer, google::dense_hash_map<std::size_t, std::size_t> *counts);

#endif
#ifndef COM_H
#define COM_H

#include <sparsehash/dense_hash_map>
#include <stddef.h>

void copyToComOutBuffer(std::size_t &current_size, uint64_t *buffer, google::dense_hash_map<uint64_t, uint64_t> *counts);

#endif
#ifndef COM_H
#define COM_H

#include <sparsehash/dense_hash_map>
#include <stddef.h>


void copyToComOutBuffer(std::size_t &current_size, size_t* buffer, google::dense_hash_map<std::size_t, std::size_t> *counts);

#endif
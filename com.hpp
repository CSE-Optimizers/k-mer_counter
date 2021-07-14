#ifndef COM_H
#define COM_H

#include <stddef.h>
#include "utils.hpp"

void copyToComOutBuffer(std::size_t &current_size, uint64_t *buffer, custom_dense_hash_map *counts);

#endif
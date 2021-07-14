#ifndef UTILS_H
#define UTILS_H

#include <sparsehash/dense_hash_map>
#include "MurmurHash2.hpp"

template <typename T>
struct CustomHasher
{
  size_t operator()(const T &t) const
  {
    // return t;
    return MurmurHash64A((void *)&t, sizeof(T), (uint64_t)7);
  }
};

typedef google::dense_hash_map<uint64_t, uint64_t, CustomHasher<const uint64_t>> custom_dense_hash_map;

#endif
#ifndef UTILS_H
#define UTILS_H

#include <sparsehash/dense_hash_map>
#include <sparsehash/sparse_hash_map>
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

typedef uint64_t hashmap_key_type;
typedef uint64_t hashmap_value_type;

// typedef google::sparse_hash_map<hashmap_key_type, hashmap_value_type, CustomHasher<const hashmap_key_type>> custom_dense_hash_map;
typedef google::dense_hash_map<hashmap_key_type, hashmap_value_type, CustomHasher<const hashmap_key_type>> custom_dense_hash_map;

struct writerArguments{
    int partition;
    custom_dense_hash_map *counts;
};

struct FileChunkData{
  int32_t first_line_type;
  char chunk_buffer[]; 
};

enum LineType
{
  FIRST_IDENTIFIER_LINE = 0,
  SEQUENCE_LINE = 1,
  SECOND_IDENTIFIER_LINE = 2,
  QUALITY_LINE = 3
};

struct CounterArguments
{
    char *buffer;
    uint64_t allowed_length;
    enum LineType first_line_type;
    bool reset_status;
};

#endif
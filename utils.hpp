#ifndef UTILS_H
#define UTILS_H

#include <vector>
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

struct WriterArguments
{
  int partition;
  custom_dense_hash_map *counts;
};

struct FileChunkData
{
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

struct EncoderArguments
{
  char *buffer;
  uint64_t buffer_size;
};

struct EncoderWriterArgs
{
  size_t encoded_size;
  uint32_t *encoded_chunk;
};

struct GeneratorArguments
{
  uint32_t *encoded_read;
  uint32_t bases_per_block;
  uint32_t read_length;
  uint32_t block_count;
};

struct KmerBin
{
  int id;
  uint64_t* bin;
  size_t filled_size;
};


#endif
#include "com.hpp"
#include <stdexcept>
#include <sparsehash/dense_hash_map>

void copyToComOutBuffer(std::size_t &current_size, size_t *buffer, google::dense_hash_map<std::size_t, std::size_t> *counts)
{
  if (current_size != (*counts).size())
  {
    throw std::invalid_argument("Hashmap size is not equal to the buffer size");
  }

  google::dense_hash_map<size_t, size_t>::iterator it = (*counts).begin();
  size_t buffer_i = 0;

  for (; it != (*counts).end(); ++it)
  {
    buffer[2 * buffer_i] = it->first;
    buffer[2 * buffer_i + 1] = it->second;
    buffer_i++;
  }
}
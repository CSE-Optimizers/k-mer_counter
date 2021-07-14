#include "com.hpp"
#include <stdexcept>


void copyToComOutBuffer(std::size_t &current_size, uint64_t *buffer, custom_dense_hash_map *counts)
{
  if (current_size != (*counts).size())
  {
    throw std::invalid_argument("Hashmap size is not equal to the buffer size");
  }

  custom_dense_hash_map::iterator it = (*counts).begin();
  size_t buffer_i = 0;

  for (; it != (*counts).end(); ++it)
  {
    buffer[2 * buffer_i] = it->first;
    buffer[2 * buffer_i + 1] = it->second;
    buffer_i++;
  }
}
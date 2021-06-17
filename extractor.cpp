#include "extractor.hpp"

static inline __attribute__((always_inline)) uint64_t getCharacterEncoding(const char character)
{
  switch (character)
  {
  case 'A':
    return (uint64_t)0;
    break;
  case 'C':
    return (uint64_t)1;
    break;
  case 'G':
    return (uint64_t)2;
    break;
  case 'T':
    return (uint64_t)3;
    break;
  default:
    return (uint64_t)4;
    break;
  }
}

void countKmersFromBuffer(int &kmer_size, char *read_buffer, google::dense_hash_map<uint64_t, uint64_t> *counts)
{
  size_t index = 0;

  uint64_t calculatedCounterIndex = 0;
  uint64_t characterEncoding = 0;

  uint64_t bit_clear_mask = ~(((uint64_t)3) << (kmer_size * 2));
  uint64_t invalid_check_mask = ((uint64_t)1) << 2;
  int kmer_filled_length = 0;

  while (read_buffer[index] != 0 && read_buffer[index] != '\n')
  {

    kmer_filled_length++;

    kmer_filled_length = std::min(kmer_filled_length, kmer_size);
    characterEncoding = getCharacterEncoding(read_buffer[index]);
    calculatedCounterIndex = ((calculatedCounterIndex << 2) & bit_clear_mask) | characterEncoding;
    // printf("%ld\t",calculatedCounterIndex);
    // printf("%ld\t", characterEncoding);
    if ((characterEncoding & invalid_check_mask) == 0)
    {
      if (kmer_filled_length == kmer_size)
      {
        (*counts)[calculatedCounterIndex]++;
      }
    }
    else
    {
      calculatedCounterIndex = 0;
      kmer_filled_length = 0;
    }
    index++;

    // printf("%d\n",counts[calculatedCounterIndex]);
  }
  // printf("\n");
}

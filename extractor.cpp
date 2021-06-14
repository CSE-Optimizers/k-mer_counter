#include "extractor.hpp"

static inline __attribute__((always_inline)) size_t getCharacterEncoding(const char character)
{
  switch (character)
  {
  case 'A':
    return 0;
    break;
  case 'C':
    return 1;
    break;
  case 'G':
    return 2;
    break;
  case 'T':
    return 3;
    break;
  default:
    return 4;
    break;
  }
}




void countKmersFromBuffer(std::size_t &kmer_size, char *read_buffer, google::dense_hash_map<std::size_t, std::size_t> *counts)
{
  size_t index = 0;
  size_t read_index = 0;
  size_t calculatedCounterIndex = 0;
  size_t characterEncoding = 0;

  size_t bit_clear_mask = ~(3 << (kmer_size * 2));
  size_t invalid_check_mask = 1 << 2;
  size_t kmer_filled_length = 0;
  bool flag;

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

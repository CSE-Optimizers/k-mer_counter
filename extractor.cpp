#include "iostream"
#include "extractor.hpp"

using google::dense_hash_map;
using std::cout;
using std::endl;

#define LINE_READ_BUFFER_SIZE 2050

int checkLineReadBuffer(size_t buffer_size, char *buffer)
{
  if (buffer[buffer_size - 2] != 0 && buffer[buffer_size - 2] != '\n')
  {
    std::cerr << "Buffer overflow when reading lines" << endl;
    exit(EXIT_FAILURE);
  }
  return 0;
}

enum LineType getLineType(FILE *fp)
{
  char buffer[LINE_READ_BUFFER_SIZE + 1] = {0};
  char char_1 = fgetc(fp);
  fgets(buffer, LINE_READ_BUFFER_SIZE, fp);
  checkLineReadBuffer(LINE_READ_BUFFER_SIZE, buffer);
  char char_2 = fgetc(fp);
  fgets(buffer, LINE_READ_BUFFER_SIZE, fp);
  checkLineReadBuffer(LINE_READ_BUFFER_SIZE, buffer);
  char char_3 = fgetc(fp);
  fgets(buffer, LINE_READ_BUFFER_SIZE, fp);
  checkLineReadBuffer(LINE_READ_BUFFER_SIZE, buffer);
  // cout << "leading characters in first 3 lines = " << char_1 << " " << char_2 << " " << char_3 << endl;
  if (char_1 == '@')
  {
    if (char_2 == '@')
    {
      return QUALITY_LINE;
    }
    else
    {
      return FIRST_IDENTIFIER_LINE;
    }
  }
  else
  {
    if (char_2 == '@')
    {
      if (char_3 == '@')
      {
        return SECOND_IDENTIFIER_LINE;
      }
      else
      {
        return QUALITY_LINE;
      }
    }
    else
    {
      if (char_1 == '+')
      {
        return SECOND_IDENTIFIER_LINE;
      }
      else
      {
        return SEQUENCE_LINE;
      }
    }
  }
}

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

void countKmersFromBuffer(
    const uint64_t kmer_size,
    char *buffer,
    const uint64_t buffer_size,
    const uint64_t allowed_length,
    const enum LineType first_line_type,
    const bool is_starting_from_line_middle,
    dense_hash_map<uint64_t, uint64_t> *counts)
{
  static bool line_type_identified = false;
  static enum LineType current_line_type;
  if (!line_type_identified)
  {
    current_line_type = first_line_type;
    line_type_identified = true;
  }

  static uint64_t current_kmer_encoding = 0;
  static uint64_t kmer_filled_length = 0;
  uint64_t current_character_encoding = 0;

  const uint64_t bit_clear_mask = ~(3 << (kmer_size * 2));
  const uint64_t invalid_check_mask = 1 << 2;

  for (uint64_t buffer_i = 0; buffer_i < allowed_length; buffer_i++)
  {
    if (buffer[buffer_i] == '\n')
    {
      current_line_type = LineType((current_line_type + 1) % 4);
      current_kmer_encoding = 0;
      kmer_filled_length = 0;
      continue;
    }
    if (current_line_type != SEQUENCE_LINE)
    {
      continue;
    }

    kmer_filled_length++;

    kmer_filled_length = std::min(kmer_filled_length, kmer_size);
    current_character_encoding = getCharacterEncoding(buffer[buffer_i]);
    current_kmer_encoding = ((current_kmer_encoding << 2) & bit_clear_mask) | current_character_encoding;
    // cout << "character=" << buffer[buffer_i] << " character_encoding=" << current_character_encoding
    //      << " kmer_encoding=" << current_kmer_encoding << " kmer_filled_length=" << kmer_filled_length << endl;
    if ((current_character_encoding & invalid_check_mask) == 0)
    {
      if (kmer_filled_length == kmer_size)
      {
        (*counts)[current_kmer_encoding]++;
      }
    }
    else
    {
      current_kmer_encoding = 0;
      kmer_filled_length = 0;
    }
  }
}
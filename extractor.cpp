#include "iostream"
#include "extractor.hpp"

using std::cout;
using std::endl;

#define LINE_READ_BUFFER_SIZE 2050
#define HASHMAP_MAX_SIZE 0x400000

// int checkLineReadBuffer(size_t buffer_size, char *buffer)
// {
//   if (buffer[buffer_size - 2] != 0 && buffer[buffer_size - 2] != '\n')
//   {
//     std::cerr << "Buffer overflow when reading lines" << endl;
//     exit(EXIT_FAILURE);
//   }
//   return 0;
// }

// enum LineType getLineType(FILE *fp)
// {
//   char buffer[LINE_READ_BUFFER_SIZE + 1] = {0};
//   char char_1 = fgetc(fp);
//   fgets(buffer, LINE_READ_BUFFER_SIZE, fp);
//   checkLineReadBuffer(LINE_READ_BUFFER_SIZE, buffer);
//   char char_2 = fgetc(fp);
//   fgets(buffer, LINE_READ_BUFFER_SIZE, fp);
//   checkLineReadBuffer(LINE_READ_BUFFER_SIZE, buffer);
//   char char_3 = fgetc(fp);
//   fgets(buffer, LINE_READ_BUFFER_SIZE, fp);
//   checkLineReadBuffer(LINE_READ_BUFFER_SIZE, buffer);
//   // cout << "leading characters in first 3 lines = " << char_1 << " " << char_2 << " " << char_3 << endl;
//   if (char_1 == '@')
//   {
//     if (char_2 == '@')
//     {
//       return QUALITY_LINE;
//     }
//     else
//     {
//       return FIRST_IDENTIFIER_LINE;
//     }
//   }
//   else
//   {
//     if (char_2 == '@')
//     {
//       if (char_3 == '@')
//       {
//         return SECOND_IDENTIFIER_LINE;
//       }
//       else
//       {
//         return QUALITY_LINE;
//       }
//     }
//     else
//     {
//       if (char_1 == '+')
//       {
//         return SECOND_IDENTIFIER_LINE;
//       }
//       else
//       {
//         return SEQUENCE_LINE;
//       }
//     }
//   }
// }

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

// static inline __attribute__((always_inline)) uint64_t getComplementCharacter(uint64_t character)
// {
//   switch (character)
//   {
//   case 0:
//     return (uint64_t)3;
//     break;
//   case 1:
//     return (uint64_t)2;
//     break;
//   case 2:
//     return (uint64_t)1;
//     break;
//   case 3:
//     return (uint64_t)0;
//     break;
//   default:
//     return (uint64_t)4;
//     break;
//   }
// }


static inline __attribute__((always_inline)) int getKmerPartition(const uint64_t kmer, int kmer_size, int partition_count)
{
  // return kmer % partition_count;
  return (kmer >> (kmer_size / 2)) % partition_count;
}

// void countKmersFromBuffer(
//     const uint64_t kmer_size,
//     char *buffer,
//     const uint64_t buffer_size,
//     const uint64_t allowed_length,
//     const enum LineType first_line_type,
//     const bool is_starting_from_line_middle,
//     custom_dense_hash_map *counts)
// {
//   static bool line_type_identified = false;
//   static enum LineType current_line_type;
//   if (!line_type_identified)
//   {
//     current_line_type = first_line_type;
//     line_type_identified = true;
//   }

//   static uint64_t current_kmer_encoding = 0;
//   static uint64_t kmer_filled_length = 0;
//   uint64_t current_character_encoding = 0;

//   const uint64_t bit_clear_mask = ~(((uint64_t)3) << (kmer_size * 2));
//   const uint64_t invalid_check_mask = ((uint64_t)1) << 2;

//   for (uint64_t buffer_i = 0; buffer_i < allowed_length; buffer_i++)
//   {
//     if (buffer[buffer_i] == '\n')
//     {
//       current_line_type = LineType((current_line_type + 1) % 4);
//       current_kmer_encoding = 0;
//       kmer_filled_length = 0;
//       continue;
//     }
//     if (current_line_type != SEQUENCE_LINE)
//     {
//       continue;
//     }

//     kmer_filled_length++;

//     kmer_filled_length = std::min(kmer_filled_length, kmer_size);

//     current_character_encoding = getCharacterEncoding(buffer[buffer_i]);
//     // current_character_encoding = (uint64_t) ((buffer[buffer_i] & 14)>>1);

//     current_kmer_encoding = ((current_kmer_encoding << 2) & bit_clear_mask) | current_character_encoding;
//     // cout << "character=" << buffer[buffer_i] << " character_encoding=" << current_character_encoding
//     //      << " kmer_encoding=" << current_kmer_encoding << " kmer_filled_length=" << kmer_filled_length << endl;
//     if ((current_character_encoding & invalid_check_mask) == 0)
//     {
//       if (kmer_filled_length == kmer_size)
//       {
//         (*counts)[current_kmer_encoding]++;
//       }
//     }
//     else
//     {
//       current_kmer_encoding = 0;
//       kmer_filled_length = 0;
//     }
//   }
// }

// void countKmersFromBufferWithPartitioning(
//     const uint64_t kmer_size,
//     char *buffer,
//     const uint64_t buffer_size,
//     const uint64_t allowed_length,
//     const enum LineType first_line_type,
//     const bool reset_status,
//     custom_dense_hash_map **counts,
//     int partition_count,
//     boost::lockfree::queue<struct WriterArguments *> *writer_queue)
// {
 
//   // static enum LineType current_line_type;
//   uint64_t current_kmer_encoding = 0;
 
//   // reversed complement of the current kmer
//   uint64_t current_rc_kmer_encoding = 0;
//   uint64_t canonical_kmer_encoding = 0;
 
//   uint64_t kmer_filled_length = 0;
//   // if (reset_status)
//   // {
//   //   current_line_type = first_line_type;
//   //   current_kmer_encoding = 0;
//   //   kmer_filled_length = 0;
//   // }


//   uint64_t current_character_encoding = 0;
//   uint64_t current_complement_character_encoding = 0;

//   const uint64_t bit_clear_mask = ~(((uint64_t)3) << (kmer_size * 2));
//   const uint64_t invalid_check_mask = ((uint64_t)1) << 2;
//   const uint64_t left_shift_amount = (kmer_size - 1) * 2;

//   for (uint64_t buffer_i = 0; buffer_i < allowed_length; buffer_i++)
//   {
//     if (buffer[buffer_i] == 0)
//     {
//       break;
//     }
//     if (buffer[buffer_i] == '\n')
//     {
//       // current_line_type = LineType((current_line_type + 1) % 4);
//       current_kmer_encoding = 0;
//       kmer_filled_length = 0;
//       continue;
//     }
//     // if (current_line_type != SEQUENCE_LINE)
//     // {
//     //   continue;
//     // }

//     kmer_filled_length++;

//     kmer_filled_length = std::min(kmer_filled_length, kmer_size);

//     current_character_encoding = getCharacterEncoding(buffer[buffer_i]);
//     current_complement_character_encoding = ((uint64_t)3) - current_character_encoding;
//     // current_complement_character_encoding = getComplementCharacter(current_character_encoding);

//     current_kmer_encoding = ((current_kmer_encoding << 2) & bit_clear_mask) | current_character_encoding;
//     current_rc_kmer_encoding = (current_rc_kmer_encoding >> 2) | (current_complement_character_encoding << left_shift_amount);
 

//     if ((current_character_encoding & invalid_check_mask) == 0)
//     {
//       if (kmer_filled_length == kmer_size)
//       {
//         // current_rc_kmer_encoding = temp_rc_kmer_encoding >> shift_amout;
//         // if (current_rc_kmer_encoding < current_kmer_encoding)
//         // {
//         //   current_kmer_encoding = current_rc_kmer_encoding;
//         // }
//         canonical_kmer_encoding = std::min(current_rc_kmer_encoding, current_kmer_encoding);
//         int current_kmer_partition = getKmerPartition(canonical_kmer_encoding, kmer_size, partition_count);
//         (*counts[current_kmer_partition])[canonical_kmer_encoding]++;

//         if ((*counts[current_kmer_partition]).size() >= HASHMAP_MAX_SIZE)
//         {
          // struct WriterArguments *this_writer_argument;
          // this_writer_argument = new WriterArguments;
          // this_writer_argument->partition = current_kmer_partition;
          // this_writer_argument->counts = counts[current_kmer_partition];

          // writer_queue->push(this_writer_argument);

          // custom_dense_hash_map *tmp = new custom_dense_hash_map;
          // tmp->set_empty_key(-1);
//           counts[current_kmer_partition] = tmp;
//         }
//       }
//     }
//     else
//     {
//       current_kmer_encoding = 0;
//       current_rc_kmer_encoding = 0;
//       kmer_filled_length = 0;
//     }
//   }
// }






void generateKmersWithPartitioning(
    const uint64_t kmer_size,
    uint32_t *encoded_read,
    uint32_t bases_per_block,
    uint32_t read_length,
    uint32_t block_count,
    int partition_count,
    struct KmerBin** bins,
    size_t bin_size,
    boost::lockfree::queue<struct KmerBin *> *writer_queue)
{
 
  uint64_t current_kmer_encoding = 0;
 
  // reversed complement of the current kmer
  uint64_t current_rc_kmer_encoding = 0;
  uint64_t canonical_kmer_encoding = 0;
 
  uint64_t kmer_filled_length = 0;


  uint64_t current_character_encoding = 0;
  uint64_t current_complement_character_encoding = 0;

  const uint64_t bit_clear_mask = ~(((uint64_t)3) << (kmer_size * 2));
  const uint64_t invalid_check_mask = ((uint64_t)1) << 2;
  const uint64_t left_shift_amount = (kmer_size - 1) * 2;

  int current_block = 0;
  int current_character_offest = (bases_per_block - 1)*3;

  for(uint32_t i = 0; i < read_length; i++){  
    
    uint32_t current_char_enc = (encoded_read[current_block] & (((uint32_t)7) << current_character_offest)) >> current_character_offest;

    current_character_offest-=3;
    
    if(current_character_offest == 0){
      current_block++;
      current_character_offest = (bases_per_block - 1)*3;
    }

    kmer_filled_length++;

    kmer_filled_length = std::min(kmer_filled_length, kmer_size);

    current_character_encoding = (uint64_t)current_char_enc;
    current_complement_character_encoding = ((uint64_t)3) - current_character_encoding;

    current_kmer_encoding = ((current_kmer_encoding << 2) & bit_clear_mask) | current_character_encoding;
    current_rc_kmer_encoding = (current_rc_kmer_encoding >> 2) | (current_complement_character_encoding << left_shift_amount);
 

    if ((current_character_encoding & invalid_check_mask) == 0)
    {
      if (kmer_filled_length == kmer_size)
      {

        canonical_kmer_encoding = std::min(current_rc_kmer_encoding, current_kmer_encoding);
        int current_kmer_partition = getKmerPartition(canonical_kmer_encoding, kmer_size, partition_count);

        // add kmer to bin
        bins[current_kmer_partition]->bin[bins[current_kmer_partition]->filled_size] = canonical_kmer_encoding;
        bins[current_kmer_partition]->filled_size++;

        if (bins[current_kmer_partition]->filled_size >=  bin_size)
        {
          //send bin if the bin is filled

          writer_queue->push(bins[current_kmer_partition]);
          bins[current_kmer_partition] = (KmerBin*) malloc(sizeof(struct KmerBin));
          assert(bins[current_kmer_partition]);
          bins[current_kmer_partition]->id = current_kmer_partition;
          bins[current_kmer_partition]->bin = (uint64_t*) malloc(bin_size * sizeof(uint64_t));
          assert(bins[current_kmer_partition]->bin);
          memset(bins[current_kmer_partition]->bin, 0, bin_size * sizeof(uint64_t));
          bins[current_kmer_partition]->filled_size = 0;
        }      
        
        
      }
    }
    else
    {
      current_kmer_encoding = 0;
      current_rc_kmer_encoding = 0;
      kmer_filled_length = 0;
    }

  }

}

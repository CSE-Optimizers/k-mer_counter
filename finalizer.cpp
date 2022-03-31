#include <iostream>
#include <mpi.h>
#include <string>
#include <unordered_map>
#include <vector>
#include "kmer_dump.hpp"
#include "utils.hpp"

#define READ_BUFFER_SIZE 100
#define PARTITION_COUNT 100

using std::cout;
using std::endl;

void exit_error(char const *error_message)
{
  cout << endl
       << "ERROR : " << error_message << endl;
  exit(EXIT_FAILURE);
}

static inline __attribute__((always_inline)) uint64_t getCharacterEncoding(const char character)
{
  switch (character)
  {
  case 'A':
    return (uint64_t)0;

  case 'C':
    return (uint64_t)1;

  case 'G':
    return (uint64_t)2;

  case 'T':
    return (uint64_t)3;

  default:
    exit_error("Invalid Base Character");
    return (uint64_t)4;
  }
}

static inline __attribute__((always_inline)) void getKmerFromIndex(const int kmer_size, const uint64_t index, char *out_buffer)
{
  uint64_t character_mask = ((uint64_t)3) << ((kmer_size - 1) * 2);
  uint64_t character_encoding = 0;
  for (int i = 0; i < kmer_size; i++)
  {
    character_encoding = (index & character_mask) >> ((kmer_size - i - 1) * 2);
    switch (character_encoding)
    {
    case 0:
      out_buffer[i] = 'A';
      break;

    case 1:
      out_buffer[i] = 'C';
      break;

    case 2:
      out_buffer[i] = 'G';
      break;

    case 3:
      out_buffer[i] = 'T';
      break;

    default:
      break;
    }

    character_mask = character_mask >> 2;
  }

  return;
}

int main(int argc, char *argv[])
{
  int num_tasks, rank;
  const int kmer_size = atoi(argv[1]);
  std::string query_file_name = argv[2];
  const int queries_n = atoi(argv[3]);
  std::string output_base_path = argv[4]; //  /home/damika/Documents/test_results/data/

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  char read_buffer[READ_BUFFER_SIZE];
  hashmap_key_type query_kmers[queries_n];
  hashmap_value_type query_final_counts[queries_n];

  if (rank == 0)
  {
    FILE *query_file;
    query_file = fopen(query_file_name.c_str(), "r");
    for (int query_i = 0; query_i < queries_n; query_i++)
    {
      memset(read_buffer, 0, READ_BUFFER_SIZE);
      fgets(read_buffer, READ_BUFFER_SIZE, query_file);
      hashmap_key_type kmer = 0;
      for (int character_i = 0; character_i < kmer_size; character_i++)
      {
        kmer = (kmer << 2) | getCharacterEncoding(read_buffer[character_i]);
      }
      query_kmers[query_i] = kmer;
    }
    fclose(query_file);
  }
  MPI_Bcast(query_kmers, queries_n, MPI_UINT64_T, 0, MPI_COMM_WORLD);

  hashmap_value_type *query_kmer_local_counts = (hashmap_value_type *)malloc(queries_n * sizeof(hashmap_value_type));
  memset(query_kmer_local_counts, 0, queries_n * sizeof(hashmap_value_type));

  if (rank > 0)
  {
    std::unordered_map<hashmap_key_type, int> query_kmer_index_map;
    std::unordered_map<int, std::vector<hashmap_key_type> *> partition_kmer_map;

    for (int kmer_i = 0; kmer_i < queries_n; kmer_i++)
    {
      hashmap_key_type kmer = query_kmers[kmer_i];
      query_kmer_index_map[kmer] = kmer_i;

      // TODO: make this consistent with the mapper function
      int partition_id = (kmer >> (kmer_size / 2)) % PARTITION_COUNT;

      if (partition_kmer_map.find(partition_id) == partition_kmer_map.end())
      {
        // partition is encountered for the 1st time
        partition_kmer_map[partition_id] = new std::vector<hashmap_key_type>;
      }
      partition_kmer_map[partition_id]->push_back(kmer);
    }


    custom_dense_hash_map *local_hashmap = new custom_dense_hash_map;
    local_hashmap->set_empty_key(-1);

    std::unordered_map<int, std::vector<hashmap_key_type> *>::iterator partition_iterator = partition_kmer_map.begin();
    // char *temp = (char *)malloc((kmer_size + 1) * sizeof(char));

    while (partition_iterator != partition_kmer_map.end())
    {
      loadHashMap(local_hashmap, partition_iterator->first, output_base_path);
      std::vector<hashmap_key_type> kmers = *(partition_iterator->second);
      std::vector<hashmap_key_type>::iterator kmer_iterator;


      for (kmer_iterator = kmers.begin(); kmer_iterator < kmers.end(); kmer_iterator++)
      {
        query_kmer_local_counts[query_kmer_index_map[*kmer_iterator]] = (*local_hashmap)[*kmer_iterator];
        // getKmerFromIndex(kmer_size, *kmer_iterator, temp);
        // cout << *kmer_iterator << "\t" << temp << "\t" << partition_iterator->first << "\t" << (*local_hashmap)[*kmer_iterator] << endl;

      }
      partition_iterator++;
    }

  }

  MPI_Reduce(query_kmer_local_counts, query_final_counts, queries_n, MPI_UINT16_T, MPI_SUM, 0, MPI_COMM_WORLD);
  if (rank == 0)
  {
    char *kmer_out = (char *)malloc((kmer_size + 1) * sizeof(char));
    memset(kmer_out, 0, kmer_size + 1);
    for (int i = 0; i < queries_n; i++)
    {
      getKmerFromIndex(kmer_size, query_kmers[i], kmer_out);
      cout << kmer_out << "\t" << query_final_counts[i] << endl;
    }
    cout << endl;
  }

  MPI_Finalize();
  return 0;
}

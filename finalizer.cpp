#include <iostream>
#include <mpi.h>
#include <string>
#include "kmer_dump.hpp"
#include "utils.hpp"

#define READ_BUFFER_SIZE 100
#define PARTITION_COUNT 10

using std::cout;
using std::endl;

struct QueryKmerPartition
{
  int size;
  int current_filled;
  hashmap_key_type **kmer_pointers;
  hashmap_value_type **kmer_count_pointers;
  int file_count;
};

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
  const char *query_file_name = argv[2];
  const int queries_n = atoi(argv[3]);

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  char read_buffer[READ_BUFFER_SIZE];
  hashmap_key_type query_kmers[queries_n];
  hashmap_value_type query_final_counts[queries_n];

  if (rank == 0)
  {
    FILE *query_file;
    query_file = fopen(query_file_name, "r");
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

  struct QueryKmerPartition query_kmer_partitions[PARTITION_COUNT];
  hashmap_value_type *query_kmer_local_counts = (hashmap_value_type *)malloc(queries_n * sizeof(hashmap_value_type));
  memset(query_kmer_local_counts, 0, queries_n * sizeof(hashmap_value_type));

  if (rank > 0)
  {
    for (int partition_i = 0; partition_i < PARTITION_COUNT; partition_i++)
    {
      query_kmer_partitions[partition_i].size = 0;
      query_kmer_partitions[partition_i].current_filled = 0;
      std::string stats_file_path = "/home/damika/Documents/test_results/data/" + std::to_string(partition_i) + "/stats";
      FILE *stats_file_fp = fopen(stats_file_path.c_str(), "r");
      fread(&query_kmer_partitions[partition_i].file_count, sizeof(int), 1, stats_file_fp);
      fclose(stats_file_fp);
      // cout << rank << " " << partition_i << " " << query_kmer_partitions[partition_i].file_count << endl;
    }

    for (int query_i = 0; query_i < queries_n; query_i++)
    {
      query_kmer_partitions[query_kmers[query_i] % PARTITION_COUNT].size++;
    }

    for (int partition_i = 0; partition_i < PARTITION_COUNT; partition_i++)
    {
      int size = query_kmer_partitions[partition_i].size;
      query_kmer_partitions[partition_i].kmer_pointers = (hashmap_key_type **)malloc(size * sizeof(hashmap_key_type *));
      query_kmer_partitions[partition_i].kmer_count_pointers = (hashmap_value_type **)malloc(size * sizeof(hashmap_value_type *));
    }

    for (int query_i = 0; query_i < queries_n; query_i++)
    {
      int partition_idx = query_kmers[query_i] % PARTITION_COUNT;
      int current_idx = query_kmer_partitions[partition_idx].current_filled;
      query_kmer_partitions[partition_idx].kmer_pointers[current_idx] = &query_kmers[query_i];
      query_kmer_partitions[partition_idx].kmer_count_pointers[current_idx] = &query_kmer_local_counts[query_i];
      query_kmer_partitions[partition_idx].current_filled++;
    }

    custom_dense_hash_map *local_hashmap = new custom_dense_hash_map;
    local_hashmap->set_empty_key(-1);

    for (int partition_i = 0; partition_i < PARTITION_COUNT; partition_i++)
    {
      int file_count = query_kmer_partitions[partition_i].file_count;
      for (int file_i = 0; file_i < file_count; file_i++)
      {
        loadHashMap(local_hashmap, partition_i, file_i, "/home/damika/Documents/test_results/data");
        const int query_kmers_n = query_kmer_partitions[partition_i].size;
        for (int kmer_i = 0; kmer_i < query_kmers_n; kmer_i++)
        {
          (*query_kmer_partitions[partition_i].kmer_count_pointers[kmer_i]) += (*local_hashmap)[*query_kmer_partitions[partition_i].kmer_pointers[kmer_i]];
        }
        local_hashmap->clear();
        local_hashmap = new custom_dense_hash_map;
        local_hashmap->set_empty_key(-1);
        if (rank == 12)
        {
          cout << "rank " << rank << "\tpartition " << partition_i << "\tfile " << file_i << endl;
        }
      }
    }
  }

  MPI_Reduce(query_kmer_local_counts, query_final_counts, queries_n, MPI_UINT64_T, MPI_SUM, 0, MPI_COMM_WORLD);
  if (rank == 0)
  {
    char *kmer_out = (char *)malloc((kmer_size + 1) * sizeof(char));
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

/**
 * Get line count before MPI part
 * Give allocated line section for each MPI process
 * 
*/
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <mpi.h>
#include <sparsehash/dense_hash_map>
#include <stdlib.h>
#include "extractor.hpp"
#include "com.hpp"
#include "kmer_dump.hpp"
#include <stddef.h>
#include <unistd.h>
#include <time.h>

#define READ_BUFFER_SIZE 2000
#define HASH_MAP_MAX_SIZE 0x1000000
#define DUMP_SIZE 10

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

  int kmer_size;
  size_t reads_n;
  char read_buffer[READ_BUFFER_SIZE];
  google::dense_hash_map<uint64_t, uint64_t> counts;

  int num_tasks, rank;
  uint64_t my_line_counter;
  uint64_t my_offset_data[2];
  uint64_t *com_out_buffer;
  MPI_Request sending_request;
  size_t com_out_counter = 0;

  const char *file_name = argv[1];
  kmer_size = atoi(argv[2]);
  counts.set_empty_key(-1);

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  uint64_t offset_data[num_tasks][2]; //offset (in lines) and no of reads for this process

  std::cout << "total processes = " << num_tasks << std::endl;
  std::cout << "rank = " << rank << std::endl;

  if (rank == 0)
  {
    // {
    //   int i = 0;
    //   while (0 == i)
    //     sleep(5);
    // }
    uint64_t total_line_count = 0;
    memset(read_buffer, 0, READ_BUFFER_SIZE * (sizeof read_buffer[0]));
    FILE *input_file;
    input_file = fopen(file_name, "r");
    while (fgets(read_buffer, READ_BUFFER_SIZE, input_file) != NULL)
    {
      total_line_count++;
    }
    fclose(input_file);

    std::cout << "Total number of lines in input file = " << total_line_count << std::endl;
    const uint64_t reads_per_process = total_line_count / (4 * (num_tasks - 1));
    const uint64_t lines_per_process = reads_per_process * 4;
    offset_data[0][0] = 0;
    offset_data[0][1] = 0;

    for (int process_i = 1; process_i < num_tasks; process_i++)
    {
      offset_data[process_i][0] = (process_i - 1) * lines_per_process;
      offset_data[process_i][1] = reads_per_process;
    }

    offset_data[num_tasks - 1][1] = (total_line_count - ((num_tasks - 2) * lines_per_process)) / 4;
  }
  MPI_Scatter(offset_data, 2, MPI_UINT64_T, my_offset_data, 2,
              MPI_UINT64_T, 0, MPI_COMM_WORLD);

  if (rank > 0)
  {
    /* wait until attaching debugger*/
    // if (rank == 1)
    // {
    //   int i = 0;
    //   while (0 == i)
    //     sleep(5);
    // }

    uint64_t my_offset = my_offset_data[0];
    uint64_t my_allocated_lines_n = my_offset_data[1] * 4;
    int tag = 0;
    clock_t t;

    std::cout << "my (" << rank << ")\t offset = " << my_offset << "\t lines = " << my_allocated_lines_n << std::endl;

    memset(read_buffer, 0, READ_BUFFER_SIZE * (sizeof read_buffer[0]));
    my_line_counter = 0;
    reads_n = 0;
    FILE *file = fopen(file_name, "r");
    while (fgets(read_buffer, READ_BUFFER_SIZE, file) && my_line_counter <= my_offset + my_allocated_lines_n)
    {
      if (my_offset > 0 && my_line_counter < my_offset)
      {
        my_line_counter++;
        continue;
      }

      if (read_buffer[READ_BUFFER_SIZE - 2] != 0 && read_buffer[READ_BUFFER_SIZE - 2] != '\n')
      {
        std::cerr << "Buffer overflow when reading lines" << std::endl;
        exit(EXIT_FAILURE);
      }
      if (!((my_line_counter - 1) % 4))
      {
        reads_n++;
        countKmersFromBuffer(kmer_size, read_buffer, &counts);
        // std::cout << "rank-" << rank << "\t line-" << my_line_counter << " " << read_buffer;
        // std::cout << rank << " - " << counts.size() << std::endl;

        if (counts.size() >= HASH_MAP_MAX_SIZE)
        {
          // If sent previously, wait for that sending to complete
          if (com_out_counter > 0)
          {
            std::cout << rank << " waiting for send complete\n";
            t = clock();
            MPI_Wait(&sending_request, MPI_STATUS_IGNORE);
            std::cout << "process " << rank << " waited " << ((double)(clock() - t)) / CLOCKS_PER_SEC << "seconds\n";
            free(com_out_buffer);
          }

          size_t current_size = counts.size();

          // contiguous memory segment is needed for 2d array
          com_out_buffer = (uint64_t *)malloc(2 * current_size * sizeof(uint64_t));

          copyToComOutBuffer(current_size, com_out_buffer, &counts);
          counts.clear();

          // char *kmer = (char *)calloc(kmer_size + 1, sizeof(char));
          // for (size_t com_out_buffer_i = 0; com_out_buffer_i < 30; com_out_buffer_i++)
          // {
          //   getKmerFromIndex(kmer_size, com_out_buffer[2 * com_out_buffer_i], kmer);
          //   std::cout << kmer << " -- " << com_out_buffer[2 * com_out_buffer_i + 1] << std::endl;
          // }

          //non blocking send
          MPI_Isend(com_out_buffer, current_size * 2, MPI_UINT64_T, 0, tag, MPI_COMM_WORLD, &sending_request);

          /* if blocking send, use following */
          // MPI_Send(com_out_buffer, current_size * 2, MPI_UINT64_T, 0, tag, MPI_COMM_WORLD);

          com_out_counter++;

          std::cout << rank << " sending data to 0 \t size = " << current_size * 2 << std::endl;
          std::cout << rank << " progress = " << (my_line_counter - my_offset) / ((double)my_allocated_lines_n) << std::endl;
          // tag++;
        }
      }
      memset(read_buffer, 0, READ_BUFFER_SIZE * (sizeof read_buffer[0]));

      my_line_counter++;
    }

    // If sent previously, wait for that sending to complete
    if (com_out_counter > 0)
    {
      MPI_Wait(&sending_request, MPI_STATUS_IGNORE);
      free(com_out_buffer);
    }

    //send any remaining values in hashmap
    if (counts.size() > 0)
    {
      size_t current_size = counts.size();

      com_out_buffer = (uint64_t *)malloc(2 * current_size * sizeof(uint64_t));

      copyToComOutBuffer(current_size, com_out_buffer, &counts);
      counts.clear();
      std::cout << rank << " sending last chunk\t size=" << current_size * 2;
      MPI_Isend(com_out_buffer, current_size * 2, MPI_UINT64_T, 0, tag, MPI_COMM_WORLD, &sending_request);
      com_out_counter++;
      MPI_Wait(&sending_request, MPI_STATUS_IGNORE);
      free(com_out_buffer);
    }

    //send an empty value to signal that this processes has finished counting
    com_out_buffer = (uint64_t *)malloc(2 * sizeof(uint64_t));
    com_out_buffer[0] = 0;
    com_out_buffer[1] = 0; //empty value

    MPI_Isend(com_out_buffer, 2, MPI_UINT64_T, 0, tag, MPI_COMM_WORLD, &sending_request);
    MPI_Wait(&sending_request, MPI_STATUS_IGNORE);
    free(com_out_buffer);

    fclose(file);
    printf("\n");

    std::cout << rank << " hashmap size = " << counts.size();
    std::cout << "\tbucket count = " << counts.bucket_count() << std::endl;

    // google::dense_hash_map<size_t, size_t>::iterator it = counts.begin();

    // char *kmer = (char *)calloc(kmer_size + 1, sizeof(char));
    // for (; it != counts.end(); ++it)
    // {
    //   getKmerFromIndex(kmer_size, it->first, kmer);
    //   std::cout << kmer << " " << it->second << std::endl;
    // }
  }
  if (rank == 0)
  {
    int receiving_size;
    MPI_Status receiving_status;
    uint64_t *com_in_buffer;
    int source;
    int finished_tasks_n = 0;
    google::dense_hash_map<uint64_t, uint64_t> final_counts;
    final_counts.set_empty_key(-1);

    while (finished_tasks_n < num_tasks - 1)
    {
      MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &receiving_status);
      source = receiving_status.MPI_SOURCE;
      MPI_Get_count(&receiving_status, MPI_UINT64_T, &receiving_size);
      std::cout << "receiving size = " << receiving_size << " from " << source << std::endl;

      com_in_buffer = (uint64_t *)malloc(receiving_size * sizeof(uint64_t));

      MPI_Recv(com_in_buffer, receiving_size, MPI_UINT64_T, source, 0, MPI_COMM_WORLD, &receiving_status);
      std::cout << "Process " << rank << " received from " << source << "\tdata size = " << receiving_size << std::endl;

      if (!com_in_buffer[1])
      {
        finished_tasks_n++;
        free(com_in_buffer);
        continue;
      }

      char *kmer = (char *)calloc(kmer_size + 1, sizeof(char));
      for (size_t com_in_buffer_i = 0; com_in_buffer_i < 3; com_in_buffer_i++)
      {
        getKmerFromIndex(kmer_size, com_in_buffer[2 * com_in_buffer_i], kmer);
        std::cout << kmer << " " << com_in_buffer[2 * com_in_buffer_i + 1] << std::endl;
      }
      // mergeArrayToHashmap(com_in_buffer, receiving_size, 1, "data");

      for (int i = 0; i < receiving_size; i += 2)
      {
        if (final_counts[com_in_buffer[i]] > 0)
        {
          final_counts[com_in_buffer[i]] += com_in_buffer[i + 1];
        }
        else
        {
          final_counts[com_in_buffer[i]] = com_in_buffer[i + 1];
        }
      }
      free(com_in_buffer);
    }
    saveHashMap(&final_counts, 1, "data");

    /* 
    Writing kmer dump to a file
    */
    FILE *dump_file;

    dump_file = fopen("dump.txt", "w");
    char *kmer = (char *)calloc(kmer_size + 1, sizeof(char));
    uint64_t dump_size = std::min((uint64_t)DUMP_SIZE, ((uint64_t)2) << (2 * kmer_size - 1));
    for (uint64_t kmer_i = 0; kmer_i < dump_size; kmer_i++)
    {
      getKmerFromIndex(kmer_size, kmer_i, kmer);
      fprintf(dump_file, "%s\t%lu\n", kmer, final_counts[kmer_i]);
    }
    fclose(dump_file);

    final_counts.clear();

    // google::dense_hash_map<uint64_t, uint64_t> loaded_counts;
    // loaded_counts.set_empty_key(-1);

    // loadHashMap(&loaded_counts, 1, "data");
    // google::dense_hash_map<uint64_t, uint64_t>::iterator loaded_counts_iterator = loaded_counts.begin();

    // char *kmer = (char *)calloc(kmer_size + 1, sizeof(char));
    // std::cout << "\nFinal Counts\n===================\n";
    // size_t dump_size = DUMP_SIZE;
    // for (; loaded_counts_iterator != loaded_counts.end(); ++loaded_counts_iterator)
    // {
    //   getKmerFromIndex(kmer_size, loaded_counts_iterator->first, kmer);
    //   std::cout << kmer << " " << loaded_counts_iterator->second << std::endl;
    //   if (!--dump_size)
    //   {
    //     break;
    //   }
    // }
    // loaded_counts.clear();

    // size_t *com_in_buffer;
    // int receiving_size;
    // int tasks_finished_status[num_tasks - 1] = {0};
    // int tasks_probe_flag_status[num_tasks - 1] = {0};
    // MPI_Message tasks_probe_message[num_tasks - 1];
    // MPI_Status tasks_probe_status[num_tasks - 1];
    // int finished_tasks_n = 0;
    // char *kmer = (char *)calloc(kmer_size + 1, sizeof(char));

    // // probe all at first
    // for (size_t i = 1; i < num_tasks; i++)
    // {
    //   MPI_Improbe(i, 0, MPI_COMM_WORLD, &tasks_probe_flag_status[i], &tasks_probe_message[i], &tasks_probe_status[i]);
    //   // MPI_Mprobe(i, 0, MPI_COMM_WORLD, &tasks_probe_message[i], &tasks_probe_status[i]);
    //   std::cout << "initial probing";
    // }

    // while (finished_tasks_n < (num_tasks - 1))
    // {

    //   size_t source_i;
    //   for (source_i = 1; source_i < num_tasks; source_i++)
    //   {
    //     if (!tasks_finished_status[source_i] && tasks_probe_flag_status[source_i])
    //     {
    //       // a message is in the queue from task, get its size and read it
    //       MPI_Get_count(&tasks_probe_status[source_i], MPI_UINT64_T, &receiving_size);
    //       com_in_buffer = (size_t *)malloc(receiving_size * sizeof(size_t));
    //       std::cout << "receiving size = " << receiving_size << " from " << source_i << std::endl;
    //       MPI_Mrecv(com_in_buffer, receiving_size, MPI_UINT64_T,
    //                 &tasks_probe_message[source_i], &tasks_probe_status[source_i]);

    //       for (size_t com_in_buffer_i = 0; com_in_buffer_i < 30; com_in_buffer_i++)
    //       {
    //         getKmerFromIndex(kmer_size, com_in_buffer[2 * com_in_buffer_i], kmer);
    //         std::cout << kmer << " " << com_in_buffer[2 * com_in_buffer_i + 1] << std::endl;
    //       }

    //       // task is finished if it has sent an empty count for first kmer
    //       tasks_finished_status[source_i] = (com_in_buffer[1] == 0);

    //       free(com_in_buffer);

    //       if (!tasks_finished_status[source_i])
    //       {
    //         MPI_Improbe(source_i, 0, MPI_COMM_WORLD, &tasks_probe_flag_status[source_i],
    //                     &tasks_probe_message[source_i], &tasks_probe_status[source_i]);
    //       }
    //       else
    //       {
    //         finished_tasks_n++;
    //       }
    //     }
    //   }
    // }
  }
  std::cout << "finalizing.." << rank << std::endl;
  MPI_Finalize();
  std::cout << "done.." << rank << std::endl;
  return 0;
}

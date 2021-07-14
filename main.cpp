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
#include <stdlib.h>
#include "extractor.hpp"
#include "com.hpp"
#include "kmer_dump.hpp"
#include <stddef.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include "utils.hpp"

#define READ_BUFFER_SIZE 0x200
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
  char read_buffer[READ_BUFFER_SIZE + 1] = {0};
  custom_dense_hash_map counts;

  int num_tasks, rank;
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
  uint64_t offset_data[num_tasks][2]; //offset and allocated size for this process (in bytes)

  std::cout << "total processes = " << num_tasks << std::endl;
  std::cout << "rank = " << rank << std::endl;

  if (rank == 0)
  {
    // {
    //   int i = 0;
    //   while (0 == i)
    //     sleep(5);
    // }

    // Calculating total file size
    struct stat data_file_stats;
    if (stat(file_name, &data_file_stats) != 0)
    {
      std::cerr << "Error when calculating file size." << std::endl;
      exit(EXIT_FAILURE);
    }

    const size_t total_size = data_file_stats.st_size;
    std::cout << "Total file size in bytes = " << total_size << std::endl;
    const uint64_t chunk_size = total_size / (num_tasks - 1);

    // Dummy offset data for rank=0 process since that process is not reading the file
    offset_data[0][0] = 0;
    offset_data[0][1] = 0;

    for (int process_i = 1; process_i < num_tasks; process_i++)
    {
      offset_data[process_i][0] = (process_i - 1) * chunk_size;
      offset_data[process_i][1] = chunk_size;
    }

    offset_data[num_tasks - 1][1] = (total_size - ((num_tasks - 2) * chunk_size));
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

    const uint64_t my_offset = my_offset_data[0];
    uint64_t my_revised_offset = my_offset;
    const uint64_t my_allocated_size = my_offset_data[1];
    int tag = 0;
    clock_t t;

    std::cout << "my (" << rank << ")\t offset = " << my_offset << "\t chunk_size = " << my_allocated_size << std::endl;

    memset(read_buffer, 0, READ_BUFFER_SIZE * (sizeof read_buffer[0]));

    FILE *file = fopen(file_name, "r");

    if (my_offset > 0)
    {
      fseek(file, my_offset - 1, SEEK_SET);
      char temp_char = fgetc(file);
      if (temp_char != '\n')
      {
        // now were are in the middle of a line
        // just do fgets and ignore the current line.
        // that line will be processed by the previous rank process.
        fgets(read_buffer, READ_BUFFER_SIZE, file);
        cout << "ignored first line = \"" << read_buffer << "\"" << endl;
        my_revised_offset = ftell(file);
      }
    }

    enum LineType first_line_type = getLineType(file);

    //return to revised starting point
    fseek(file, my_revised_offset, SEEK_SET);

    memset(read_buffer, 0, READ_BUFFER_SIZE + 1);

    // adjust range size due to ignoring the first line
    const size_t remaining = my_allocated_size - (my_revised_offset - my_offset);
    size_t processed = 0;
    size_t current_chunk_size = 0;
    uint64_t log_counter = 0;

    while (processed <= remaining && !feof(file))
    {
      log_counter++;
      if (log_counter%100000 == 0)
      {
        std::cout << rank << " " << 100* (ftell(file) - my_offset) / ((double)my_allocated_size) <<"%\n";
      }
      

      current_chunk_size = fread(read_buffer, sizeof(char), READ_BUFFER_SIZE, file);

      countKmersFromBuffer(
          kmer_size,
          read_buffer,
          READ_BUFFER_SIZE,
          (processed + current_chunk_size) <= remaining ? current_chunk_size : remaining - processed,
          first_line_type, true, &counts);
      processed += current_chunk_size;
      memset(read_buffer, 0, READ_BUFFER_SIZE + 1);

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
        std::cout << rank << " progress = " << (ftell(file) - my_offset) / ((double)my_allocated_size) << std::endl;
        // tag++;
      }
    }

    // return to one character before the end of allocated range
    fseek(file, my_offset + my_allocated_size - 1, SEEK_SET);
    char temp_char = 0;
    temp_char = fgetc(file);
    if (temp_char != '\n')
    {
      // we are in the middle of a line
      // process the remaining part of the line

      // cout << "\nrading the remaining part of the last line \n";
      fgets(read_buffer, READ_BUFFER_SIZE, file);
      countKmersFromBuffer(
          kmer_size,
          read_buffer,
          READ_BUFFER_SIZE,
          strlen(read_buffer),
          first_line_type, true, &counts);
      memset(read_buffer, 0, READ_BUFFER_SIZE + 1);
    }

    cout << "\n\n===============\nfinal position = " << ftell(file) << std::endl;

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
    custom_dense_hash_map final_counts;
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

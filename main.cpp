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
#include <boost/lockfree/queue.hpp>

#include "utils.hpp"
#include "counter.hpp"
#include "thread_safe_queue.hpp"
#include "writer.hpp"

#define READ_BUFFER_SIZE 0x1000
#define HASH_MAP_MAX_SIZE 0x1000000
#define DUMP_SIZE 10
#define READ_QUEUE_SIZE 10
#define PARTITION_COUNT 10
#define SEGMENT_COUNT 50

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
      out_buffer[i] = 'T';
      break;

    case 3:
      out_buffer[i] = 'G';
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

  int num_tasks, rank;
  // uint64_t my_offset_data[2];

  const char *file_name = argv[1];
  kmer_size = atoi(argv[2]);
  std::string output_file_path = argv[3];

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Status s;
  // uint64_t offset_data[num_tasks][2]; //offset and allocated size for this process (in bytes)

  int namelen;
  char processor_name[MPI_MAX_PROCESSOR_NAME];
  MPI_Get_processor_name(processor_name, &namelen);

  boost::lockfree::queue<struct writerArguments *> writer_queue(10);

  // if (rank == 0)
  //   std::cout << "total processes = " << num_tasks << std::endl;
  // std::cout << "rank = " << rank << " Node: " << processor_name << std::endl;

  std::system(("rm -rf "+output_file_path +"*/*.data").c_str());

  if (rank == 0)
  {
    // Calculating total file size
    struct stat data_file_stats;
    if (stat(file_name, &data_file_stats) != 0)
    {
      std::cerr << "Error when calculating file size." << std::endl;
      exit(EXIT_FAILURE);
    }

    const size_t total_size = data_file_stats.st_size;
    std::cout << "Total file size in bytes = " << total_size << std::endl;
    const uint64_t chunk_size = total_size / SEGMENT_COUNT;

    int node_rank;


    int segmentData[2] = {0, 0};
    int i = 0;
    while(i < SEGMENT_COUNT) 
    {   
      MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      segmentData[0] = i*chunk_size;
      segmentData[1] = chunk_size;

      if (i == (SEGMENT_COUNT-1)) 
      {
        segmentData[1] = (total_size - ((SEGMENT_COUNT - 1) * chunk_size));
      }
      MPI_Send(segmentData, 2, MPI_INT, node_rank, 1, MPI_COMM_WORLD);
      i++;
      cout <<"overall progress : " << 100 * i / ((double)SEGMENT_COUNT) << "%\n";
    }
  
    for (int j =1; j < num_tasks; j++) {
      MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      segmentData[0]=0;
      segmentData[1]=0;
      MPI_Send(segmentData, 2, MPI_INT, node_rank, 1, MPI_COMM_WORLD);
      cout << "Finish Sending Allocations to "  << node_rank << endl; 
    }

    // for (int process_i = 1; process_i < num_tasks; process_i++)
    // {
    //   offset_data[process_i][0] = (process_i - 1) * chunk_size;
    //   offset_data[process_i][1] = chunk_size;
    // }

    // offset_data[0][0] = 0;
    // offset_data[0][1] = 0;

    // offset_data[num_tasks - 1][1] = (total_size - ((num_tasks - 2) * chunk_size)); // last ranked procedd will do all remaining work
  }

  // MPI_Scatter(offset_data, 2, MPI_UINT64_T, my_offset_data, 2,
  //             MPI_UINT64_T, 0, MPI_COMM_WORLD);

  /* wait until attaching debugger*/
  // if (rank == 1)
  // {
  //   int i = 0;
  //   while (0 == i)
  //     sleep(5);
  // }

  // Counting step starts from here

  // int tag = 0;
  // clock_t t;

  
  if (rank > 0)
  {
    Counter counter(kmer_size, READ_BUFFER_SIZE, READ_QUEUE_SIZE, &writer_queue, PARTITION_COUNT);
    Writer writer("data", &writer_queue, PARTITION_COUNT, rank, output_file_path);

    int currentAllocations[2];

    FILE *file = fopen(file_name, "r");

    while(true) {
      MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Recv(currentAllocations, 2, MPI_INT, 0, 1, MPI_COMM_WORLD, &s);

      if (currentAllocations[1] == 0) {
        break;
      }

      // cout << rank <<" Current Allocations : " << currentAllocations[0] << endl;

      uint64_t my_offset = currentAllocations[0];
      uint64_t my_revised_offset = my_offset;
      uint64_t my_allocated_size = currentAllocations[1];

      cout << "my (" << rank << ")\t offset = " << my_offset << "\t chunk_size = " << my_allocated_size << endl;

      memset(read_buffer, 0, READ_BUFFER_SIZE * (sizeof read_buffer[0]));

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
          // cout << "ignored first line = \"" << read_buffer << "\"" << endl;
          my_revised_offset = ftell(file);
        }
      }

      enum LineType first_line_type = getLineType(file);

      //return to revised starting point
      fseek(file, my_revised_offset, SEEK_SET);

      // adjust range size due to ignoring the first line
      const size_t remaining = my_allocated_size - (my_revised_offset - my_offset);
      size_t processed = 0;
      size_t current_chunk_size = 0;
      bool reset_status = true;

      // cout << "Hello" << endl;
      while (processed <= remaining && !feof(file))
      {
        
        char *read_buffer2 = (char *)malloc((READ_BUFFER_SIZE + 1) * sizeof(char));
        current_chunk_size = fread(read_buffer2, sizeof(char), READ_BUFFER_SIZE, file);

        struct counterArguments *args = (struct counterArguments *)malloc(sizeof(struct counterArguments));
        args->allowed_length = (processed + current_chunk_size) <= remaining ? current_chunk_size : remaining - processed;
        args->buffer = read_buffer2;
        args->first_line_type = first_line_type;
        args->reset_status = reset_status;

        counter.enqueue(args);
        reset_status = false;

        processed += current_chunk_size;
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
        char *read_buffer2 = (char *)malloc((READ_BUFFER_SIZE + 1) * sizeof(char));
        fgets(read_buffer2, READ_BUFFER_SIZE, file);

        struct counterArguments *args = (struct counterArguments *)malloc(sizeof(struct counterArguments));
        args->allowed_length = strlen(read_buffer);
        args->buffer = read_buffer2;
        args->first_line_type = first_line_type;
        args->reset_status = reset_status;

        counter.enqueue(args);
      }
    } 
    counter.explicitStop();
    writer.explicitStop();
    printf("\n");
    fclose(file);
    }
  std::cout << "finalizing.." << rank << std::endl;
  MPI_Finalize();
  std::cout << "done.." << rank << std::endl;
  return 0;
}
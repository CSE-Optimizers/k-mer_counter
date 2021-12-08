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
#include <sys/time.h>
#include <boost/lockfree/queue.hpp>

#include "utils.hpp"
#include "counter.hpp"
#include "thread_safe_queue.hpp"
#include "writer.hpp"

// #define READ_BUFFER_SIZE 0x1000
#define HASH_MAP_MAX_SIZE 0x1000000
#define DUMP_SIZE 10
#define READ_QUEUE_SIZE 10
#define PARTITION_COUNT 10
#define SEGMENT_COUNT 0x100
#define MAX_LINE_LENGTH 2050

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

float time_diffs2(struct timeval *start, struct timeval *end) {
  return (end->tv_sec - start->tv_sec) + 1e-6*(end->tv_usec - start->tv_usec);
};

int main(int argc, char *argv[])
{

  struct timeval start_time, end_time;
  float total_time_to_read = 0;
  float total_time_to_communicate = 0;


  int kmer_size;

  int num_tasks, rank;

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
  uint64_t total_max_buffer_size;
  uint64_t chunk_size;

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
    chunk_size = total_size / SEGMENT_COUNT;
    total_max_buffer_size = chunk_size + MAX_LINE_LENGTH + 1;
    std::cout << "Max buffer size = " << total_max_buffer_size << std::endl;

  }

  // send the calculated segment size
  MPI_Bcast(&total_max_buffer_size, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
  // std::cout << "(" << rank << ") total_max_buffer_size = " << total_max_buffer_size << std::endl;

  if (rank == 0){

    FILE *file = fopen(file_name, "r");

    int node_rank;

    struct FileChunkData* file_chunk_data = (struct FileChunkData*)malloc(sizeof(int32_t)+total_max_buffer_size*sizeof(char));
    uint64_t file_position_before_reading;
    uint64_t current_chunk_size;
    uint64_t log_counter = 0;
    while (!feof(file)){
      gettimeofday(&start_time,NULL);

      file_position_before_reading = ftell(file);
      file_chunk_data->first_line_type = getLineType(file);    // this function changes the file pointer position
      fseek(file, file_position_before_reading, SEEK_SET);  // reset the file pointer
      memset(file_chunk_data->chunk_buffer, 0, total_max_buffer_size);
      current_chunk_size = fread(file_chunk_data->chunk_buffer, sizeof(char), chunk_size, file);   
      fgets(&(file_chunk_data->chunk_buffer[current_chunk_size]), MAX_LINE_LENGTH, file);  // read the remaining part of the last line of the chunk
      current_chunk_size += strlen(&(file_chunk_data->chunk_buffer[current_chunk_size]));

      gettimeofday(&end_time,NULL);
      total_time_to_read+=time_diffs2(&start_time, &end_time);

      if(file_chunk_data->chunk_buffer[total_max_buffer_size-1]!=0){
        std::cerr << "Buffer overflow when reading lines" << std::endl;
        exit(EXIT_FAILURE);
      }

      MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      gettimeofday(&start_time,NULL);
      MPI_Send(file_chunk_data, sizeof(int32_t)+total_max_buffer_size*sizeof(char), MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);
      gettimeofday(&end_time,NULL);
      total_time_to_communicate+=time_diffs2(&start_time, &end_time);
      // cout << "time to send = " <<  total_time<< endl;
      // cout << "sending to "<< node_rank << endl;

      // std::cout << "sent line type = " << file_chunk_data->first_line_type << std::endl;
      log_counter++;
      // if (log_counter % 100 == 0)
      // {
        cout <<"overall progress : " << 100 * ftell(file) / ((double)(chunk_size*SEGMENT_COUNT)) << "%\n";
      // }
      
    }

    // send empty buffer as the last message
    memset(file_chunk_data->chunk_buffer, 0, total_max_buffer_size);

  
    for (int j =1; j < num_tasks; j++) {
      MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(file_chunk_data, sizeof(int32_t)+total_max_buffer_size*sizeof(char), MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);

      cout << "Finish Sending Allocations to "  << node_rank << endl; 
    }
    fclose(file);
    cout << "total time to read = " <<  total_time_to_read<< endl;
    cout << "total time to communicate = " <<  total_time_to_communicate<< endl;

  }

  
  if (rank > 0)
  {
    std::cout << "(" << rank << ") total_max_buffer_size = " << total_max_buffer_size << std::endl;
    Counter counter(kmer_size, total_max_buffer_size, READ_QUEUE_SIZE, &writer_queue, PARTITION_COUNT);
    Writer writer("data", &writer_queue, PARTITION_COUNT, rank, output_file_path);

    // int currentAllocations[2];
    struct FileChunkData* file_chunk_data;


    while(true) {
      file_chunk_data = (struct FileChunkData*)malloc(sizeof(int32_t)+total_max_buffer_size*sizeof(char));
      MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Recv(file_chunk_data, sizeof(int32_t)+total_max_buffer_size*sizeof(char), MPI_BYTE, 0, 1, MPI_COMM_WORLD, &s);

      if (file_chunk_data->chunk_buffer[0] == 0) {
        // last empty message received
        break;
      }

        
      char *data_buffer = (char *)malloc(total_max_buffer_size * sizeof(char));
      memcpy(data_buffer, file_chunk_data->chunk_buffer, total_max_buffer_size); //copy to a new buffer


      struct counterArguments *args = (struct counterArguments *)malloc(sizeof(struct counterArguments));
      args->allowed_length = total_max_buffer_size;
      args->buffer = data_buffer;
      args->first_line_type = LineType(file_chunk_data->first_line_type);
      args->reset_status = true;

      counter.enqueue(args);
      free(file_chunk_data);

      
    } 
    counter.explicitStop();
    writer.explicitStop();
    printf("\n");
    
    }
  std::cout << "finalizing.." << rank << std::endl;
  MPI_Finalize();
  std::cout << "done.." << rank << std::endl;
  return 0;
}
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
#include "file_reader.hpp"

// #define READ_BUFFER_SIZE 0x1000
#define HASH_MAP_MAX_SIZE 0x1000000
#define DUMP_SIZE 10
#define READ_QUEUE_SIZE 10
#define MASTER_FILE_QUEUE_SIZE 40
#define PARTITION_COUNT 10
#define SEGMENT_COUNT 0x400
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
    ThreadSafeQueue <struct FileChunkData> master_file_queue;
    master_file_queue.setLimit(MASTER_FILE_QUEUE_SIZE);


    FileReader fileReader(file_name, chunk_size, total_max_buffer_size, MAX_LINE_LENGTH, &master_file_queue);


    int node_rank;

    struct FileChunkData* file_chunk_data;

    uint64_t log_counter = 0;
    while (true){

      file_chunk_data = master_file_queue.dequeue();
      if(file_chunk_data!=NULL){
        MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(file_chunk_data, sizeof(int32_t)+total_max_buffer_size*sizeof(char), MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);
        free(file_chunk_data);
        log_counter++;
        cout <<"overall progress : " << 100 * log_counter / ((double)(SEGMENT_COUNT)) << "%\n";

      }


      if (fileReader.isCompleted() && master_file_queue.isEmpty())
      {
          break;
      }
      
    }
    fileReader.finish();

    // send empty buffer as the last message
    file_chunk_data= (struct FileChunkData*)malloc(sizeof(int32_t)+total_max_buffer_size*sizeof(char));
    memset(file_chunk_data->chunk_buffer, 0, total_max_buffer_size);

  
    for (int j =1; j < num_tasks; j++) {
      MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(file_chunk_data, sizeof(int32_t)+total_max_buffer_size*sizeof(char), MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);

      cout << "Finish Sending Allocations to "  << node_rank << endl; 
    }
    free(file_chunk_data);

    // cout << "total time to read = " <<  total_time_to_read<< endl;
    // cout << "total time to communicate = " <<  total_time_to_communicate<< endl;

  }

  
  if (rank > 0)
  {
    std::cout << "(" << rank << ") total_max_buffer_size = " << total_max_buffer_size << std::endl;
    Counter counter(kmer_size, total_max_buffer_size, READ_QUEUE_SIZE, &writer_queue, PARTITION_COUNT);
    Writer writer("data", &writer_queue, PARTITION_COUNT, rank, output_file_path);


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


      struct CounterArguments *args = (struct CounterArguments *)malloc(sizeof(struct CounterArguments));
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
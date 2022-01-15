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
#define MASTER_FILE_QUEUE_SIZE 400
#define PARTITION_COUNT 10
// #define SEGMENT_COUNT 0x400
#define COM_BUFFER_SIZE 0x40000    //0x40000 = 256KB
#define MAX_LINE_LENGTH 103000

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


float time_diffs3(struct timeval *y, struct timeval *x)
{
    struct timeval result;

    if (x->tv_usec > 999999)
    {
        x->tv_sec += x->tv_usec / 1000000;
        x->tv_usec %= 1000000;
    }

    if (y->tv_usec > 999999)
    {
        y->tv_sec += y->tv_usec / 1000000;
        y->tv_usec %= 1000000;
    }

    result.tv_sec = x->tv_sec - y->tv_sec;

    if ((result.tv_usec = x->tv_usec - y->tv_usec) < 0)
    {
        result.tv_usec += 1000000;
        result.tv_sec--; // borrow
    }

    return result.tv_sec*1.0 + (1e-6)*result.tv_usec;
}



int main(int argc, char *argv[])
{

  struct timeval start_time, end_time;

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

  size_t total_file_size;
  size_t com_buffer_size = COM_BUFFER_SIZE;

  if (rank == 0)
  {
    // Calculating total file size
    struct stat data_file_stats;
    if (stat(file_name, &data_file_stats) != 0)
    {
      std::cerr << "Error when calculating file size." << std::endl;
      exit(EXIT_FAILURE);
    }

    total_file_size = data_file_stats.st_size;
    std::cout << "Total file size in bytes = " << total_file_size << std::endl;

    // std::cout << "Com buffer size =  " << com_buffer_size/1024 << " KB" << std::endl;

  }

  // send the calculated segment size
  MPI_Bcast(&com_buffer_size, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);
  std::cout << "(" << rank << ") com buffer size = " << com_buffer_size << std::endl;

  if (rank == 0){
    ThreadSafeQueue <char> master_file_queue;
    master_file_queue.setLimit(MASTER_FILE_QUEUE_SIZE);


    FileReader fileReader(file_name, COM_BUFFER_SIZE, MAX_LINE_LENGTH, &master_file_queue, total_file_size);


    int node_rank;

    char* send_buffer;

    uint64_t log_counter = 0;
    while (true){

      send_buffer = master_file_queue.dequeue();
      if(send_buffer != NULL){
        gettimeofday(&start_time,NULL);
        MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Send(send_buffer, COM_BUFFER_SIZE, MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);
        gettimeofday(&end_time,NULL);
        total_time_to_communicate += time_diffs3(&start_time, &end_time);
        
        free(send_buffer);
        log_counter++;
        // cout <<"overall progress : " << 100 * log_counter / ((double)(SEGMENT_COUNT)) << "%\n";

      }


      if (fileReader.isCompleted() && master_file_queue.isEmpty())
      {
          break;
      }
      
    }
    fileReader.finish();

    // send empty buffer as the last message
    send_buffer = (char* )malloc(COM_BUFFER_SIZE);
    memset(send_buffer, 0, COM_BUFFER_SIZE);

  
    for (int j =1; j < num_tasks; j++) {
      MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(send_buffer, COM_BUFFER_SIZE, MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);

      cout << "Finish Sending Allocations to "  << node_rank << endl; 
    }
    free(send_buffer);


    cout << "total time to communicate = " <<  total_time_to_communicate<< endl;

  }

  
  if (rank > 0)
  {
    std::cout << "(" << rank << ") com buffer size = " << com_buffer_size/1024 << " KB" << std::endl;
    Counter counter(kmer_size, com_buffer_size, READ_QUEUE_SIZE, &writer_queue, PARTITION_COUNT);
    Writer writer("data", &writer_queue, PARTITION_COUNT, rank, output_file_path);


    char* recv_buffer;
    


    while(true) {
      recv_buffer = (char* )malloc(com_buffer_size);
      assert(recv_buffer);
      MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
      MPI_Recv(recv_buffer, com_buffer_size, MPI_BYTE, 0, 1, MPI_COMM_WORLD, &s);

      if (recv_buffer[0] == 0) {
        // last empty message received
        break;
      }

        
      // char *data_buffer = (char *)malloc(total_max_buffer_size * sizeof(char));
      // memcpy(data_buffer, file_chunk_data->chunk_buffer, total_max_buffer_size); //copy to a new buffer


      struct CounterArguments *args = (struct CounterArguments *)malloc(sizeof(struct CounterArguments));
      args->allowed_length = com_buffer_size;
      args->buffer = recv_buffer;
      // args->first_line_type = NULL;
      args->reset_status = true;

      counter.enqueue(args);
      // free(file_chunk_data);

      
    } 
    free(recv_buffer);
    counter.explicitStop();
    writer.explicitStop();
    printf("\n");
    
    }
  std::cout << "finalizing.." << rank << std::endl;
  MPI_Finalize();
  std::cout << "done.." << rank << std::endl;
  return 0;
}
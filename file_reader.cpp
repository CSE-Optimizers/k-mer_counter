#include <thread>
#include <string>
#include <iostream>
#include <sys/time.h>

#include "thread_safe_queue.hpp"
#include "file_reader.hpp"
#include "extractor.hpp"
#include "utils.hpp"

float time_diffs2(struct timeval *start, struct timeval *end) {
  return (end->tv_sec - start->tv_sec) + 1e-6*(end->tv_usec - start->tv_usec);
};

FileReader::FileReader(const char* file_name, uint64_t chunk_size, uint64_t max_total_size, uint64_t max_line_length,
                     ThreadSafeQueue <struct FileChunkData> *read_chunk_queue)
{
    this->file_name = file_name;
    this->chunk_size = chunk_size;
    this->max_total_size = max_total_size;
    this->max_line_length = max_line_length;

    this->q = read_chunk_queue;

    this->start();
}

FileReader::~FileReader()
{
    return;
}


void FileReader::start()
{
    runner = std::thread(
        [=]
        {
            FILE *file = fopen(this->file_name, "r");
            uint64_t file_position_before_reading;
            uint64_t current_chunk_size;
            struct FileChunkData* file_chunk_data;

            float total_time_to_read = 0;
            struct timeval start_time, end_time;
            while (!feof(file))
            {
                gettimeofday(&start_time,NULL);
                file_chunk_data = (struct FileChunkData*)malloc(sizeof(int32_t)+this->max_total_size*sizeof(char));
                memset(file_chunk_data->chunk_buffer, 0, this->max_total_size);

                file_position_before_reading = ftell(file);
                file_chunk_data->first_line_type = getLineType(file);    // this function changes the file pointer position
                fseek(file, file_position_before_reading, SEEK_SET);  // reset the file pointer

                current_chunk_size = fread(file_chunk_data->chunk_buffer, sizeof(char), this->chunk_size, file);   
                fgets(&(file_chunk_data->chunk_buffer[current_chunk_size]), this->max_line_length, file);  // read the remaining part of the last line of the chunk
                current_chunk_size += strlen(&(file_chunk_data->chunk_buffer[current_chunk_size]));

                gettimeofday(&end_time,NULL);
                total_time_to_read+=time_diffs2(&start_time, &end_time);

                if(file_chunk_data->chunk_buffer[max_total_size-1]!=0){
                    std::cerr << "Buffer overflow when reading lines" << std::endl;
                    exit(EXIT_FAILURE);
                }
                this->q->enqueue(file_chunk_data);
                
            }
            fclose(file);

            this->finished = true;
            std::cout << "File reading finished" << std::endl;
            std::cout << "total time to read = " <<  total_time_to_read<< std::endl;
        });
}

bool FileReader::isCompleted() 
{
    
    return this->finished;
    
}

void FileReader::finish() noexcept
{
    
    runner.join();
    
}
#include <thread>
#include <string>
#include <iostream>
#include <sys/time.h>

#include "thread_safe_queue.hpp"
#include "file_reader.hpp"
#include "extractor.hpp"
#include "utils.hpp"

float time_diffs2(struct timeval *y, struct timeval *x)
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

FileReader::FileReader(const char* file_name, size_t max_buffer_size, size_t max_line_length,
                     ThreadSafeQueue <char> *read_chunk_queue, size_t total_file_size)
{
    this->file_name = file_name;
    this->max_buffer_size = max_buffer_size;
    this->max_line_length = max_line_length;

    this->q = read_chunk_queue;
    this->total_file_size = total_file_size;

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
            // uint64_t file_position_before_reading;
            // uint64_t current_chunk_size;
            size_t filled_length = 0;
            size_t current_line_length = 0;
            char* combined_buffer = (char* )malloc(this->max_buffer_size);
            char* line_read_buffer = (char* )malloc(this->max_line_length);
            assert(combined_buffer);
            assert(line_read_buffer);
            memset(combined_buffer, 0, this->max_buffer_size);
            memset(line_read_buffer, 0, this->max_line_length);

            float total_time_to_read = 0;
            struct timeval start_time, end_time;
            size_t log_counter = 0;
            int line_counter = 0;

            while (!feof(file))
            {
                gettimeofday(&start_time,NULL);
                memset(line_read_buffer, 0, current_line_length + 1);       //+1 just to be safe
                fgets(line_read_buffer, this->max_line_length, file);
                gettimeofday(&end_time,NULL);

                total_time_to_read+=time_diffs2(&start_time, &end_time);

                if(line_read_buffer[this->max_line_length - 1] != 0){
                    std::cerr << "Buffer overflow when reading lines at master node" << std::endl;
                    exit(EXIT_FAILURE);
                }
                current_line_length = strlen(line_read_buffer);
                if (line_counter != 1)
                {
                    // not a sequence line

                    line_counter = (line_counter + 1)%4;
                    continue;
                }
                

                
                if ((current_line_length + 1) > (this->max_buffer_size - filled_length))
                {
                    // not sufficient space in combined buffer, add it to the queue
                    this->q->enqueue(combined_buffer);

                    // create a new combined buffer
                    combined_buffer = (char* )malloc(this->max_buffer_size);
                    assert(combined_buffer);
                    memset(combined_buffer, 0, this->max_buffer_size);
                    filled_length = 0;
                    
                }


                /*
                now definitely combined buffer should have enough space since
                we are defining constants s.t. max_buffer_size >> max_line_length
                */

                memcpy(&combined_buffer[filled_length], line_read_buffer, current_line_length);
                filled_length += current_line_length;

                line_counter = (line_counter + 1)%4;
                log_counter++;

                if (log_counter % 0x80000 == 0)
                {
                    std::cout << "File read progress " << 100 * (ftell(file) / ((double)this->total_file_size)) << "%\n";
                }
                
            }

            // enqueue any remainig data in combined_buffer
            if (filled_length > 0)
            {
                this->q->enqueue(combined_buffer);
            }

            free(line_read_buffer);
            

            
            // while (!feof(file))
            // {
            //     gettimeofday(&start_time,NULL);
            //     file_chunk_data = (struct FileChunkData*)malloc(sizeof(int32_t)+this->max_total_size*sizeof(char));
            //     memset(file_chunk_data->chunk_buffer, 0, this->max_total_size);

            //     file_position_before_reading = ftell(file);
            //     file_chunk_data->first_line_type = getLineType(file);    // this function changes the file pointer position
            //     fseek(file, file_position_before_reading, SEEK_SET);  // reset the file pointer

            //     current_chunk_size = fread(file_chunk_data->chunk_buffer, sizeof(char), this->chunk_size, file);   
            //     fgets(&(file_chunk_data->chunk_buffer[current_chunk_size]), this->max_line_length, file);  // read the remaining part of the last line of the chunk
            //     current_chunk_size += strlen(&(file_chunk_data->chunk_buffer[current_chunk_size]));

            //     gettimeofday(&end_time,NULL);
            //     total_time_to_read+=time_diffs2(&start_time, &end_time);

            //     if(file_chunk_data->chunk_buffer[max_total_size-1]!=0){
            //         std::cerr << "Buffer overflow when reading lines" << std::endl;
            //         exit(EXIT_FAILURE);
            //     }
            //     this->q->enqueue(file_chunk_data);
                
            // }
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
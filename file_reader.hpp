#include <thread>
#include <string>
#include "thread_safe_queue.hpp"
#include "utils.hpp"

#ifndef FILE_READER_H
#define FILE_READER_H

class FileReader
{
public:
    explicit FileReader(const char* file_name, size_t max_buffer_size, size_t max_line_length,
                     ThreadSafeQueue <char> *read_chunk_queue, size_t total_file_size);

    ~FileReader();

    bool isCompleted();

    void finish() noexcept;



private:
    const char* file_name;
    size_t max_buffer_size;
    size_t max_line_length;
    std::thread runner;
    ThreadSafeQueue <char>* q;
    bool finished = false;
    size_t total_file_size;
    
    void start();

    
};

#endif
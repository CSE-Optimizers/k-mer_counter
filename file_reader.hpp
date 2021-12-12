#include <thread>
#include <string>
#include "thread_safe_queue.hpp"
#include "utils.hpp"

#ifndef FILE_READER_H
#define FILE_READER_H

class FileReader
{
public:
    explicit FileReader(const char* file_name, uint64_t chunk_size, uint64_t max_total_size, uint64_t max_line_length,
                     ThreadSafeQueue <struct FileChunkData> *read_chunk_queue);

    ~FileReader();

    bool isCompleted();

    void finish() noexcept;



private:
    const char* file_name;
    uint64_t chunk_size;
    uint64_t max_total_size;
    uint64_t max_line_length;
    std::thread runner;
    ThreadSafeQueue <struct FileChunkData>* q;
    bool finished = false;
    
    void start();

    
};

#endif
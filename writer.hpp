#include <thread>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"
#include <string>

#ifndef WRITER_H
#define WRITER_H

class Writer
{
public:
    explicit Writer(std::string file_path,
                     boost::lockfree::queue<struct writerArguments*> *writer_queue,
                     int partition_count,
                     int rank,
                     std::string base_path);

    ~Writer();

    void explicitStop();

private:
    std::thread runner;
    bool finished = false;
    std::string file_path;
    boost::lockfree::queue<struct writerArguments*> *writer_queue;
    int partition_count;
    int *file_counts;
    int rank;
    std::string base_path;

    void start();

    void stop() noexcept;
};

#endif
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
    explicit Writer(std::string output_base_path,
               boost::lockfree::queue<struct WriterArguments *> *writer_queue,
               int partition_count,
               int rank);

    ~Writer();

    void explicitStop();

private:
    std::thread runner;
    bool finished = false;
    std::string output_base_path;
    boost::lockfree::queue<struct WriterArguments*> *writer_queue;
    int partition_count;
    int rank;
    size_t total_distinct_count;

    void start();

    void stop() noexcept;
};

#endif
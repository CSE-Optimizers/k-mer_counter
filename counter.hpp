#include <thread>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"

#ifndef THREADER_H
#define THREADER_H

class Counter
{
public:
    explicit Counter(uint64_t k,
                     uint64_t buffer_size,
                     int read_queue_size,
                     boost::lockfree::queue<struct writerArguments*> *writer_queue,
                     int partition_count);

    ~Counter();

    void enqueue(struct CounterArguments *args);

    void explicitStop();

private:
    std::thread runner;
    ThreadSafeQueue <struct CounterArguments> q;
    bool finished = false;
    uint64_t k;
    uint64_t buffer_size;
    custom_dense_hash_map **counts;
    boost::lockfree::queue<struct writerArguments*> *writer_queue;
    int partition_count;
    
    void start();

    void stop() noexcept;
};

#endif
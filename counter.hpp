#include <thread>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"

#ifndef COUNTER_H
#define COUNTER_H

class Counter
{
public:
    explicit Counter(uint64_t k,
                     int bin_queue_size,
                     boost::lockfree::queue<struct WriterArguments *> *writer_queue,
                     int partition_count);

    ~Counter();

    void enqueue(struct KmerBin *args);

    void explicitStop();

private:
    std::thread runner;
    ThreadSafeQueue<struct KmerBin> q;
    bool finished = false;
    uint64_t k;
    custom_dense_hash_map *current_hashmap;
    boost::lockfree::queue<struct WriterArguments *> *writer_queue;
    int partition_count;
    int current_partition;

    void start();

    void stop() noexcept;
};

#endif
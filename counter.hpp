#include <thread>

#include "thread_safe_queue.hpp"

#ifndef THREADER_H
#define THREADER_H

class Counter
{
public:
    explicit Counter(uint64_t k,
                     uint64_t buffer_size,
                     custom_dense_hash_map *counts,
                     int read_queue_size);

    ~Counter();

    void enqueue(struct counterArguments *args);

    void explicitStop();

private:
    std::thread runner;
    ThreadSafeQueue q;
    bool finished = false;
    uint64_t k;
    uint64_t buffer_size;
    custom_dense_hash_map *counts;

    void start();

    void stop() noexcept;
};

#endif
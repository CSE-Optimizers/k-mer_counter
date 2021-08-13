#include <thread>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"
#include "counter.hpp"

Counter::Counter(uint64_t k,
                 uint64_t buffer_size,
                 custom_dense_hash_map *counts)
{
    this->k = k;
    this->buffer_size = buffer_size;
    this->counts = counts;
    start();
}

Counter::~Counter()
{
    stop();
}

void Counter::enqueue(struct counterArguments *args)
{
    q.enqueue(args);
}

void Counter::explicitStop()
{
    finished = true;
    stop();
}

void Counter::start()
{
    runner = std::thread(
        [=]
        {
            struct counterArguments *args;
            while (true)
            {
                if (finished && q.isEmpty())
                {
                    break;
                }

                if (!q.isEmpty())
                {
                    args = q.dequeue();
                }

                std::cout << args->buffer << std::endl;
                free(args);
            }
        });
}

void Counter::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }
}
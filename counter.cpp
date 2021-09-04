#include <thread>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"
#include "counter.hpp"

Counter::Counter(uint64_t k,
                 uint64_t buffer_size,
                 custom_dense_hash_map *counts,
                 int read_queue_size)
{
    q.setLimit(read_queue_size);
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
                args = q.dequeue();

                if (args != NULL)
                {
                    // std::cout << args->buffer << std::endl;
                    countKmersFromBuffer(
                        k,
                        args->buffer,
                        buffer_size,
                        args->allowed_length,
                        args->first_line_type, true, counts);

                    free(args->buffer);
                    free(args);
                }

                if (finished && q.isEmpty())
                {
                    break;
                }
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
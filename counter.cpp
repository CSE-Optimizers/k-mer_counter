#include <thread>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"
#include "counter.hpp"

Counter::Counter(uint64_t k,
                 uint64_t buffer_size,
                 int read_queue_size,
                 boost::lockfree::queue<struct writerArguments*> *writer_queue,
                 int partition_count)
{
    q.setLimit(read_queue_size);
    this->k = k;
    this->buffer_size = buffer_size;
    this->writer_queue = writer_queue;
    this->partition_count = partition_count;
    this->counts = (custom_dense_hash_map**)malloc(partition_count*sizeof(custom_dense_hash_map**));
    
    for(int i=0; i<partition_count; i++){
        this->counts[i] = new custom_dense_hash_map;
        this->counts[i]->set_empty_key(-1);
    }

    start();
}

Counter::~Counter()
{
    if(!finished)
        stop();
}

void Counter::enqueue(struct CounterArguments *args)
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
            struct CounterArguments *args;
            while (true)
            {
                args = q.dequeue();

                if (args != NULL)
                {
                    countKmersFromBufferWithPartitioning(
                        k,
                        args->buffer,
                        buffer_size,
                        args->allowed_length,
                        args->first_line_type,
                        args->reset_status, counts, partition_count, writer_queue);

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
    
    for(int i=0; i<partition_count; i++){
        struct writerArguments * this_writer_argument;
        this_writer_argument = new writerArguments;
        this_writer_argument->partition = i;
        this_writer_argument->counts = counts[i];
    
        writer_queue->push(this_writer_argument);
    }
}
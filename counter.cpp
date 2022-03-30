#include <thread>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"
#include "counter.hpp"

Counter::Counter(uint64_t k,
                 int bin_queue_size,
                 boost::lockfree::queue<struct WriterArguments *> *writer_queue,
                 int partition_count)
{
    q.setLimit(bin_queue_size);
    this->k = k;
    this->writer_queue = writer_queue;
    this->partition_count = partition_count;
    this->current_hashmap = new custom_dense_hash_map;
    this->current_hashmap->set_empty_key(-1);
    this->current_partition = 0;

    start();
}

Counter::~Counter()
{
    if (!finished)
        stop();
}

void Counter::enqueue(struct KmerBin *args)
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
            struct KmerBin *bin;
            while (true)
            {
                bin = q.dequeue();

                if (bin != NULL)
                {
                    if (bin->id != this->current_partition)
                    {
                        assert(bin->id == (this->current_partition + 1));

                        // prev partition is finished. dump it to the writer queue
                        struct WriterArguments *writer_argument = (struct WriterArguments*) malloc(sizeof(struct WriterArguments));
                        writer_argument->partition = this->current_partition;
                        writer_argument->counts = this->current_hashmap;

                        writer_queue->push(writer_argument);

                        this->current_hashmap = new custom_dense_hash_map;
                        this->current_hashmap->set_empty_key(-1);
                        this->current_partition = bin->id;
                    }

                    for (size_t kmer_i = 0; kmer_i < bin->filled_size; kmer_i++)
                    {
                        (*this->current_hashmap)[bin->bin[kmer_i]]++;
                    }

                    free(bin->bin);
                    free(bin);
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

    //dump last hashmap

    struct WriterArguments *writer_argument = new WriterArguments;
    writer_argument->partition = this->current_partition;
    writer_argument->counts = this->current_hashmap;

    writer_queue->push(writer_argument);
}
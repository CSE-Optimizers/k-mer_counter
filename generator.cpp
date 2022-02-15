#include <thread>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"
#include "generator.hpp"

Generator::Generator(uint64_t kmer_size, uint32_t bases_per_block,
                     int read_queue_size, int partition_count, size_t bin_size,
                     boost::lockfree::queue<struct KmerBin *> *writer_queue)
{
    q.setLimit(read_queue_size);
    this->bases_per_block = bases_per_block;
    this->kmer_size = kmer_size;
    this->partition_count = partition_count;

    this->writer_queue = writer_queue;
    this->bin_size = bin_size;
    this->bins = (KmerBin **)malloc(partition_count * sizeof(struct KmerBin *));
    for (int i = 0; i < this->partition_count; i++)
    {
        this->bins[i] = (KmerBin *)malloc(sizeof(struct KmerBin));
        assert(this->bins[i]);
        this->bins[i]->id = i;
        this->bins[i]->bin = (uint64_t *)malloc(bin_size * sizeof(uint64_t));
        assert(this->bins[i]->bin);
        memset(this->bins[i]->bin, 0, bin_size * sizeof(uint64_t));
        this->bins[i]->filled_size = 0;
    }

    start();
}

Generator::~Generator()
{
    if (!finished)
        stop();
}

void Generator::enqueue(struct GeneratorArguments *args)
{
    q.enqueue(args);
}

void Generator::explicitStop()
{
    finished = true;
    stop();
}

void Generator::start()
{
    runner = std::thread(
        [=]
        {
            struct GeneratorArguments *args;
            while (true)
            {
                args = q.dequeue();

                if (args != NULL)
                {
                    generateKmersWithPartitioning(
                        this->kmer_size,
                        args->encoded_read,
                        args->bases_per_block,
                        args->read_length,
                        args->block_count,
                        partition_count, this->bins, this->bin_size, writer_queue);
                    free(args->encoded_read);
                    free(args);
                }

                if (finished && q.isEmpty())
                {
                    break;
                }
            }
        });
}

void Generator::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }
    for (int i = 0; i < this->partition_count; i++)
    {
        if (this->bins[i]->filled_size > 0)
        {
            this->writer_queue->push(this->bins[i]);
        }
    }
    free(this->bins);
}
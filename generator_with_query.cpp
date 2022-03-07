
#include "generator_with_query.hpp"

GeneratorWithQuery::GeneratorWithQuery(
    int kmer_size,
    int read_queue_size,
    boost::lockfree::queue<uint64_t> *kmer_queue)
{
    this->q.setLimit(read_queue_size);
    this->kmer_size = kmer_size;
    this->kmer_queue = kmer_queue;

    start();
}

GeneratorWithQuery::~GeneratorWithQuery()
{
    if (!finished)
    {
        stop();
    }
}

void GeneratorWithQuery::enqueue(struct GeneratorArguments *args)
{
    q.enqueue(args);
}

void GeneratorWithQuery::explicitStop()
{
    finished = true;
    stop();
}

void GeneratorWithQuery::start()
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
                    generateKmersWithQueryKmers(
                        this->kmer_size,
                        args->encoded_read,
                        args->bases_per_block,
                        args->read_length,
                        args->block_count,
                        this->kmer_queue);
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
#include <thread>
#include <string>
#include <iostream>

#include "writer.hpp"
#include "kmer_dump.hpp"

Writer::Writer(std::string output_base_path,
               boost::lockfree::queue<struct WriterArguments *> *writer_queue,
               int partition_count,
               int rank)
{
    this->output_base_path = output_base_path;
    this->writer_queue = writer_queue;
    this->partition_count = partition_count;
    this->rank = rank;
    this->total_distinct_count = 0;

    start();
}

Writer::~Writer()
{
    if (!finished)
        stop();
}

void Writer::explicitStop()
{
    finished = true;
    stop();
}

void Writer::start()
{
    runner = std::thread(
        [=]
        {
            struct WriterArguments *args;
            while (true)
            {
                bool pop_success = writer_queue->pop(args);

                if (pop_success)
                {
                    this->total_distinct_count += args->counts->size();
                    saveHashMap(args->counts, 0, this->output_base_path + std::to_string(args->partition));

                    args->counts->clear(); // this this should be here to clear the hashmap
                    delete args->counts;
                    free(args);
                }

                if (finished && writer_queue->empty())
                {
                    break;
                }
            }
        });
}

void Writer::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }

    std::cout << "(" << rank << ") Dump Size =  " << total_distinct_count * (sizeof(hashmap_key_type) + sizeof(hashmap_value_type)) / (1024 * 1024 * 1024) << " GB" << std::endl;

}
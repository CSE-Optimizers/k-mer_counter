#include <thread>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"
#include "extractor.hpp"

#ifndef GENERATOR_H
#define GENERATOR_H
/*
Generates kmers from encoded data file and adds to bins
*/
class Generator
{
public:
    explicit Generator(uint64_t kmer_size, uint32_t bases_per_block,
                       int read_queue_size, int partition_count, size_t bin_size,
                       boost::lockfree::queue<struct KmerBin *> *writer_queue);

    ~Generator();

    void enqueue(struct GeneratorArguments *args);

    void explicitStop();

private:
    size_t bin_size;
    uint64_t kmer_size;
    int partition_count;
    int read_queue_size;
    std::thread runner;
    ThreadSafeQueue<struct GeneratorArguments> q;
    bool finished = false;
    uint32_t bases_per_block;
    struct KmerBin **bins;
    boost::lockfree::queue<struct KmerBin *> *writer_queue;

    void start();

    void stop() noexcept;
};

#endif
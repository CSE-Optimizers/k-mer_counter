#include <thread>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"
#include <string>

#ifndef KMER_WRITER_H
#define KMER_WRITER_H

class KmerWriter
{
public:
    explicit KmerWriter(
        boost::lockfree::queue<struct KmerBin *> *writer_queue,
        int partition_count,
        int rank,
        std::string output_base_path);

    ~KmerWriter();

    void explicitStop();

private:
    std::thread runner;
    bool finished = false;
    boost::lockfree::queue<struct KmerBin *> *writer_queue;
    int partition_count;
    size_t *file_sizes;
    int rank;
    std::string output_base_path;

    void start();

    void stop() noexcept;
};

#endif
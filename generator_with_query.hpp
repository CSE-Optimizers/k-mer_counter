#include <thread>
#include <string>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"
#include "extractor.hpp"

#ifndef GENERATORWITHQEURY_H
#define GENERATORWITHQEURY_H
/*
Generates kmers from encoded data file and adds to bins
*/
class GeneratorWithQuery
{
public:
    explicit GeneratorWithQuery(int kmer_size,
                                int read_queue_size,
                                boost::lockfree::queue<uint64_t> *kmer_queue);

    ~GeneratorWithQuery();

    void enqueue(struct GeneratorArguments *args);

    void explicitStop();

private:
    int kmer_size;
    int read_queue_size;
    std::thread runner;
    boost::lockfree::queue<uint64_t> *kmer_queue;
    ThreadSafeQueue<struct GeneratorArguments> q;
    bool finished = false;

    void start();

    void stop() noexcept;
};

#endif
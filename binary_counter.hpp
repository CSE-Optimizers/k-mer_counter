#ifndef BINARY_COUNTER_H
#define BINARY_COUNTER_H

#include <queue>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>
#include <boost/lockfree/queue.hpp>
#include "thread_safe_queue.hpp"

struct Node
{
    uint64_t key;
    uint16_t value;
    struct Node *left, *right;
};

class BinaryCounter
{

public:
    explicit BinaryCounter(
        boost::lockfree::queue<uint64_t> *kmer_queue,
        std::vector<uint64_t> query_list);

    void count(struct Node* root, uint64_t kmer);

    void explicitStop();

private:
    boost::lockfree::queue<uint64_t> *kmer_queue;
    struct Node *root;
    std::thread runner;
    bool finished = false;
    std::vector<uint64_t> query_list;


    void sort();

    struct Node* createNewNode(uint64_t key);

    struct Node* build(std::vector<uint64_t> list, std::size_t start, std::size_t end);

    void start();

    void stop();
};

#endif
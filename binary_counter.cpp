
#include <bits/stdc++.h>
#include "binary_counter.hpp"

BinaryCounter::BinaryCounter(
    boost::lockfree::queue<uint64_t> *kmer_queue,
    std::vector<uint64_t> query_list)
{
    this->kmer_queue = kmer_queue;
    this->query_list = query_list;

    start();
}

BinaryCounter::~BinaryCounter()
{
    if (!finished)
    {
        stop();
    }
}

void BinaryCounter::sort()
{
    std::sort(this->query_list.begin(), this->query_list.end());
}

struct Node *BinaryCounter::createNewNode(uint64_t key)
{
    struct Node *newNode = (struct Node *)malloc(sizeof(struct Node));
    newNode->key = key;
    newNode->left = NULL;
    newNode->right = NULL;
    return newNode;
}

struct Node *BinaryCounter::build(std::vector<uint64_t> list, std::size_t start, std::size_t end)
{
    if (start > end)
    {
        return NULL;
    }

    int mid = (start + end) / 2;
    struct Node *root = createNewNode(list.at(mid));

    root->left = build(list, start, mid - 1);

    root->right = build(list, mid + 1, end);
}

void BinaryCounter::count(struct Node *root, uint64_t kmer)
{
    if (root->key == kmer)
    {
        root->value++;
    }
    else if (root->key < kmer)
    {
        count(root->right, kmer);
    }
    else if (root->key > kmer)
    {
        count(root->left, kmer);
    }
}

void BinaryCounter::start()
{
    runner = std::thread(
        [=]
        {
            sort();
            build(this->query_list, 0, this->query_list.size());
            uint64_t kmer;
            while (true)
            {
                bool success = kmer_queue->pop(kmer);
                if (success)
                {
                    count(this->root, kmer);
                }

                if (finished && kmer_queue->empty())
                {
                    break;
                }
            }
        });
}

void BinaryCounter::explicitStop()
{
    finished = true;
    stop();
}

void BinaryCounter::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }
}
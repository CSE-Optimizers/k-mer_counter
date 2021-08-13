#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <string>

#include "extractor.hpp"

struct counterArguments
{
    char *buffer;
    uint64_t allowed_length;
    enum LineType first_line_type;
};

class ThreadSafeQueue
{

public:
    ThreadSafeQueue();

    void enqueue(struct counterArguments *item);

    struct counterArguments* dequeue();

    bool isEmpty();

private:
    std::queue<struct counterArguments*> q;
    std::mutex mu;
};

#endif
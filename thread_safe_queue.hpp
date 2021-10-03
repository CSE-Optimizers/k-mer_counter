#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>

#include "extractor.hpp"

struct counterArguments
{
    char *buffer;
    uint64_t allowed_length;
    enum LineType first_line_type;
    bool reset_status;
};

class ThreadSafeQueue
{

public:
    ThreadSafeQueue();

    void setLimit(int value);

    void enqueue(struct counterArguments *item);

    struct counterArguments* dequeue();

    bool isEmpty();

private:
    std::queue<struct counterArguments*> q;
    std::condition_variable condition;
    std::mutex mu;
    int count = 0;
    int limit =10;
};

#endif
#include <queue>
#include <mutex>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"

ThreadSafeQueue::ThreadSafeQueue() {}

void ThreadSafeQueue::setLimit(int value)
{
    limit = value;
}

void ThreadSafeQueue::enqueue(struct counterArguments *item)
{
    // std::cout << "Hello" << std::endl;
    std::unique_lock<std::mutex> lock(mu);
    if (count >= limit)
    {
        condition.wait(lock);
    }
    q.push(item);
    count++;
    lock.unlock();
}

struct counterArguments *ThreadSafeQueue::dequeue()
{
    // std::cout << "World" << std::endl;
    std::unique_lock<std::mutex> lock(mu);
    struct counterArguments *out = NULL;
    if (!q.empty())
    {
        out = q.front();
        q.pop();
        count--;
        condition.notify_one();
    }
    lock.unlock();
    return out;
}

bool ThreadSafeQueue::isEmpty()
{
    std::unique_lock<std::mutex> lock(mu);
    bool val = q.empty();
    lock.unlock();
    return val;
}

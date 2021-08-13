#include <queue>
#include <mutex>
#include <string>

#include "thread_safe_queue.hpp"

ThreadSafeQueue::ThreadSafeQueue() {}

void ThreadSafeQueue::enqueue(struct counterArguments *item) {
    mu.lock();
    q.push(item);
    mu.unlock();
}

struct counterArguments* ThreadSafeQueue::dequeue() {
    mu.lock();
    struct counterArguments *out = q.front();
    q.pop();
    mu.unlock();
    return out;
    
}

bool ThreadSafeQueue::isEmpty() {
    mu.lock();
    bool val = q.empty();
    mu.unlock();
    return val;
}

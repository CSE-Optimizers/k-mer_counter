#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

#include <queue>
#include <mutex>
#include <string>
#include <condition_variable>

#include "extractor.hpp"


template <class T>
class ThreadSafeQueue
{

public:
    ThreadSafeQueue();

    void setLimit(int value);

    void enqueue(T *item);

    T* dequeue();

    bool isEmpty();

private:
    std::queue<T*> q;
    std::condition_variable condition;
    std::mutex mu;
    int count = 0;
    int limit =10;
};

#endif
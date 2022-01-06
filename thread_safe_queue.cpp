#include <queue>
#include <mutex>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"

template <class T>
ThreadSafeQueue<T>::ThreadSafeQueue() {}


template <class T>
void ThreadSafeQueue<T>::setLimit(int value)
{
    limit = value;
}


template <class T>
void ThreadSafeQueue<T>::enqueue(T *item)
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


template <class T>
T *ThreadSafeQueue<T>::dequeue()
{
    // std::cout << "World" << std::endl;
    std::unique_lock<std::mutex> lock(mu);
    T *out = NULL;
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


template <class T>
bool ThreadSafeQueue<T>::isEmpty()
{
    std::unique_lock<std::mutex> lock(mu);
    bool val = q.empty();
    lock.unlock();
    return val;
}


// this is to fix linker errors
// if you're using ThreadSafeQueue class, add the relevant template instance as a new line
// example: template class ThreadSafeQueue<YourClassHere>;

template class ThreadSafeQueue<struct CounterArguments>;
// template class ThreadSafeQueue<struct FileChunkData>;
template class ThreadSafeQueue<char>;


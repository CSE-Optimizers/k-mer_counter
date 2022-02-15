#include <thread>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"
#include <string>

#ifndef ENCODER_WRITER_H
#define ENCODER_WRITER_H

class EncoderWriter
{
public:
    explicit EncoderWriter(boost::lockfree::queue<struct EncoderWriterArgs *> *writer_queue,
                           int rank, std::string base_path);

    ~EncoderWriter();

    void explicitStop();

private:
    std::thread runner;
    bool finished = false;
    boost::lockfree::queue<struct EncoderWriterArgs *> *writer_queue;
    int rank;
    std::string base_path;
    size_t total_encoded_size;

    void start();

    void stop() noexcept;
};

#endif
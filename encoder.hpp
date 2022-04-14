#include <thread>

#include "thread_safe_queue.hpp"
#include <boost/lockfree/queue.hpp>
#include "utils.hpp"

#ifndef ENCODER_H
#define ENCODER_H

class Encoder
{
public:
    explicit Encoder(uint64_t buffer_size,
                     int read_queue_size,
                     boost::lockfree::queue<struct EncoderWriterArgs *> *writer_queue);

    ~Encoder();

    void enqueue(struct EncoderArguments *args);

    void explicitStop();

private:
    std::thread runner;
    ThreadSafeQueue<struct EncoderArguments> q;
    bool finished = false;
    uint64_t buffer_size;
    boost::lockfree::queue<struct EncoderWriterArgs *> *writer_queue;

    void encodeChunk(char *original_chunk);

    uint32_t *encodeRead(char *original_read, uint32_t read_length, size_t *encoded_read_size_out);

    void start();

    void stop() noexcept;
};

#endif
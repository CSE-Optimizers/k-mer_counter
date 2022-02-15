#include <thread>
#include <string>
#include <iostream>

#include "thread_safe_queue.hpp"
#include "encoder.hpp"

static inline __attribute__((always_inline)) uint32_t getCharEncoding(const char character)
{
    switch (character)
    {
    case 'A':
        return (uint32_t)0;
        break;
    case 'C':
        return (uint32_t)1;
        break;
    case 'G':
        return (uint32_t)2;
        break;
    case 'T':
        return (uint32_t)3;
        break;
    default:
        return (uint32_t)4;
        break;
    }
}

Encoder::Encoder(uint64_t buffer_size,
                 int read_queue_size,
                 boost::lockfree::queue<struct EncoderWriterArgs *> *writer_queue)
{
    q.setLimit(read_queue_size);
    this->buffer_size = buffer_size;
    this->writer_queue = writer_queue;

    start();
}

Encoder::~Encoder()
{
    if (!finished)
        stop();
}

void Encoder::enqueue(struct EncoderArguments *args)
{
    q.enqueue(args);
}

void Encoder::explicitStop()
{
    finished = true;
    stop();
}

uint32_t *Encoder::encodeRead(char *original_read, uint32_t read_length, size_t *encoded_read_size_out)
{
    const size_t bases_per_block = 10;
    const uint32_t HEADER_BLOCK_IDENTIFIER = ((uint32_t)3) << 30;
    // block size = 32bits
    // reserve 1st 2bits for identifier
    const size_t total_blocks = ((read_length + bases_per_block - 1) / bases_per_block) + 1; // equals to ceil(read_length/bases_per_block) + 1
    (*encoded_read_size_out) = total_blocks;
    uint32_t *encoded_read = (uint32_t *)calloc(total_blocks, sizeof(uint32_t));
    assert(encoded_read);
    encoded_read[0] = HEADER_BLOCK_IDENTIFIER + (uint32_t)read_length;
    for (size_t block_i = 1; block_i < total_blocks - 1; block_i++)
    {
        for (size_t base_in_block_i = 0; base_in_block_i < bases_per_block; base_in_block_i++)
        {
            encoded_read[block_i] = encoded_read[block_i] |
                                    (getCharEncoding(
                                         original_read[bases_per_block * (block_i - 1) + base_in_block_i])
                                     << ((bases_per_block - base_in_block_i - 1) * 3));
        }
    }

    // last block
    const size_t bases_in_last_block = read_length - (bases_per_block * (total_blocks - 2));
    for (size_t base_in_block_i = 0; base_in_block_i < bases_in_last_block; base_in_block_i++)
    {
        encoded_read[total_blocks - 1] = encoded_read[total_blocks - 1] |
                                         (getCharEncoding(
                                              original_read[bases_per_block * (total_blocks - 2) + base_in_block_i])
                                          << ((bases_per_block - base_in_block_i - 1) * 3));
    }

    return encoded_read;
}
void Encoder::encodeChunk(char *original_chunk)
{
    uint64_t character_i = 0;
    uint32_t current_read_length;
    size_t current_encoded_read_size;
    uint32_t *encoded_chunk = NULL;
    size_t total_encoded_size = 0;

    while (character_i < this->buffer_size)
    {
        current_read_length = 0;

        // loop to calculate current read length
        for (uint64_t i = character_i; original_chunk[i] != '\n' && original_chunk[i] != '\0'; i++)
        {
            current_read_length++;
        }

        if (original_chunk[character_i] == '\0')
        {
            // reached the end of buffer
            break;
        }

        uint32_t *current_encoded_read = this->encodeRead(
            &original_chunk[character_i], current_read_length, &current_encoded_read_size);

        if (encoded_chunk == NULL)
        {
            encoded_chunk = current_encoded_read;
            total_encoded_size = current_encoded_read_size;
        }
        else
        {
            encoded_chunk = (uint32_t *)realloc(
                encoded_chunk, (total_encoded_size + current_encoded_read_size) * sizeof(uint32_t));
            assert(encoded_chunk);
            memcpy(&encoded_chunk[total_encoded_size], current_encoded_read, current_encoded_read_size * sizeof(uint32_t));
            free(current_encoded_read);
            total_encoded_size += current_encoded_read_size;
        }

        character_i += (current_read_length + 1); // +1 because of ignoring terminating \n or \0 character previously
    }

    if (total_encoded_size > 0)
    {

        struct EncoderWriterArgs *writer_args = (struct EncoderWriterArgs *)malloc(sizeof(struct EncoderWriterArgs));
        writer_args->encoded_size = total_encoded_size;
        writer_args->encoded_chunk = encoded_chunk;
        writer_queue->push(writer_args);
    }

    return;
}

void Encoder::start()
{
    runner = std::thread(
        [=]
        {
            struct EncoderArguments *args;
            while (true)
            {
                args = q.dequeue();

                if (args != NULL)
                {
                    this->encodeChunk(args->buffer);

                    free(args->buffer);
                    free(args);
                }

                if (finished && q.isEmpty())
                {
                    break;
                }
            }
        });
}

void Encoder::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }
}
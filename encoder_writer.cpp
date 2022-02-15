#include <thread>
#include <string>
#include <iostream>

#include "encoder_writer.hpp"

EncoderWriter::EncoderWriter(boost::lockfree::queue<struct EncoderWriterArgs *> *writer_queue,
                             int rank, std::string base_path)
{
    this->writer_queue = writer_queue;
    this->rank = rank;
    this->base_path = base_path;
    this->total_encoded_size = 0;

    start();
}

EncoderWriter::~EncoderWriter()
{
    if (!finished)
        stop();
}

void EncoderWriter::explicitStop()
{
    finished = true;
    stop();
}

void EncoderWriter::start()
{
    runner = std::thread(
        [=]
        {
            struct EncoderWriterArgs *args;
            FILE *output_encoded_file = fopen((base_path + "encoded.bin").c_str(), "wb");
            while (true)
            {
                bool pop_success = writer_queue->pop(args);

                if (pop_success)
                {
                    fwrite(args->encoded_chunk, sizeof(uint32_t), args->encoded_size, output_encoded_file);
                    this->total_encoded_size += args->encoded_size;
                    free(args->encoded_chunk);
                    free(args);

                }

                if (finished && writer_queue->empty())
                {
                    break;
                }
            }
            fclose(output_encoded_file);
        });
}

void EncoderWriter::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }


    std::cout << "(" << rank << ") " << "encoded file size = "
              << (this->total_encoded_size * sizeof(uint32_t))/(1024*1024) <<" MB" << std::endl;

}
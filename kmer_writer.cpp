#include <thread>
#include <string>
#include <iostream>

#include "kmer_writer.hpp"

KmerWriter::KmerWriter(
    boost::lockfree::queue<struct KmerBin *> *writer_queue,
    int partition_count,
    int rank,
    std::string output_base_path)
{
    this->writer_queue = writer_queue;
    this->partition_count = partition_count;
    this->rank = rank;
    this->output_base_path = output_base_path;
    this->file_sizes = (size_t *)malloc(partition_count * sizeof(size_t));
    assert(this->file_sizes);

    for (int i = 0; i < partition_count; i++)
    {
        std::system(("mkdir -p " + output_base_path + std::to_string(i)).c_str());
        FILE *fp = fopen((output_base_path + std::to_string(i) + "/0.kmer").c_str(), "wb");
        fclose(fp);
        this->file_sizes[i] = 0;
    }
    start();
}

KmerWriter::~KmerWriter()
{
    if (!finished)
        stop();
}

void KmerWriter::explicitStop()
{
    finished = true;
    stop();
}

void KmerWriter::start()
{
    runner = std::thread(
        [=]
        {
            struct KmerBin *bin;
            while (true)
            {
                bool pop_success = writer_queue->pop(bin);

                if (pop_success)
                {
                    this->file_sizes[bin->id] += bin->filled_size;
                    FILE *fp = fopen((output_base_path + std::to_string(bin->id) + "/0.kmer").c_str(), "ab");
                    fwrite(bin->bin, sizeof(uint64_t), bin->filled_size, fp);
                    fclose(fp);
                    free(bin->bin);

                    free(bin);
                }

                if (finished && writer_queue->empty())
                {
                    break;
                }
            }
        });
}

void KmerWriter::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }

    std::string st = "";
    for (int i = 0; i < this->partition_count; i++)
    {
        st += " " + std::to_string((this->file_sizes[i] * 8) / (1024 * 1024));
    }

    std::cout << "Bin sizes in MB -> " << rank << "\n"
              << st << std::endl;
    free(this->file_sizes);
}
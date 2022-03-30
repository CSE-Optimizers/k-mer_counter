#include <iostream>
#include <mpi.h>
#include <sys/stat.h>
#include <boost/lockfree/queue.hpp>

#include "generator.hpp"
#include "kmer_writer.hpp"
#include "counter.hpp"
#include "writer.hpp"

#define HASHMAP_MAX_SIZE 0x400000
// #define KMER_BIN_MAX_SIZE 0x2000000 // for 10 bins, 2.5 GB memory
#define KMER_BIN_MAX_SIZE 0x8000 // 256KB per one bin
#define BIN_QUEUE_SIZE 100
#define READ_QUEUE_SIZE 1000
#define PARTITION_COUNT 100
// #define PARTITION_FACTOR 26     //26 will give approx 100 partitions for 273GB file for 64 nodes
#define BASES_PER_BLOCK 10

int main(int argc, char *argv[])
{
    int num_tasks, rank;

    const char *encoded_file_name = argv[1];
    int kmer_size = atoi(argv[2]);
    std::string output_base_file_path = argv[3];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &namelen);

    std::system(("rm -rf " + output_base_file_path + "*/*.kmer").c_str());
    std::system(("rm -rf " + output_base_file_path + "*/*.data").c_str());

    if (rank > 0)
    {
        boost::lockfree::queue<struct KmerBin *> kmer_writer_queue(BIN_QUEUE_SIZE);
        Generator generator(kmer_size, BASES_PER_BLOCK,
                            READ_QUEUE_SIZE, PARTITION_COUNT, KMER_BIN_MAX_SIZE, &kmer_writer_queue);
        KmerWriter kmer_writer(&kmer_writer_queue, PARTITION_COUNT, rank, output_base_file_path);

        // Calculating total file size
        struct stat encoded_file_stats;
        size_t encoded_file_size;
        if (stat(encoded_file_name, &encoded_file_stats) != 0)
        {
            std::cerr << "Error when calculating file size." << std::endl;
            exit(EXIT_FAILURE);
        }

        encoded_file_size = encoded_file_stats.st_size;
        std::cout << "(" << rank << ") Encoded file size  = " << encoded_file_size / (1024 * 1024) << " MB" << std::endl;

        FILE *encoded_file = fopen(encoded_file_name, "rb"); //"encoded.bin"
        uint32_t current_header_block;
        uint32_t current_read_block_count;
        uint32_t current_read_length;
        uint32_t *encoded_read;
        const uint32_t bases_per_block = (uint32_t)10;
        size_t log_counter = 0;

        while (fread(&current_header_block, sizeof(uint32_t), 1, encoded_file))
        {
            assert((current_header_block >> 30) == ((uint32_t)3)); // verify it is a header block
            current_read_length = current_header_block & ~(3 << 30);

            current_read_block_count = ((current_read_length + bases_per_block - 1) / bases_per_block);
            encoded_read = (uint32_t *)malloc(current_read_block_count * sizeof(uint32_t));
            assert(encoded_read);
            memset(encoded_read, 0, current_read_block_count * sizeof(uint32_t));

            fread(encoded_read, sizeof(uint32_t), current_read_block_count, encoded_file);

            struct GeneratorArguments *args = (struct GeneratorArguments *)malloc(sizeof(struct GeneratorArguments));

            args->encoded_read = encoded_read;
            args->bases_per_block = bases_per_block;
            args->read_length = current_read_length;
            args->block_count = current_read_block_count;

            generator.enqueue(args);
            log_counter++;

            if (log_counter % 0xFFFFF == 0)
            {
                std::cout << "(" << rank << ") File read progress " << 100 * (ftell(encoded_file) / ((double)encoded_file_size)) << "%\n";
            }
        }

        fclose(encoded_file);
        generator.explicitStop();
        kmer_writer.explicitStop();
        printf("\n");
        std::cout << "(" << rank << ") Kmer writing stage finished" << std::endl;

        // counting stage
        boost::lockfree::queue<struct WriterArguments *> hashmap_writer_queue(PARTITION_COUNT);

        Counter counter(kmer_size, BIN_QUEUE_SIZE, &hashmap_writer_queue, PARTITION_COUNT);
        Writer hashmap_writer(output_base_file_path, &hashmap_writer_queue, PARTITION_COUNT, rank);

        for (int partition_i = 0; partition_i < PARTITION_COUNT; partition_i++)
        {
            std::string kmer_file_path = output_base_file_path + std::to_string(partition_i) + "/0.kmer";
            FILE *kmer_file = fopen(kmer_file_path.c_str(), "rb"); // .kmer file

            while (!feof(kmer_file))
            {
                struct KmerBin *new_bin = (struct KmerBin *)malloc(sizeof(struct KmerBin));
                new_bin->bin = (uint64_t *)malloc(KMER_BIN_MAX_SIZE * sizeof(uint64_t));
                new_bin->id = partition_i;
                size_t bin_size = fread(new_bin->bin, sizeof(uint64_t), KMER_BIN_MAX_SIZE, kmer_file);
                new_bin->filled_size = bin_size;
                counter.enqueue(new_bin);             
            }
            std::cout << "(" << rank << ") finished partition " << partition_i << std::endl;
            fclose(kmer_file);
            remove(kmer_file_path.c_str());
        }

        counter.explicitStop();
        hashmap_writer.explicitStop();
    }
    std::cout << "finalizing.." << rank << std::endl;
    MPI_Finalize();
    std::cout << "done.." << rank << std::endl;

    return 0;
}

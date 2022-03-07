#include <iostream>
#include <fstream>
#include <mpi.h>
#include <string>
#include <sys/stat.h>
#include <boost/lockfree/queue.hpp>

#include "binary_counter.hpp"
#include "generator_with_query.hpp"
#include "thread_safe_queue.hpp"

#define HASHMAP_MAX_SIZE 0x400000
// #define KMER_BIN_MAX_SIZE 0x2000000 // for 10 bins, 2.5 GB memory
#define KMER_BIN_MAX_SIZE 0x8000 // 256KB per one bin
#define BIN_QUEUE_SIZE 1000
#define READ_QUEUE_SIZE 10000
#define PARTITION_COUNT 100
#define BASES_PER_BLOCK 10
#define KMER_QUEUE_SIZE 100000

int main(int argc, char *argv[])
{
    int num_tasks, rank;

    const char *encoded_file_name = argv[1];
    int kmer_size = atoi(argv[2]);
    std::string output_base_file_path = argv[3];
    std::string query_kmer_file_path = argv[4];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &namelen);

    std::system(("rm -rf " + output_base_file_path + "*/*.kmer").c_str());
    std::system(("rm -rf " + output_base_file_path + "*/*.data").c_str());

    // vector<char *> queries;
    // char *query_kmer = (char *)malloc(kmer_size*sizeof(char));

    // if (rank == 0)
    // {
    //     FILE *fp = fopen(query_kmer_file_path, "r");
    //     while (!feof(fp))
    //     {
    //         fgets(query_kmer, kmer_size, fp);
    //         queries.emplace_back(query_kmer);
    //     }
    //     fclose(fp);
    // }

    // for (int i = 0; i < queries.size(); i++)
    // {
    //     MPI_Bcast(queries.at(i), kmer_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    //     cout << rank << " " << queries.at(i) << endl;
    // }

    std::vector<std::string> queries;
    std::string query_kmer;

    if (rank = 0)
    {
        std::ifstream fp(query_kmer_file_path);
        while (getline(fp, query_kmer))
        {
            queries.emplace_back(query_kmer);
        }
        fp.close();
    }

    for (int i = 0; i < queries.size(); i++)
    {
        MPI_Bcast(&(queries.at(i)), kmer_size, MPI_CHAR, 0, MPI_COMM_WORLD);
    }

    if (rank > 0)
    {
        // boost::lockfree::queue<struct KmerBin *> kmer_writer_queue(BIN_QUEUE_SIZE);
        // Generator generator(kmer_size, BASES_PER_BLOCK,
        //                     READ_QUEUE_SIZE, PARTITION_COUNT, KMER_BIN_MAX_SIZE, &kmer_writer_queue);
        // KmerWriter kmer_writer(&kmer_writer_queue, PARTITION_COUNT, rank, output_base_file_path);
        // BinaryCounter binaryCounter();
        boost::lockfree::queue<uint64_t> kmer_queue(KMER_QUEUE_SIZE);

        GeneratorWithQuery generator(kmer_size, READ_QUEUE_SIZE, &kmer_queue);
        BinaryCounter binaryCounter(&kmer_queue, queries);

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
        const uint32_t bases_per_block = (size_t)10;
        size_t log_counter = 0;

        while (fread(&current_header_block, sizeof(uint32_t), 1, encoded_file))
        {
            assert((current_header_block >> 30) == ((uint32_t)3)); // verify it is a header block
            current_read_length = current_header_block & ~(3 << 30);

            current_read_block_count = ((current_read_length + bases_per_block - 1) / bases_per_block);
            encoded_read = (uint32_t *)calloc(current_read_block_count, sizeof(uint32_t));

            fread(encoded_read, sizeof(uint32_t), current_read_block_count, encoded_file);

            struct GeneratorArguments *args = (struct GeneratorArguments *)malloc(sizeof(struct GeneratorArguments));

            args->encoded_read = encoded_read;
            args->bases_per_block = bases_per_block;
            args->read_length = current_read_length;
            args->block_count = current_read_block_count;

            generator.enqueue(args);
            log_counter++;

            if (log_counter % 0xF000 == 0)
            {
                std::cout << "(" << rank << ") File read progress " << 100 * (ftell(encoded_file) / ((double)encoded_file_size)) << "%\n";
            }
        }

        fclose(encoded_file);
        generator.explicitStop();
        binaryCounter.explicitStop();
        printf("\n");
        std::cout << "(" << rank << ") Kmer writing stage finished" << std::endl;
    }
    std::cout << "finalizing.." << rank << std::endl;
    MPI_Finalize();
    std::cout << "done.." << rank << std::endl;

    return 0;
}

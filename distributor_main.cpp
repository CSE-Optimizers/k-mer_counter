#include <string>
#include <iostream>
#include <sys/stat.h>
#include <mpi.h>
#include <boost/lockfree/queue.hpp>

#include "utils.hpp"
#include "encoder.hpp"
#include "thread_safe_queue.hpp"
#include "encoder_writer.hpp"
#include "file_reader.hpp"

#define COUNTER_NODE_READ_QUEUE_SIZE 10
#define MASTER_NODE_READ_QUEUE_SIZE 400
#define COM_BUFFER_SIZE 0x40000 // 0x40000 = 256KB
#define MAX_LINE_LENGTH 103000  // TODO: make this a parameter

int main(int argc, char *argv[])
{
    int num_tasks, rank;

    const char *file_name = argv[1];
    std::string output_base_path = argv[2];

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int namelen;
    char processor_name[MPI_MAX_PROCESSOR_NAME];
    MPI_Get_processor_name(processor_name, &namelen);

    // removing previous encoded file
    std::system(("rm " + output_base_path + "encoded.bin").c_str()); // removing previous encoded file

    uint64_t com_buffer_size = COM_BUFFER_SIZE;

    // send buffer size
    // MPI_Bcast(&com_buffer_size, 1, MPI_UINT64_T, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        // Calculating total file size

        size_t total_file_size;
        struct stat data_file_stats;
        if (stat(file_name, &data_file_stats) != 0)
        {
            std::cerr << "Error when calculating file size." << std::endl;
            exit(EXIT_FAILURE);
        }

        total_file_size = data_file_stats.st_size;
        std::cout << "Total file size  = " << total_file_size / 1024 << " KB" << std::endl;

        ThreadSafeQueue<char> master_file_queue;
        master_file_queue.setLimit(MASTER_NODE_READ_QUEUE_SIZE);

        FileReader file_reader(file_name, COM_BUFFER_SIZE, MAX_LINE_LENGTH, &master_file_queue, total_file_size);

        int node_rank;

        char *send_buffer;

        uint64_t log_counter = 0;
        while (true)
        {

            send_buffer = master_file_queue.dequeue();
            if (send_buffer != NULL)
            {
                MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(send_buffer, COM_BUFFER_SIZE, MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);

                free(send_buffer);
                log_counter++;
            }

            if (file_reader.isCompleted() && master_file_queue.isEmpty())
            {
                break;
            }
        }
        file_reader.finish();

        // send empty buffer as the last message
        send_buffer = (char *)malloc(COM_BUFFER_SIZE);
        assert(send_buffer);
        memset(send_buffer, 0, COM_BUFFER_SIZE);

        for (int j = 1; j < num_tasks; j++)
        {
            MPI_Recv(&node_rank, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(send_buffer, COM_BUFFER_SIZE, MPI_BYTE, node_rank, 1, MPI_COMM_WORLD);

            std::cout << "Finish Sending Allocations to " << node_rank << std::endl;
        }
        free(send_buffer);
    }

    if (rank > 0)
    {
        std::cout << "(" << rank << ") com buffer size = " << com_buffer_size / 1024 << " KB" << std::endl;

        boost::lockfree::queue<struct EncoderWriterArgs *> writer_queue(10);

        Encoder encoder(com_buffer_size, COUNTER_NODE_READ_QUEUE_SIZE, &writer_queue);
        EncoderWriter writer(&writer_queue, rank, output_base_path);

        char *recv_buffer;

        while (true)
        {
            recv_buffer = (char *)malloc(com_buffer_size);
            assert(recv_buffer);
            MPI_Send(&rank, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
            MPI_Recv(recv_buffer, com_buffer_size, MPI_BYTE, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (recv_buffer[0] == 0)
            {
                // last empty message received
                free(recv_buffer);
                break;
            }

            struct EncoderArguments *args = (struct EncoderArguments *)malloc(sizeof(struct EncoderArguments));
            args->buffer_size = com_buffer_size;
            args->buffer = recv_buffer;


            encoder.enqueue(args);
        }
        
        encoder.explicitStop();
        writer.explicitStop();
        printf("\n");
    }

    std::cout << "finalizing.." << rank << std::endl;
    MPI_Finalize();
    std::cout << "done.." << rank << std::endl;

    return 0;
}

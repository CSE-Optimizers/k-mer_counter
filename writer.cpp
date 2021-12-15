#include <thread>
#include <string>
#include <iostream>

#include "writer.hpp"
#include "kmer_dump.hpp"

Writer::Writer(std::string file_path,
                     boost::lockfree::queue<struct writerArguments*> *writer_queue,
                     int partition_count,
                     int rank,
                     std::string base_path)
{
    this->file_path = file_path;
    this->writer_queue = writer_queue;
    this->partition_count = partition_count;
    this->rank = rank;
    this->base_path = base_path;

    this->file_counts = (int*)malloc(partition_count*sizeof(int));
    for(int i=0; i<partition_count; i++){
        file_counts[i]  = 0;
    }
    start();
}

Writer::~Writer()
{
    if(!finished)
        stop();
}

void Writer::explicitStop()
{
    finished = true;
    stop();
}

void Writer::start()
{
    runner = std::thread(
        [=]
        {
            struct writerArguments *args;
            while (true)
            {
                bool pop_success = writer_queue->pop(args);

                if(pop_success){
                    // std::cout<<"Successful pop"<<std::endl;
                    // string save_directory_base_path = "data/"+ std::to_string(args->partition);
                    // saveHashMap(args->counts, file_counts[args->partition], "/home/damika/Documents/test_results/data/"+ std::to_string(args->partition));
                    saveHashMap(args->counts, file_counts[args->partition], base_path + std::to_string(args->partition));
                   
                    file_counts[args->partition]++;
                    args->counts->clear();      // this this should be here to clear the hashmap
                    // free(args->counts);
                    free(args);

                    // std::cout<<args->partition<<std::endl;
                }

                if (finished && writer_queue->empty())
                {
                    break;
                }
            }
        });
}

void Writer::stop() noexcept
{
    if (runner.joinable())
    {
        runner.join();
    }
    
    string st="";
    for(int i=0;i<partition_count;i++){
        st+=" "+std::to_string(file_counts[i]);
        string stats_file_path = base_path + std::to_string(i) +"/stats";
        FILE *stats_file_fp = fopen(stats_file_path.c_str(), "w");
        fwrite(&file_counts[i], sizeof(int), 1, stats_file_fp);
        fclose(stats_file_fp);
    }
    
    std::cout<<"Paritition summary-> "<<rank<<"\n"<<st<<std::endl;

    // std::cout<<"stoping writer"<<rank<<std::endl;
}
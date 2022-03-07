#ifndef EXTRACTOR_H
#define EXTRACTOR_H


#include <boost/lockfree/queue.hpp>

#include "utils.hpp"
#include "thread_safe_queue.hpp"
// enum LineType getLineType(FILE *fp);

// void countKmersFromBuffer(
//     const uint64_t kmer_size,
//     char *buffer,
//     const uint64_t buffer_size,
//     const uint64_t allowed_length,
//     const enum LineType first_line_type,
//     const bool is_starting_from_line_middle,
//     custom_dense_hash_map *counts);

// void countKmersFromBufferWithPartitioning(
//     const uint64_t kmer_size,
//     char *buffer,
//     const uint64_t buffer_size,
//     const uint64_t allowed_length,
//     const enum LineType first_line_type,
//     const bool reset_status,
//     custom_dense_hash_map **counts,
//     int partition_count,
//     boost::lockfree::queue<struct WriterArguments *> *writer_queue);

void generateKmersWithPartitioning(
    const uint64_t kmer_size,
    uint32_t *encoded_read,
    uint32_t bases_per_block,
    uint32_t read_length,
    uint32_t block_count,
    int partition_count,
    struct KmerBin **bins,
    size_t bin_size,
    boost::lockfree::queue<struct KmerBin *> *writer_queue);

void generateKmersWithQueryKmers(
    const uint64_t kmer_size,
    uint32_t *encoded_read,
    uint32_t bases_per_block,
    uint32_t read_length,
    uint32_t block_count,
    boost::lockfree::queue<uint64_t> *kmer_queue);

#endif
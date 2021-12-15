#ifndef EXTRACTOR_H
#define EXTRACTOR_H

#include "utils.hpp"
#include <boost/lockfree/queue.hpp>

enum LineType getLineType(FILE *fp);

void countKmersFromBuffer(
    const uint64_t kmer_size,
    char *buffer,
    const uint64_t buffer_size,
    const uint64_t allowed_length,
    const enum LineType first_line_type,
    const bool is_starting_from_line_middle,
    custom_dense_hash_map *counts);

void countKmersFromBufferWithPartitioning(
    const uint64_t kmer_size,
    char *buffer,
    const uint64_t buffer_size,
    const uint64_t allowed_length,
    const enum LineType first_line_type,
    const bool reset_status,
    custom_dense_hash_map **counts,
    int partition_count,
    boost::lockfree::queue<struct writerArguments *> *writer_queue);

#endif
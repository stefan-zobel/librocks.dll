#pragma once

#include "rocksdb/table.h"

class BlockBasedTableConfig
{
public:
    static inline const rocksdb::BlockBasedTableOptions options = [] {
        rocksdb::BlockBasedTableOptions opts;
        opts.block_size = 16 * 1024;
        opts.format_version = 6;
        return opts;
        }();
};

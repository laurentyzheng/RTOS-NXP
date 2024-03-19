#include "k_mem.h"
#include "lpc1768_mem.h"        // board memory map


//helper function for bit tree array --> k = level 
intptr_t get_start_addr (mpool_t pid);
intptr_t get_end_addr (mpool_t pid);
size_t get_tree_height(mpool_t pid);
size_t get_pool_size (mpool_t pid);

size_t get_depth(size_t idx, size_t k);
size_t get_block_size(mpool_t pid, size_t k);

void* get_addr(mpool_t pid, size_t idx, size_t k);
size_t get_idx(mpool_t pid, void* addr, size_t k);
size_t get_buddy_idx(int idx);

int get_level(mpool_t pid, size_t size);

#include "k_mem.h"

//helper function for free list
size_t print_and_count_list(mpool_t pid, NODE* head, U8* pHeap, size_t k);
void free_node(NODE* node);
int delete_node(NODE* to_delete);
int insert_node(NODE* lhead, void* addr);


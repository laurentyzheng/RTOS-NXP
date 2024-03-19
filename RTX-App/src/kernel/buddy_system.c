#include "buddy_system.h"
#include "k_inc.h"
#include "printf.h"

intptr_t get_start_addr (mpool_t pid)
{
	if (pid == MPID_IRAM1){
			return RAM1_START;
	} else if (pid == MPID_IRAM2) {
			return RAM2_START;
	} else {
			errno = EINVAL;
	return 0;
	}
}

intptr_t get_end_addr (mpool_t pid)
{
	if (pid == MPID_IRAM1){
			return RAM1_END;
	} else if (pid == MPID_IRAM2) {
			return RAM2_END;
	} else {
			errno = EINVAL;
	return 0;
	}
}


size_t get_tree_height(mpool_t pid)
{
	if (pid == MPID_IRAM1){
			return 8;
	} else if (pid == MPID_IRAM2) {
			return 11;
	} else {
			errno = EINVAL;
	return 0;
	}
}

size_t get_pool_size (mpool_t pid) 
{
	if (pid == MPID_IRAM1){
			return RAM1_SIZE;
	} else if (pid == MPID_IRAM2) {
			return RAM2_SIZE;
	} else {
			errno = EINVAL;
	return 0;
	}
}

size_t get_depth(size_t idx, size_t k)
{	
	size_t level_start = (0x01UL << k) - 1;

	if (idx < level_start){
		printf("ERROR: index = %n is located below the level at %n. ", idx, k);
		return NULL;
	}
	
	return idx - level_start;
}

size_t get_block_size(mpool_t pid, size_t k)
{
	size_t pool_size = get_pool_size(pid);
	return pool_size/(0x1UL << k);
}

void* get_addr(mpool_t pid, size_t idx, size_t k)
{
	size_t pool_size = get_pool_size(pid);
	intptr_t start_addr = get_start_addr(pid);

	if (pool_size <= 0 || start_addr <= 0){
		printf("ERROR: pid = %n not valid", pid);
		return NULL;
	}

	size_t depth = get_depth(idx, k);
	intptr_t offset = depth * (get_block_size(pid, k));
	
	return (void*)(start_addr + offset);
}

size_t get_idx(mpool_t pid, void* addr, size_t k)
{
	size_t pool_size = get_pool_size(pid);
	intptr_t start_addr = get_start_addr(pid);

	if (pool_size <= 0 || start_addr <= 0){
		printf("ERROR: pid = %n not valid", pid);
		return NULL;
	}
	
	size_t offset = (size_t)( (intptr_t)addr - start_addr );
	
	size_t pow_2_k = (0x1UL << k);
	return (pow_2_k*offset)/pool_size + pow_2_k - 1;
}

size_t get_buddy_idx(int idx)
{
	if (idx == 0)
		return 0;
	
	return !(idx & 1) ? idx - 1 : idx + 1;	
}

int get_level(mpool_t pid, size_t size)
{
	int k = 0;
	size_t pool_size = get_pool_size(pid);

	if (size > pool_size)
		return -1;
	
	int temp = pool_size/2;
	while(temp >= size && k < get_tree_height(pid)-1){
		temp /= 2;
		k++;
	}
	
	return k;
}


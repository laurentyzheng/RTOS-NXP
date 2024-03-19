

#include "k_inc.h"
#include "k_mem.h"
#include "buddy_system.h"
#include "free_list.h"
#include "bitheap.h"

/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |                           |     |
                              |---------------------------|     |
                              |                           |     |
                              |      other data           |     |
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
             g_k_stacks[15]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 

/*
 *===========================================================================
 *                            GLOBAL VARIABLES
 *===========================================================================
 */

//array representation of bit tree for IRAM1 and IRAM2, respectively
U8 heap_1 [32] = {0};
U8 heap_2 [256] = {0};
//free list of IRAM1 and IRAM2, respectively
NODE flist_1 [8];
NODE flist_2 [11];

void *g_k_stacks;

U32 g_p_stacks[NUM_TASKS][PROC_STACK_SIZE >> 2] __attribute__((aligned(8)));
/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */
/* note list[n] is for blocks with order of n */

mpool_t k_mpool_create (int algo, U32 start, U32 end)
{
    mpool_t mpid = MPID_IRAM1;

#ifdef DEBUG_0
    printf("k_mpool_init: algo = %d\r\n", algo);
    printf("k_mpool_init: RAM range: [0x%x, 0x%x].\r\n", start, end);
#endif /* DEBUG_0 */    
    
    if (algo != BUDDY ) {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    if ( start == RAM1_START) {
				flist_1[0].prev = NULL;
				flist_1[0].next = (void *) RAM1_START;
				flist_1[0].next->next = NULL;
				flist_1[0].next->prev = flist_1;
				for (int i = 1; i < 8; i++){
					flist_1[i].prev = flist_1[i].next = NULL;
				}
    } else if ( start == RAM2_START) {
        mpid = MPID_IRAM2;
				flist_2[0].prev = NULL;
				flist_2[0].next = (void *) RAM2_START;
				flist_2[0].next->next = NULL;
				flist_2[0].next->prev = flist_2;
				for (int i = 1; i < 11; i++){
					flist_2[i].prev = flist_2[i].next = NULL;
				}
    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    return mpid;
}

void *k_mpool_alloc (mpool_t mpid, size_t size)
{
#ifdef DEBUG_0
    printf("k_mpool_alloc: mpid = %d, size = %d, 0x%x\r\n", mpid, size, size);
#endif /* DEBUG_0 */
    U8* pHeap;
    NODE* pFlist;
    
		if( size == 0 )
			return NULL;
	
    if(mpid == MPID_IRAM1){
        pHeap = heap_1;
        pFlist = flist_1;
    } else if (mpid == MPID_IRAM2){
        pHeap = heap_2;
        pFlist = flist_2;
    } else {
        errno = EINVAL;
        return NULL;
    }
		
		// Filters out requests that are too big to handle.
		if( size > get_pool_size(mpid) )
		{
				errno = ENOMEM;
				return NULL;
		}
    
    // go down in levels to find the required level
    size_t k = get_level(mpid, size);

    // go up the list looking for the level with a free node
    size_t free_lvl = k;
    while(pFlist[free_lvl].next == NULL && free_lvl > 0) free_lvl--;
    
    void* ret = pFlist[free_lvl].next;
    if (ret == NULL){
        errno = ENOMEM;
        return NULL;
    }
        
    size_t idx = get_idx(mpid, pFlist[free_lvl].next, free_lvl);
    
		// iterate until find size needed.
    for(size_t i = free_lvl; i <= k; i++){
        set_bit(pHeap, idx, 1);
        delete_node(pFlist[i].next);
        if(i != free_lvl){
            void* bud_addr = get_addr(mpid, idx + 1, i);
            insert_node(pFlist+i, bud_addr);
        }
        idx = idx*2 + 1;
    }
		
    return ret;
}

int k_mpool_dealloc(mpool_t mpid, void *ptr)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d, ptr = 0x%x\r\n", mpid, ptr);
#endif /* DEBUG_0 */
    U8* pHeap;
    NODE* pFlist;
	
		if( ptr == NULL )
		{
				return 0;
		}
    else if( mpid == MPID_IRAM1 )
		{
        pHeap = heap_1;
        pFlist = flist_1;
    }
		else if( mpid == MPID_IRAM2 )
		{
        pHeap = heap_2;
        pFlist = flist_2;
    }
		else
		{
        errno = EINVAL;
        return -1;
    }
    
		// Filter it out if out of range.
    intptr_t ptr_addr = (intptr_t) ptr;
    if( (ptr_addr < get_start_addr( mpid )) || ( ptr_addr >= get_end_addr( mpid )) )
		{
        errno = EFAULT;
        return -1;
    }

    //find size of the block that ptr points to
    size_t cur_level;
    size_t max_level = get_tree_height(mpid) - 1;
    size_t depth = (ptr_addr - get_start_addr(mpid)) / 32;
    size_t idx = get_idx (mpid, ptr, max_level);
    
    //get the index, and the list to add the address
		// Note the fancy comma operator trick. Checkmate, ECE350 marker!
		cur_level = max_level;
		while( (idx = get_idx( mpid, ptr, cur_level )),	!get_bit( pHeap, idx ) )
			cur_level--;
		
    //update pHeap at ptr node
    set_bit( pHeap, idx, 0 );

    //coalesce until the buddy is filled
    while( (get_bit(pHeap, get_buddy_idx(idx)) == 0) && (idx != 0) )
    {
        //delete buddy from free list
        NODE* buddy_node = (NODE*) get_addr(mpid, get_buddy_idx(idx), cur_level);
        delete_node(buddy_node);

        //go to the parent and update
        idx = (idx - 1) / 2;
        cur_level--;
        set_bit(pHeap, idx, 0);
    }

    //add current level node to free list
    insert_node( pFlist+cur_level, get_addr( mpid, idx, cur_level ) );
    
    return RTX_OK; 
}

int k_mpool_dump (mpool_t mpid)
{
#ifdef DEBUG_0
    printf("k_mpool_dump: mpid = %d\r\n", mpid);
#endif /* DEBUG_0 */
		//goes through the free lists and printout the status and address of each node
		U8* pHeap;
    NODE* pFlist;
    
    if( mpid == MPID_IRAM1 )
		{
        pHeap = heap_1;
        pFlist = flist_1;
    }
		else if( mpid == MPID_IRAM2 )
		{
        pHeap = heap_2;
        pFlist = flist_2;
    }
		else
		{
        return 0;
    }
		
		size_t height = get_tree_height(mpid);
		size_t pool_size = get_pool_size(mpid);
		
		size_t free_block_count = 0;
		// This only works because k is signed.
		for( int k = height - 1; k >= 0; k-- )
		{
				free_block_count += print_and_count_list( mpid, pFlist+k, pHeap, k );
		}
		printf( "%d free memory block(s) found\r\n", free_block_count );
		
    return free_block_count;
}
 
int k_mem_init(int algo)
{
#ifdef DEBUG_0
    printf("k_mem_init: algo = %d\r\n", algo);
#endif /* DEBUG_0 */
        
    if( k_mpool_create(algo, RAM1_START, RAM1_END) < 0 )
		{
        return RTX_ERR;
    }
    
    if( k_mpool_create(algo, RAM2_START, RAM2_END) < 0 )
		{
        return RTX_ERR;
    }
    
    return RTX_OK;
}

U32* k_alloc_p_stack(task_t tid)
{
    if ( tid >= NUM_TASKS ) {
        errno = EAGAIN;
        return NULL;
    }
    
    U32 *sp = g_p_stacks[tid+1];
    
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */


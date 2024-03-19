#ifndef K_MEM_H_
#define K_MEM_H_
#include "k_inc.h"
#include "lpc1768_mem.h"        // board memory map

/*
 * ------------------------------------------------------------------------
 *                             FUNCTION PROTOTYPES
 * ------------------------------------------------------------------------
 */
// kernel API that requires mpool ID
typedef struct node {
    struct node* prev;
    struct node* next;
}NODE;

mpool_t k_mpool_create  (int algo, U32 strat, U32 end);
void   *k_mpool_alloc   (mpool_t mpid, size_t size);
int     k_mpool_dealloc (mpool_t mpid, void *ptr);
int     k_mpool_dump    (mpool_t mpid);

int     k_mem_init      (int algo);
U32    *k_alloc_p_stack (task_t tid);
// declare newly added functions here


/*
 * ------------------------------------------------------------------------
 *                             FUNCTION MACROS
 * ------------------------------------------------------------------------
 */

#endif // ! K_MEM_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */


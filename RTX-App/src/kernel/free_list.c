#include "free_list.h"
#include "k_inc.h"
#include "buddy_system.h"
#include "printf.h"
#include "bitheap.h"


size_t print_and_count_list(mpool_t pid, NODE* head, U8* pHeap, size_t k)
{
		size_t count = 0;
		for( NODE* trav = head->next; trav; trav = trav->next )
		{
				printf( "0x%x: 0x%x\r\n", trav, get_block_size( pid, k ) );
				count++;
		}
		return count;
}

void free_node(NODE* node)
{
		node->prev = NULL;
		node->next = NULL;
}

int delete_node(NODE* to_delete)
{
		if(to_delete == NULL)
			return -1;
		
		if(to_delete->next != NULL){
			to_delete->next->prev = to_delete->prev;
		}
		
		to_delete->prev->next = to_delete->next;
		
		free_node(to_delete);
		
		return 0;
}

int insert_node(NODE* head, void* addr)
{
		if(addr == NULL)
			return -1;
		
		
		NODE* to_insert = (NODE*) addr;
		
		//case: empty list
		if (head->next == NULL){
			head->next = to_insert;
			to_insert->prev = head;
			to_insert->next = NULL;
			return 0;
		}
		
		//case: non-empty list
		NODE* prev_insert = head;
		
		while(prev_insert->next < to_insert && prev_insert->next != NULL)
			prev_insert = prev_insert->next;
		
		// At this point,
		// [?] <-- [L] --> [prev]
		// [L] <-- [prev] --> [N (maybe NULL)]
		// [prev] <-- [N (maybe NULL)] --> [?]
		// [NULL] <-- [ins] --> [NULL]
		//
		to_insert->next = prev_insert->next;
		// At this point,
		// [?] <-- [L] --> [prev]
		// [L] <-- [prev] --> [N (maybe NULL)]
		// [prev] <-- [N (maybe NULL)] --> [?]
		// [NULL] <-- [ins] --> [N (maybe NULL)] <<<< delta
		//
		to_insert->prev = prev_insert;
		// At this point,
		// [?] <-- [L] --> [prev]
		// [L] <-- [prev] --> [N (maybe NULL)]
		// [prev] <-- [N (maybe NULL)] --> [?]
		// [prev] <-- [ins] --> [N (maybe NULL)]  <<<< delta
		//
		
		if(to_insert->next != NULL)
			to_insert->next->prev = to_insert;
		// At this point, (case not NULL)
		// [?] <-- [L] --> [prev]
		// [L] <-- [prev] --> [N]
		// [ins] <-- [N] --> [?] <<<< delta
		// [prev] <-- [ins] --> [N]
		//
		
		prev_insert->next = to_insert;
		// At this point, (case not NULL)
		// [?] <-- [L] --> [prev]
		// [L] <-- [prev] --> [ins] <<<< delta
		// [ins] <-- [N] --> [?]
		// [prev] <-- [ins] --> [N]
		//
		
		return 0;
}

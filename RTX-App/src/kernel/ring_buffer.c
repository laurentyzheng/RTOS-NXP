#include "ring_buffer.h"

int pushData_RingBuffer( RingBuffer * leBuff, U8 * leData, size_t dataLen )
{
    int did_work = RTX_OK;
    
    if( ( leBuff != NULL ) && ( leBuff->freeLength >= dataLen ) )
    {
        U8 * leTail = leBuff->tail;
        intptr_t start = (intptr_t) leBuff->start;
        
        leBuff->freeLength -= dataLen;
        
        // BEHOLD, The forbidden arrow operator!
        while( dataLen --> 0 )
        {
            *leTail = *leData;
            leTail++;
            leData++;
            if( (intptr_t)leTail >= (intptr_t) ( start + leBuff->maxLength ) )
                leTail -= leBuff->maxLength;
        }
        // note that past this point, dataLen is 0
        
        leBuff->tail = leTail;
    }
    else
        did_work = RTX_ERR;
    
    return did_work;
}

int popData_RingBuffer( RingBuffer * leBuff, U8 * leData, size_t dataLen, int doPopNotRead )
{
    int did_work = RTX_OK;
    
    if( ( leBuff != NULL ) && ( ( leBuff->maxLength - leBuff->freeLength ) >= dataLen ) )
    {
        intptr_t start = (intptr_t) leBuff->start;
        U8 *leHead = leBuff->head;
        if( doPopNotRead )
					leBuff->freeLength += dataLen;
        
        // BEHOLD, The forbidden arrow operator!
        while( dataLen --> 0 )
        {
            *leData = *leHead;
            leData++;
            leHead++;
            
            if( (intptr_t)leHead >= (intptr_t) ( start + leBuff->maxLength ) )
                leHead -= leBuff->maxLength;

        }
        // note that past this point, dataLen is 0
        if( doPopNotRead )
					leBuff->head = leHead;
    }
    else
        did_work = RTX_ERR;
    
    return did_work;
}

size_t readMessageSize_RingBuffer( RingBuffer * leBuff )
{
	RTX_MSG_HDR message_header = {0};
	popData_RingBuffer( leBuff, (U8*) &message_header, sizeof(RTX_MSG_HDR), 0 );
	return message_header.length;
}

size_t readMessageSenderId_RingBuffer( RingBuffer * leBuff )
{
	RTX_MSG_HDR message_header = {0};
	popData_RingBuffer( leBuff, (U8*) &message_header, sizeof(RTX_MSG_HDR), 0 );
	return message_header.sender_tid;
}

int popMessage_RingBuffer( RingBuffer * leBuff, U8 * leData, size_t maxDataLength )
{
	
		if( leBuff == NULL )
			return RTX_ERR;
		
		if( leBuff->freeLength == leBuff->maxLength ) //nothing in the buffer
			return RTX_ERR;
		
//		task_t first_sender = readMessageSenderId_RingBuffer( leBuff );
//		if ((first_sender >= MAX_TASKS ||  first_sender <= TID_NULL) && (first_sender != TID_UART)){
//			//garbage data, reset ring buffer
//			leBuff->freeLength = leBuff->maxLength;
//			leBuff->head = leBuff->start;
//			leBuff->tail = leBuff->start;
//			return RTX_ERR;
//		}
		
		// Note that the single = sign is intentional
		size_t msgSize;
		if( (( msgSize = readMessageSize_RingBuffer( leBuff ) ) == 0) || maxDataLength < msgSize )
			return RTX_ERR;
		
		return popData_RingBuffer( leBuff, leData, msgSize, 1 );
}


#ifndef RingBuffer_H
#define RingBuffer_H

#include "k_inc.h"


int pushData_RingBuffer( RingBuffer * leBuff, U8 * leData, size_t dataLen );
int popData_RingBuffer( RingBuffer * leBuff, U8 * leData, size_t dataLen, int doPopNotRead );
size_t readMessageSize_RingBuffer( RingBuffer * leBuff );
int popMessage_RingBuffer( RingBuffer * leBuff, U8 * leData, size_t maxDataLength );


#endif

#ifndef BITHEAP_H
#define BITHEAP_H

#include "k_inc.h"

U8 get_bit( U8 * bitArray, size_t offset );
void set_bit( U8 * bitArray, size_t offset, U8 bit );

void set_bit_heap( U8 * leHeap, size_t level, size_t offset, U8 bit );
U8 get_bit_heap( U8 * leHeap, size_t level, size_t offset );

#endif

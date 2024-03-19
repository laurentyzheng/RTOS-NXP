#include "bitheap.h"

U8 get_bit( U8 * bitArray, size_t array_offset )
{
	size_t byte_offset = array_offset >> 3;
	size_t bit_offset = array_offset & 0x7;
	return (bitArray[byte_offset] >> bit_offset) & 0x1;
}

void set_bit( U8 * bitArray, size_t offset, U8 bit )
{
	size_t byte_offset = offset >> 3;
	size_t bit_offset = offset & 0x7;

	if( bit )
		bitArray[byte_offset] |= (1 << bit_offset);
	else
		bitArray[byte_offset] &= ~(1 << bit_offset);
}

void set_bit_heap( U8 * pHeap, size_t level, size_t offset, U8 bit )
{
	size_t arrayOffset = ( 0x1UL << level ) + offset - 1;
	set_bit( pHeap, arrayOffset, bit );
}

U8 get_bit_heap( U8 * pHeap, size_t level, size_t offset )
{
	size_t arrayOffset = ( 0x1UL << level ) + offset - 1;
	return get_bit( pHeap, arrayOffset );
}

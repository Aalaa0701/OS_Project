/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"
bool is_initialized = 0;


//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	uint32 id = 0;
	LIST_FOREACH(blk, &list)
	{
		cprintf("%d (size: %d, isFree: %d)\n", id, blk->size, blk->is_free) ;
		id++;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//=========================================
	//DON'T CHANGE THESE LINES=================
	if (initSizeOfAllocatedSpace == 0)
		return ;
	is_initialized = 1;
	//=========================================
	//=========================================

	//TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()
	if((struct BlockMetaData*)daStart == NULL){
		return;
	}
	LIST_INIT(&myListOfBlocks);
	struct BlockMetaData* first_Block;
	first_Block = (struct BlockMetaData*)daStart;
	LIST_INSERT_HEAD(&myListOfBlocks, first_Block);
	first_Block->size=initSizeOfAllocatedSpace;
	first_Block->is_free=1;
	first_Block->prev_next_info.le_next=NULL;
	first_Block->prev_next_info.le_prev=NULL;


}




//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *alloc_block_FF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	//panic("alloc_block_FF is not implemented yet");
	struct BlockMetaData* _Block;
	uint32 size_to_be_allocated = size+(unsigned int)sizeOfMetaData();
	uint32 address_offset_2 = size_to_be_allocated;
	uint8 found = 0;

	if(size==0){
		return NULL;
	}
	if (!is_initialized)
	{
		uint32 required_size = size + sizeOfMetaData();
		uint32 da_start = (uint32)sbrk(required_size);
		//get new break since it's page aligned! thus, the size can be more than the required one
		uint32 da_break = (uint32)sbrk(0);
		initialize_dynamic_allocator(da_start, da_break - da_start);
	}
	int listsize = LIST_SIZE(&myListOfBlocks);
	struct BlockMetaData* list_iterator = LIST_FIRST(&myListOfBlocks);
	for(int i = 0; i < listsize; i++){
//		if(list_iterator == 0){
//			break;
//		}
		if(list_iterator->is_free == 1 && list_iterator->size >= size_to_be_allocated){
			uint32 size_remaining = list_iterator->size - size_to_be_allocated;
			if(size_remaining > (uint32)sizeOfMetaData()){
				found = 1;
				list_iterator->is_free = 0;
				list_iterator->size = size_to_be_allocated;
				_Block = (struct BlockMetaData*)((unsigned int)list_iterator + address_offset_2);
				_Block->is_free = 1;
				_Block->size = size_remaining;
				LIST_INSERT_AFTER(&myListOfBlocks, list_iterator, _Block);
				struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)list_iterator + (unsigned int)sizeOfMetaData());
				return address;

			}
			else{
				found = 1;
				list_iterator->is_free = 0;
				list_iterator->size = size_to_be_allocated + size_remaining;
				struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)list_iterator + (unsigned int)sizeOfMetaData());
				return address;
			}
		}
		else{
			list_iterator = list_iterator->prev_next_info.le_next;
			found = 0;
			continue;
		}
	}

	if(found == 0){

		uint32 returned_address = (uint32)sbrk(size_to_be_allocated);
		if(returned_address != 0 && returned_address != -1){

			uint32 allowed_to = (uint32)sbrk(0);
			uint32 size_allocated_in_sbrk = allowed_to - returned_address;
			_Block = (struct BlockMetaData*)returned_address;
			LIST_INSERT_TAIL(&myListOfBlocks, _Block);
			_Block->is_free = 0;
			_Block->size = size_to_be_allocated;
			uint32 start_size_free = (uint32)_Block + _Block->size;
			uint32 remaining_size = allowed_to - start_size_free;
			if(remaining_size > (uint32)sizeOfMetaData()){
				struct BlockMetaData*  new_block = (struct BlockMetaData*)((unsigned int)_Block + _Block->size);
				LIST_INSERT_TAIL(&myListOfBlocks, new_block);
				new_block->is_free = 1;
				new_block->size = remaining_size;
			}
			else{
				_Block->size = size_to_be_allocated + remaining_size;
			}
			struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)_Block + (unsigned int)sizeOfMetaData());
			return address;
		}
	}
		return NULL;
	}

//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
	//TODO: [PROJECT'23.MS1 - BONUS] [3] DYNAMIC ALLOCATOR - alloc_block_BF()
	//panic("alloc_block_BF is not implemented yet");
	struct BlockMetaData* _Block;
	struct BlockMetaData* min_block = NULL;
	struct BlockMetaData Block;
	uint32 size_to_be_allocated = size+(unsigned int)sizeOfMetaData();
	uint32 address_offset_2 = size_to_be_allocated;
	uint8 found = 0;

	int listsize = LIST_SIZE(&myListOfBlocks);
	if(size==0){
		return NULL;
	}
	// case 1 -> list has one block which is free
	if(listsize == 1 && LIST_FIRST(&myListOfBlocks)->is_free == 1){
		// case size is greater -> sbrk
		if(size_to_be_allocated > LIST_FIRST(&myListOfBlocks)->size){
			uint32 size_requested = size_to_be_allocated - LIST_FIRST(&myListOfBlocks)->size;
			if((int)sbrk(size_requested) == -1){
				return NULL;
			}
			else{
				LIST_FIRST(&myListOfBlocks)->is_free = 0;
				LIST_FIRST(&myListOfBlocks)->size = size_to_be_allocated;
				struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)LIST_FIRST(&myListOfBlocks)+(unsigned int)sizeOfMetaData());
				return address;
			}
		}
		// case size is equal
		else if(size_to_be_allocated == LIST_FIRST(&myListOfBlocks)->size){
			LIST_FIRST(&myListOfBlocks)->is_free = 0;
			LIST_FIRST(&myListOfBlocks)->size = size_to_be_allocated;
			struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)LIST_FIRST(&myListOfBlocks)+(unsigned int)sizeOfMetaData());
			return address;
		}
		// case size is less
		else{
			uint32 size_first = LIST_FIRST(&myListOfBlocks)->size;
			uint32 size_remaining = size_first - size_to_be_allocated;
			if(size_remaining >= sizeOfMetaData()){
				_Block = (struct BlockMetaData*)((unsigned int)LIST_FIRST(&myListOfBlocks) + size_to_be_allocated);
				LIST_INSERT_TAIL(&myListOfBlocks,_Block);
				LIST_FIRST(&myListOfBlocks)->size = size_to_be_allocated;
				LIST_FIRST(&myListOfBlocks)->is_free = 0;
				LIST_LAST(&myListOfBlocks)->size = size_remaining;
				LIST_LAST(&myListOfBlocks)->is_free = 1;
			}
			else{
				LIST_FIRST(&myListOfBlocks)->size = size_to_be_allocated + size_remaining;
				LIST_FIRST(&myListOfBlocks)->is_free = 0;
			}
			struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)LIST_FIRST(&myListOfBlocks)+sizeOfMetaData());
			return address;
		}
	}
	// case 2 list has one block which is not free
	else if(listsize == 1 && LIST_FIRST(&myListOfBlocks)->is_free == 0){
		if((int)sbrk(size_to_be_allocated) == -1){
			return NULL;
		}
		else{
			_Block = (struct BlockMetaData*)((unsigned int)LIST_FIRST(&myListOfBlocks) + address_offset_2);
			LIST_INSERT_TAIL(&myListOfBlocks, _Block);
			LIST_LAST(&myListOfBlocks)->is_free = 0;
			LIST_LAST(&myListOfBlocks)->size = size_to_be_allocated;
			struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)LIST_LAST(&myListOfBlocks)+(unsigned int)sizeOfMetaData());
			return address;
		}
	}
	// case 3 list has more than one block
	else if(listsize > 1){
		struct BlockMetaData* temp_iterator;
		LIST_FOREACH(temp_iterator, &myListOfBlocks){
			if(temp_iterator->is_free == 1 && size_to_be_allocated <= temp_iterator->size){
				if(size_to_be_allocated == temp_iterator->size){
					temp_iterator->is_free = 0;
					temp_iterator->size = size_to_be_allocated;
					struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)temp_iterator + (unsigned int)sizeOfMetaData());
					return address;
				}
				min_block = temp_iterator;
				break;
			}
		}
		if(min_block != NULL){
			struct BlockMetaData* iterator;
			// loop on the list
			LIST_FOREACH(iterator, &myListOfBlocks){
				// case size is greater than block
				if(iterator->is_free == 1 && size_to_be_allocated > iterator->size){
					continue;
				}
				// case size is equal to block
				else if(iterator->is_free == 1 && size_to_be_allocated == iterator->size){
					iterator->is_free = 0;
					iterator->size = size_to_be_allocated;
					struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)iterator + (unsigned int)sizeOfMetaData());
					return address;
				}
				// case size is less than size of block && not last element
				else if(iterator->is_free == 1 && size_to_be_allocated < iterator->size && min_block != NULL){
					if(iterator->size - size_to_be_allocated >= min_block->size - size_to_be_allocated){
						continue;
					}
					else{
						min_block = iterator;
						found = 1;
					}
				}
		}

		}
	}

	if(found == 0 && min_block == NULL){
		if((int)sbrk(size_to_be_allocated) == -1){
			return NULL;
		}
		else{
			_Block = (struct BlockMetaData*)((unsigned int)LIST_LAST(&myListOfBlocks) + size_to_be_allocated);
			LIST_INSERT_TAIL(&myListOfBlocks, _Block);
			_Block->is_free = 0;
			_Block->size = size_to_be_allocated;
			struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)_Block + (unsigned int)sizeOfMetaData());
			return address;

		}

	}
	else if(found == 0 && min_block != NULL){
		uint32 remaining_size = min_block->size - size_to_be_allocated;
		if(remaining_size >= (unsigned int)sizeOfMetaData()){
			_Block = (struct BlockMetaData*)((unsigned int)min_block + size_to_be_allocated);
			min_block->size = size_to_be_allocated;
			min_block->is_free = 0;
			LIST_INSERT_AFTER(&myListOfBlocks, min_block, _Block);
			_Block->size = remaining_size;
			_Block->is_free = 1;
		}
		else{
			min_block->size = size_to_be_allocated + remaining_size;
			min_block->is_free = 0;
		}
		struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)min_block + (unsigned int)sizeOfMetaData());
		return address;
	}
	else if (found == 1){
		uint32 remaining_size = min_block->size - size_to_be_allocated;
		if(remaining_size >= (unsigned int)sizeOfMetaData()){
			_Block = (struct BlockMetaData*)((unsigned int)min_block + size_to_be_allocated);
			min_block->size = size_to_be_allocated;
			min_block->is_free = 0;
			LIST_INSERT_AFTER(&myListOfBlocks, min_block, _Block);
			_Block->size = remaining_size;
			_Block->is_free = 1;
		}
		else{
			min_block->size = size_to_be_allocated + remaining_size;
			min_block->is_free = 0;
		}
		struct BlockMetaData* address = (struct BlockMetaData*)((unsigned int)min_block + (unsigned int)sizeOfMetaData());
		return address;
	}

	cprintf("last null\n");
	return NULL;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");
	//case address is null
	if(va == NULL){
		return;
	}
	uint32 list_size = LIST_SIZE(&myListOfBlocks);
	struct BlockMetaData * block_to_be_deleted;
	void* new_address = va - (unsigned int)sizeOfMetaData();
	block_to_be_deleted = new_address;

	struct BlockMetaData * block_prev = LIST_PREV(block_to_be_deleted);
	if(block_to_be_deleted->is_free == 1){
		return;
	}
	// cases block is last element
	if(block_to_be_deleted == LIST_LAST(&myListOfBlocks)){
		// case 1 -> prev is not free
		if(block_to_be_deleted->prev_next_info.le_prev->is_free == 0){
			block_to_be_deleted->is_free = 1;
			return;
		}
		// case 2 -> prev is free -> merge current block into prev
		else{
			LIST_PREV(block_to_be_deleted)->size += block_to_be_deleted->size;
			block_to_be_deleted->is_free = 0;
			block_to_be_deleted->size = 0;

			LIST_REMOVE(&(myListOfBlocks), block_to_be_deleted);

			return;
			struct BlockMetaData* prev_block = block_to_be_deleted->prev_next_info.le_prev;
			prev_block->prev_next_info.le_next = NULL;
		}
		return;
	}
	// cases block is first element
	else if(block_to_be_deleted == LIST_FIRST(&myListOfBlocks)){
		// case 1 -> next is not free
		if(block_to_be_deleted->prev_next_info.le_next->is_free == 0){
			block_to_be_deleted->is_free = 1;
			return;
		}
		// case 2 -> next is free
		else{
			uint32 added_size = block_to_be_deleted->prev_next_info.le_next->size;





			block_to_be_deleted->size += added_size;
			block_to_be_deleted->is_free = 1;
			block_to_be_deleted->prev_next_info.le_next->is_free = 0;
			block_to_be_deleted->prev_next_info.le_next->size = 0;

			struct BlockMetaData* next_block = block_to_be_deleted->prev_next_info.le_next;
			LIST_REMOVE(&(myListOfBlocks), next_block);

			return;
			//struct BlockMetaData* next_block = block_to_be_deleted->prev_next_info.le_next->prev_next_info.le_next;
			block_to_be_deleted->prev_next_info.le_next = next_block;
			next_block->prev_next_info.le_prev = block_to_be_deleted;

		}
		return;
	}
	// cases where block is in middle
	else{
		// case 1 -> prev and next not free
		if(block_to_be_deleted->prev_next_info.le_prev->is_free == 0 && block_to_be_deleted->prev_next_info.le_next->is_free == 0){
			block_to_be_deleted->is_free = 1;
			return;
		}
		// case 2 -> prev not free but next is free
		else if(block_to_be_deleted->prev_next_info.le_prev->is_free == 0 && block_to_be_deleted->prev_next_info.le_next->is_free == 1){
			struct BlockMetaData* next_block = block_to_be_deleted->prev_next_info.le_next;
			uint32 added_size = next_block->size;
			block_to_be_deleted->size += added_size;
			block_to_be_deleted->is_free = 1;
			next_block->is_free = 0;
			next_block->size = 0;

			LIST_REMOVE(&(myListOfBlocks), next_block);

			return;
			struct BlockMetaData* next_of_next_block = next_block->prev_next_info.le_next;
			block_to_be_deleted->prev_next_info.le_next = next_of_next_block;
			next_of_next_block->prev_next_info.le_prev = block_to_be_deleted;
			return;

		}
		// case 3 -> prev is free but next not free
		else if(block_to_be_deleted->prev_next_info.le_prev->is_free == 1 && block_to_be_deleted->prev_next_info.le_next->is_free == 0){
			struct BlockMetaData* next_block = block_to_be_deleted->prev_next_info.le_next;
			struct BlockMetaData* prev_block = block_to_be_deleted->prev_next_info.le_prev;
			uint32 added_size = block_to_be_deleted->size;
			prev_block->size += added_size;
			block_to_be_deleted->is_free = 0;
			block_to_be_deleted->size = 0;

			LIST_REMOVE(&(myListOfBlocks), block_to_be_deleted);

			return;

			prev_block->prev_next_info.le_next = next_block;
			next_block->prev_next_info.le_prev = prev_block;
			return;
		}

		// case 4 -> prev and next are free
		else{
			struct BlockMetaData* prev_block = block_to_be_deleted->prev_next_info.le_prev;
			struct BlockMetaData* next_block = block_to_be_deleted->prev_next_info.le_next;
			struct BlockMetaData* next_of_next = next_block->prev_next_info.le_next;
			uint32 added_size = block_to_be_deleted->size + next_block->size;
			next_block->is_free = 0;
			next_block->size = 0;
			block_to_be_deleted->is_free = 0;
			block_to_be_deleted->size = 0;
			prev_block->size += added_size;

			LIST_REMOVE(&(myListOfBlocks), next_block);
			LIST_REMOVE(&(myListOfBlocks), block_to_be_deleted);
//			uint32 added_size = block_to_be_deleted->size + block_to_be_deleted->prev_next_info.le_next->size;
//			struct BlockMetaData* prev_block = block_to_be_deleted->prev_next_info.le_prev;
//			struct BlockMetaData* next_block = block_to_be_deleted->prev_next_info.le_next;
//			prev_block += added_size;
//			block_to_be_deleted->prev_next_info.le_next->is_free = 0;
//			block_to_be_deleted->prev_next_info.le_next->size = 0;
//			block_to_be_deleted->is_free = 0;
//			block_to_be_deleted->size = 0;
//			LIST_REMOVE(&myListOfBlocks, block_to_be_deleted);
//			LIST_REMOVE(&myListOfBlocks, next_block);
			return;
		}
		return;
	}


}

//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");

	if(new_size == 0){
		free_block(va);
		return NULL;
	}
	else if(new_size == 0 && va == NULL){
		return NULL;
	}
	else if(new_size != 0 && va == NULL){
		return alloc_block_FF(new_size);
	}
	else{
		void* new_address = va - sizeOfMetaData();
		struct BlockMetaData* currentBlockMetaData = new_address;
		struct BlockMetaData* nextBlockMetaData;
		nextBlockMetaData = LIST_NEXT(currentBlockMetaData);
		uint32 increased_size = -1;
		uint32 decreased_size = -1;
		uint32 real_size = currentBlockMetaData->size - (unsigned int)sizeOfMetaData();
		if(new_size > real_size){
			increased_size = new_size - real_size;
		}
		else if(new_size < real_size){
			decreased_size = real_size - new_size;
		}
		else{
			return va;
		}
		// case -> size increased
		if(increased_size != -1){
			// case block not last element and not first
			if(currentBlockMetaData != LIST_LAST(&myListOfBlocks) && currentBlockMetaData != LIST_LAST(&myListOfBlocks)){
				// case next not free
				uint32 next_size = nextBlockMetaData->size;
				void* next_address = (struct BlockMetaData*)((unsigned int)nextBlockMetaData + (unsigned int)sizeOfMetaData());
				if(nextBlockMetaData->is_free == 0){
					//currentBlockMetaData->is_free = 1;
					short *new_start = va;
	                short *new_mid = va + real_size/2;
	                short *new_end = va + real_size - sizeof(short);
					short* returned_address = alloc_block_FF(new_size);
					if(returned_address == 0){
						return va;
					}
	                short *returned_mid = (short*)((unsigned int)returned_address + real_size/2);
	                short *returned_end = (short*)((unsigned int)returned_address + real_size - sizeof(short));
					*(returned_address) = *(new_start);
					*(returned_mid) = *(new_start);
					*(returned_end) = *(new_start);
					free_block(va);
					return returned_address;
				}
				// case next free
				else{
					// case next has more size
					if(nextBlockMetaData->size > increased_size){
						if(nextBlockMetaData->size - increased_size > sizeOfMetaData()){

							uint32 size_of_removed = nextBlockMetaData->size;
							nextBlockMetaData->size = 0;
							nextBlockMetaData->is_free = 0;
							LIST_REMOVE(&myListOfBlocks, nextBlockMetaData);
							 struct BlockMetaData* new_block = (struct BlockMetaData*)((unsigned int)currentBlockMetaData + new_size + (unsigned int)sizeOfMetaData());

							currentBlockMetaData->size = new_size + (unsigned int)sizeOfMetaData();
							LIST_INSERT_AFTER(&myListOfBlocks, currentBlockMetaData, new_block);
							new_block->size = size_of_removed - increased_size;
							new_block->is_free = 1;
							return va;
						}
						else{
							uint32 remaining_size = nextBlockMetaData->size - increased_size;
							currentBlockMetaData->size += increased_size + remaining_size;
							free_block(next_address);
							return va;
						}

					}
					// case size is equal
					else if(nextBlockMetaData->size == increased_size){
						currentBlockMetaData->size += increased_size;
						free_block(next_address);
//						nextBlockMetaData->is_free = 0;
//						nextBlockMetaData->size = 0;
						return va;
					}
					// case size is less
					else{
						short *new_start = va;
		                short *new_mid = va + real_size/2;
		                short *new_end = va + real_size - sizeof(short);
						short* returned_address = alloc_block_FF(new_size);
						if(returned_address == 0){
							return va;
						}
		                short *returned_mid = (short*)((unsigned int)returned_address + real_size/2);
		                short *returned_end = (short*)((unsigned int)returned_address + real_size - sizeof(short));
						*(returned_address) = *(new_start);
						*(returned_mid) = *(new_start);
						*(returned_end) = *(new_start);
						free_block(va);
						return returned_address;
					}
					return va;
				}
			}
			// case element is first
			else if(currentBlockMetaData == LIST_FIRST(&myListOfBlocks)){
				void* next_address = (struct BlockMetaData*)((unsigned int)nextBlockMetaData + (unsigned int)sizeOfMetaData());
				// case next is free
				if(nextBlockMetaData->is_free == 1){
					//logic
					if(nextBlockMetaData->size > increased_size){
						if(nextBlockMetaData->size - increased_size > sizeOfMetaData()){

							uint32 size_of_removed = nextBlockMetaData->size;
							nextBlockMetaData->size = 0;
							nextBlockMetaData->is_free = 0;
							LIST_REMOVE(&myListOfBlocks, nextBlockMetaData);
							 struct BlockMetaData* new_block = (struct BlockMetaData*)((unsigned int)currentBlockMetaData + new_size + (unsigned int)sizeOfMetaData());

							currentBlockMetaData->size = new_size + (unsigned int)sizeOfMetaData();
							LIST_INSERT_AFTER(&myListOfBlocks, currentBlockMetaData, new_block);
							new_block->size = size_of_removed - increased_size;
							new_block->is_free = 1;
							return va;
						}
						else{
							uint32 remaining_size = nextBlockMetaData->size - increased_size;
							currentBlockMetaData->size += increased_size + remaining_size;
							free_block(next_address);
							return va;
						}

					}
					// case size is equal
					else if(nextBlockMetaData->size == increased_size){
						currentBlockMetaData->size += increased_size;
						free_block(next_address);
//						nextBlockMetaData->is_free = 0;
//						nextBlockMetaData->size = 0;
						return va;
					}
					// case size is less
					else{
						short *new_start = va;
		                short *new_mid = va + real_size/2;
		                short *new_end = va + real_size - sizeof(short);
						short* returned_address = alloc_block_FF(new_size);
						if(returned_address == 0){
							return va;
						}
		                short *returned_mid = (short*)((unsigned int)returned_address + real_size/2);
		                short *returned_end = (short*)((unsigned int)returned_address + real_size - sizeof(short));
						*(returned_address) = *(new_start);
						*(returned_mid) = *(new_start);
						*(returned_end) = *(new_start);
						free_block(va);
						return returned_address;
					}
					return va;
				}
				// case next not free
				else{
					short *new_start = va;
	                short *new_mid = va + real_size/2;
	                short *new_end = va + real_size - sizeof(short);
					short* returned_address = alloc_block_FF(new_size);
					if(returned_address == 0){
						return va;
					}
	                short *returned_mid = (short*)((unsigned int)returned_address + real_size/2);
	                short *returned_end = (short*)((unsigned int)returned_address + real_size - sizeof(short));
					*(returned_address) = *(new_start);
					*(returned_mid) = *(new_start);
					*(returned_end) = *(new_start);
					free_block(va);
					return returned_address;
				}
			}
			// case element is last
			else if(currentBlockMetaData == LIST_LAST(&myListOfBlocks)){
				short *new_start = va;
                short *new_mid = va + real_size/2;
                short *new_end = va + real_size - sizeof(short);
				short* returned_address = alloc_block_FF(new_size);
				if(returned_address == 0){
					return va;
				}
                short *returned_mid = (short*)((unsigned int)returned_address + real_size/2);
                short *returned_end = (short*)((unsigned int)returned_address + real_size - sizeof(short));
				*(returned_address) = *(new_start);
				*(returned_mid) = *(new_start);
				*(returned_end) = *(new_start);
				free_block(va);
				return returned_address;
			}

		}
		// case -> size decreased
		else if(decreased_size != -1){
			// case element not first or last
			if(currentBlockMetaData != LIST_LAST(&myListOfBlocks) && currentBlockMetaData != LIST_FIRST(&myListOfBlocks)){
				// case next is free
				if(nextBlockMetaData->is_free == 1){
					uint32 next_size = nextBlockMetaData->size;
					void* next_address = (struct BlockMetaData*)((unsigned int)nextBlockMetaData + (unsigned int)sizeOfMetaData());
					free_block(next_address);
					struct BlockMetaData* new_free_block = (struct BlockMetaData*)((unsigned int)currentBlockMetaData + (unsigned int)sizeOfMetaData() + new_size);
					new_free_block->is_free = 1;
					new_free_block->size = next_size + decreased_size;
					currentBlockMetaData->size -= decreased_size;
					LIST_INSERT_AFTER(&myListOfBlocks, currentBlockMetaData, new_free_block);
					return va;
				}
				// case next not free
				else{
					if(decreased_size > sizeOfMetaData()){
						currentBlockMetaData->size -= decreased_size;
						struct BlockMetaData* new_block = (struct BlockMetaData*)((unsigned int)currentBlockMetaData + currentBlockMetaData->size);
						LIST_INSERT_AFTER(&myListOfBlocks, currentBlockMetaData, new_block);
						new_block->is_free = 1;
						new_block->size = decreased_size;
					}
					return va;
				}
			}
			// case element is first
			else if(currentBlockMetaData == LIST_FIRST(&myListOfBlocks)){
				// case next is free
				if(nextBlockMetaData->is_free == 1){
					uint32 next_size = nextBlockMetaData->size;
					void* next_address = (struct BlockMetaData*)((unsigned int)nextBlockMetaData + (unsigned int)sizeOfMetaData());
					free_block(next_address);
					struct BlockMetaData* new_free_block = (struct BlockMetaData*)((unsigned int)currentBlockMetaData + (unsigned int)sizeOfMetaData() + new_size);
					new_free_block->is_free = 1;
					new_free_block->size = next_size + decreased_size;
					currentBlockMetaData->size -= decreased_size;
					LIST_INSERT_AFTER(&myListOfBlocks, currentBlockMetaData, new_free_block);
					return va;
				}
				// case next not free
				else{
					if(decreased_size > sizeOfMetaData()){
						struct BlockMetaData* new_block = (struct BlockMetaData*)((unsigned int)currentBlockMetaData + currentBlockMetaData->size);
						currentBlockMetaData->size -= decreased_size;
						LIST_INSERT_AFTER(&myListOfBlocks, currentBlockMetaData, new_block);
						new_block->is_free = 1;
					}
					return va;
				}
			}
			// case element is last
			else if(currentBlockMetaData == LIST_LAST(&myListOfBlocks)){
				if(decreased_size > sizeOfMetaData()){
					struct BlockMetaData* new_block = (struct BlockMetaData*)((unsigned int)currentBlockMetaData + currentBlockMetaData->size);
					currentBlockMetaData->size -= decreased_size;
					LIST_INSERT_AFTER(&myListOfBlocks, currentBlockMetaData, new_block);
					new_block->is_free = 1;
				}
				return va;
			}
		}

	}
return NULL;
}

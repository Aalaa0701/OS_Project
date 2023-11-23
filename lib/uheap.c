#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
#define pages_in_user_heap (((unsigned int)USER_HEAP_MAX - ((unsigned int)USER_HEAP_START + (unsigned int)DYN_ALLOC_MAX_SIZE) + (unsigned int)PAGE_SIZE)/PAGE_SIZE)
uint32 pages_allocated_in_uheap = 0;
int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================
void* malloc(uint32 size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	// Write your code here, remove the panic and write your code
	if(sys_isUHeapPlacementStrategyFIRSTFIT()){
		if(size <= (unsigned int)DYN_ALLOC_MAX_BLOCK_SIZE){
			struct BlockMetaData* v_address = alloc_block_FF(size);
			return v_address;
		}
		else{
			uint32 requested_pages;
			if(size % PAGE_SIZE == 0){
				requested_pages = size / PAGE_SIZE;
			}
			else{
				requested_pages = (size / PAGE_SIZE) + 1;
			}
			if(requested_pages > pages_in_user_heap - pages_allocated_in_uheap){
				return NULL;
			}
			else{
				struct Pages* v_address = NULL;
				if(LIST_FIRST(&list_of_pages) == NULL){
					struct Pages* first_page = (struct Pages*)(((unsigned int)USER_HEAP_START + (unsigned int)DYN_ALLOC_MAX_SIZE + (unsigned int)PAGE_SIZE));
					first_page->is_free = 0;
					first_page->size = requested_pages;
					v_address = first_page;
					LIST_INSERT_HEAD(&list_of_pages, first_page);
					pages_allocated_in_uheap += requested_pages;
					struct Pages* last_block = (struct Pages*)((unsigned int)LIST_FIRST(&list_of_pages)+ ((unsigned int)PAGE_SIZE * requested_pages));
					last_block->is_free = 1;
					last_block->size = pages_in_user_heap - requested_pages;
					LIST_INSERT_TAIL(&list_of_pages, last_block);
					sys_allocate_user_mem((uint32)v_address, requested_pages);
					return v_address;
				}
				else{
					struct Pages* iterator;
					LIST_FOREACH(iterator, &list_of_pages){
						if(iterator->is_free == 1 && iterator->size > requested_pages){
							uint32 old_size = iterator->size;
							iterator->is_free = 0;
							iterator->size = requested_pages;
							struct Pages* free_block = (struct Pages*)(((unsigned int)iterator + ((unsigned int)PAGE_SIZE * requested_pages)));
							free_block->is_free = 1;
							free_block->size = old_size - requested_pages;
							LIST_INSERT_AFTER(&list_of_pages, iterator, free_block);
							v_address = iterator;
							sys_allocate_user_mem((uint32)v_address, requested_pages);
							return v_address;
						}
						else if(iterator->is_free == 1 && iterator->size == requested_pages){
							iterator->is_free = 0;
							iterator->size = requested_pages;
							v_address = iterator;
							sys_allocate_user_mem((uint32)v_address, requested_pages);
							return v_address;
						}
						else{
							continue;
						}
					}
					cprintf("not found\n");
					return NULL;
				}
			}
		}
	}
	//panic("malloc() is not implemented yet...!!");
	return NULL;
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================
void free(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	// Write your code here, remove the panic and write your code
	//case address out of range
	if((uint32)virtual_address < (uint32)USER_HEAP_START || (uint32)virtual_address >= (uint32)USER_HEAP_MAX){
		panic("not a valid address\n");
	}
	//case address in block allocator range
	if((uint32)virtual_address >= (uint32)USER_HEAP_START && (uint32)virtual_address < (uint32)USER_HEAP_START + DYN_ALLOC_MAX_SIZE){
		free_block(virtual_address);
	}
	//case address in page allocator range
	else if((uint32)virtual_address >= ((uint32)USER_HEAP_START + (uint32)DYN_ALLOC_MAX_SIZE + (uint32)PAGE_SIZE) && (uint32)virtual_address < (uint32)USER_HEAP_MAX){
		struct Pages* pages_deleted = (struct Pages*)virtual_address;
		struct Pages* prev_pages;
		struct Pages* next_pages;
		if(pages_deleted == LIST_FIRST(&list_of_pages)){
			next_pages = pages_deleted->prev_next_info.le_next;
		}
		else if(pages_deleted == LIST_LAST(&list_of_pages)){
			prev_pages = pages_deleted->prev_next_info.le_prev;
		}
		else{
			next_pages = pages_deleted->prev_next_info.le_next;
			prev_pages = pages_deleted->prev_next_info.le_prev;
		}
		//free logic

		//case 1 first element
		if(pages_deleted == LIST_FIRST(&list_of_pages)){
			//case next free
			if(next_pages->is_free == 1){
				pages_deleted->size += next_pages->size;
				pages_deleted->is_free = 1;
				struct Pages* next_of_next = next_pages->prev_next_info.le_next;
				next_pages->is_free = 0;
				next_pages->size = 0;
				pages_deleted->prev_next_info.le_next = next_of_next;
				next_of_next->prev_next_info.le_prev = pages_deleted;
			}
			//case next not free
			else{
				pages_deleted->is_free = 1;
			}
		}

		//case 2 last element
		else if(pages_deleted == LIST_LAST(&list_of_pages)){
			//case prev free
			if(prev_pages->is_free == 1){
				prev_pages->size += pages_deleted->size;
				pages_deleted->is_free = 0;
				pages_deleted->size = 0;
				prev_pages->prev_next_info.le_next = NULL;
			}
			//case prev not free
			else{
				pages_deleted->is_free = 1;
			}
		}

		//case 3 middle
		else{
			//case prev and next not free
			if(prev_pages->is_free == 0 && next_pages->is_free == 0){
				pages_deleted->is_free = 1;
			}
			//case prev free and next not free
			else if(prev_pages->is_free == 1 && next_pages->is_free == 0){
				prev_pages->size += pages_deleted->size;
				pages_deleted->is_free = 0;
				pages_deleted->size = 0;
				prev_pages->prev_next_info.le_next = next_pages;
			}
			//case prev not free and next free
			else if(prev_pages->is_free == 0 && next_pages->is_free == 1){
				struct Pages* next_of_next = next_pages->prev_next_info.le_next;
				pages_deleted->size += next_pages->size;
				pages_deleted->is_free = 1;
				next_pages->is_free = 0;
				next_pages->size = 0;
				pages_deleted->prev_next_info.le_next = next_of_next;
				next_of_next->prev_next_info.le_prev = pages_deleted;
			}
			//case prev and next free
			else if(prev_pages->is_free == 1 && next_pages->is_free == 1){
				struct Pages* next_of_next = next_pages->prev_next_info.le_next;
				prev_pages->size += (pages_deleted->size + next_pages->size);
				pages_deleted->is_free = 0;
				pages_deleted->size = 0;
				next_pages->is_free = 0;
				next_pages->size = 0;
				prev_pages->prev_next_info.le_next = next_of_next;
				next_of_next->prev_next_info.le_prev = prev_pages;
			}
		}

		sys_free_user_mem((uint32)virtual_address, pages_deleted->size);
	}
}


//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}

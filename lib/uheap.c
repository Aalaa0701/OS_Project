#include <inc/lib.h>

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//
#define pages_in_user_heap (((unsigned int)USER_HEAP_MAX - ((unsigned int)USER_HEAP_START + (unsigned int)DYN_ALLOC_MAX_SIZE) + (unsigned int)PAGE_SIZE)/PAGE_SIZE)
uint32 pages_allocated_in_uheap = 0;
int FirstTimeFlag = 1;
uint32 save_pages[12280] = {0};//122881
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
				uint32 start_address = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE;
				uint32 found_address = 0;
				for(int i = 0; i < pages_in_user_heap; i++){
					uint32 iteration_address = start_address + ((uint32)PAGE_SIZE * i);
					uint32 permissions = sys_get_permissions(iteration_address);
					uint32 num_of_free_pages = 0;
					if(permissions == -1 || permissions == 0){
						num_of_free_pages++;
						int m = 1;
						while(1 == 1){
							uint32 next_permissions = sys_get_permissions(start_address + ((uint32)PAGE_SIZE * i) + ((uint32)PAGE_SIZE * m));
							if(next_permissions == -1 || next_permissions == 0){
								m++;
								num_of_free_pages++;
								if(num_of_free_pages == requested_pages){
									found_address = start_address + ((uint32)PAGE_SIZE * i);
									break;
								}
							}
							else{
								break;
							}
						}
						if(num_of_free_pages < requested_pages){
							i += m;
							continue;
						}
						else if(num_of_free_pages == requested_pages){
							found_address = start_address + ((uint32)PAGE_SIZE * i);
							break;
						}
						else{
							found_address = (uint32)start_address + ((uint32)PAGE_SIZE * i);
							break;
						}
					}
					else{
						continue;
					}
				}
				if(found_address != 0){
					//assign page and mark it
					sys_allocate_user_mem(found_address, requested_pages);
					int index_to_save_page = ((found_address - start_address) / (uint32)PAGE_SIZE);
					save_pages[index_to_save_page] = (int)requested_pages;
					pages_allocated_in_uheap += requested_pages;
					return (void*)found_address;
				}

			}
		}
	}
	return NULL;
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
		uint32 new_va = ROUNDDOWN((uint32)virtual_address, PAGE_SIZE);
		uint32 start_address = USER_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE;
		uint32 index = ((new_va - start_address) / (uint32)PAGE_SIZE);
		uint32 size_of_pages = save_pages[index];
		save_pages[index] = 0;
		sys_free_user_mem((uint32)virtual_address, size_of_pages);
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

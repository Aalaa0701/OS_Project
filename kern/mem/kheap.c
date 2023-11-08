#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

uint32 num_of_pages_allocated = 0;
#define num_of_pages_in_allocator (((unsigned int)KERNEL_HEAP_MAX - (hard_limit + (unsigned int)PAGE_SIZE)) / (unsigned int)PAGE_SIZE)

int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM
	if (initSizeToAllocate > (daLimit-daStart)|| daLimit == daStart){
		return E_NO_MEM;
	}
	KHstart = daStart;
	segment_break = KHstart + initSizeToAllocate;
	hard_limit = daLimit;
	struct FrameInfo* frame_allocated = NULL;
	allocate_frame(&frame_allocated);
	map_frame(ptr_page_directory, frame_allocated, daStart, PERM_WRITEABLE);
	initialize_dynamic_allocator(KHstart,initSizeToAllocate);
	//Comment the following line(s) before start coding...
	//panic("not implemented yet");
	return 0;

}

void* sbrk(int increment)
{
	//TODO: [PROJECT'23.MS2 - #02] [1] KERNEL HEAP - sbrk()
	/* increment > 0: move the segment break of the kernel to increase the size of its heap,
	 * 				you should allocate pages and map them into the kernel virtual address space as necessary,
	 * 				and returns the address of the previous break (i.e. the beginning of newly mapped memory).
	 * increment = 0: just return the current position of the segment break
	 * increment < 0: move the segment break of the kernel to decrease the size of its heap,
	 * 				you should deallocate pages that no longer contain part of the heap as necessary.
	 * 				and returns the address of the new break (i.e. the end of the current heap space).
	 *
	 * NOTES:
	 * 	1) You should only have to allocate or deallocate pages if the segment break crosses a page boundary
	 * 	2) New segment break should be aligned on page-boundary to avoid "No Man's Land" problem
	 * 	3) Allocating additional pages for a kernel dynamic allocator will fail if the free frames are exhausted
	 * 		or the break exceed the limit of the dynamic allocator. If sbrk fails, kernel should panic(...)
	 */

	//MS2: COMMENT THIS LINE BEFORE START CODING====
	return (void*)-1 ;
	panic("not implemented yet");
}


void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	if(isKHeapPlacementStrategyFIRSTFIT() == 1){
		cprintf("first fit\n");
		// case size greater than heap
		if(size >= ((uint32)KERNEL_HEAP_MAX) - ((uint32)KERNEL_HEAP_START + (uint32)DYN_ALLOC_MAX_SIZE + (uint32)PAGE_SIZE)){
			cprintf("first null\n");
			return NULL;
		}
		struct BlockMetaData* v_address;
		// case allocator ms1
		if(size <= (uint32)DYN_ALLOC_MAX_BLOCK_SIZE){
			cprintf("size block\n");
			v_address = alloc_block_FF(size);
			cprintf("before frame in block\n");
			struct FrameInfo* frame_allocated = NULL;
			allocate_frame(&frame_allocated);
			map_frame(ptr_page_directory, frame_allocated, (uint32)v_address, PERM_WRITEABLE);
			return v_address;
		}
		// case page allocator
		else{
			cprintf("size page\n");
			uint32 number_of_pages;
			struct BlockMetaData* iterator;
			if(size % (uint32)PAGE_SIZE == 0){
				number_of_pages = size / (uint32)PAGE_SIZE;
			}
			else{
				number_of_pages = (size / (uint32)PAGE_SIZE) + 1;
			}
			cprintf("num of pages: %d\n", number_of_pages);
			if(number_of_pages <= num_of_pages_in_allocator){
				if(LIST_FIRST(&list_of_pages) == NULL){
					cprintf("null list\n");
					for(int i = 0; i < number_of_pages; i++){
						cprintf("loop\n");
						if(i == 0){
							struct BlockMetaData* first_page = (struct BlockMetaData*)(hard_limit + (unsigned int)PAGE_SIZE);
							cprintf("assign address\n");
							struct FrameInfo* frame_allocated = NULL;
							allocate_frame(&frame_allocated);
							cprintf("allocate frame\n");
							map_frame(ptr_page_directory, frame_allocated, (unsigned int)first_page, PERM_WRITEABLE);
							cprintf("map frame\n");
							num_of_pages_allocated++;
							first_page->is_free = 0;
							first_page->size = PAGE_SIZE;
							LIST_INSERT_HEAD(&list_of_pages, first_page);


						}
						else if(i == number_of_pages - 1){
							struct BlockMetaData* before_last_page = (struct BlockMetaData*)((unsigned int)LIST_LAST(&list_of_pages) + (unsigned int)PAGE_SIZE);
							cprintf("assign address\n");
							struct FrameInfo* frame_allocated = NULL;
							cprintf("allocate frame\n");
							allocate_frame(&frame_allocated);
							map_frame(ptr_page_directory, frame_allocated, (unsigned int)before_last_page, PERM_WRITEABLE);
							cprintf("map frame\n");
							before_last_page->is_free = 0;
							before_last_page->size = PAGE_SIZE;
							cprintf("change values\n");
							LIST_INSERT_AFTER(&list_of_pages, LIST_LAST(&list_of_pages), before_last_page);
							num_of_pages_allocated++;
						}
						else{
							cprintf("else\n");
							struct BlockMetaData* normal_page = (struct BlockMetaData*)((unsigned int)LIST_LAST(&list_of_pages) + (unsigned int)PAGE_SIZE);
							struct FrameInfo* frame_allocated = NULL;
							allocate_frame(&frame_allocated);
							map_frame(ptr_page_directory, frame_allocated, (unsigned int)normal_page, PERM_WRITEABLE);
							normal_page->is_free = 0;
							normal_page->size = PAGE_SIZE;
							LIST_INSERT_TAIL(&list_of_pages, normal_page);

						}
					}
					cprintf("before return\n");
					return LIST_FIRST(&list_of_pages);
				}
				else{
					struct BlockMetaData* iterator = LIST_LAST(&list_of_pages);
					for(int i = 0; i < number_of_pages; i++){
						cprintf("loop\n");
						struct BlockMetaData* before_last_page = (struct BlockMetaData*)((unsigned int)LIST_LAST(&list_of_pages) + (unsigned int)PAGE_SIZE);
						cprintf("assign address\n");
						struct FrameInfo* frame_allocated = NULL;
						cprintf("allocate frame\n");
						allocate_frame(&frame_allocated);
						map_frame(ptr_page_directory, frame_allocated, (unsigned int)before_last_page, PERM_WRITEABLE);
						cprintf("map frame\n");
						before_last_page->is_free = 0;
						before_last_page->size = PAGE_SIZE;
						cprintf("change values\n");
						LIST_INSERT_AFTER(&list_of_pages, LIST_LAST(&list_of_pages), before_last_page);
						num_of_pages_allocated++;
					}
					cprintf("return in list not null\n");
					return iterator;
				}
			}
			else{
				cprintf("insufficient pages\n");
				return NULL;
			}

		}
	}
	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	return NULL;
}

void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kfree() is not implemented yet...!!");
}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	//change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	panic("kheap_physical_address() is not implemented yet...!!");

	//change this "return" according to your answer
	return 0;
}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}

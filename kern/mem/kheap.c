#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"


#define num_of_pages_in_allocator (((unsigned int)KERNEL_HEAP_MAX - (hard_limit + (unsigned int)PAGE_SIZE) ) / (unsigned int)PAGE_SIZE)
#define start_of_page_allocator (hard_limit + (unsigned int)PAGE_SIZE) + 16384 - 20480)
uint32 num_of_pages_allocated = 0;
bool is_initialized_page = 0;
uint32 page_boundary_tracking_decrement = 0;

extern uint32 sys_calculate_free_frames() ;

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
	frame_allocated->va = daStart;
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
	uint32 previous_break= segment_break;

		if (increment >0 && increment <= (segment_break - hard_limit)){
			if(increment > (segment_break - hard_limit)|| segment_break == hard_limit){
				panic("exceeded the limit");
			}
			page_boundary_tracking_decrement = 0;

			if (increment % PAGE_SIZE==0){
				segment_break += increment;
				for(uint32 i=previous_break ;i<segment_break;i+=PAGE_SIZE){
					struct FrameInfo* frame_allocated =NULL;
					allocate_frame(&frame_allocated);
					frame_allocated->va = i;
					if(frame_allocated==NULL){
						panic("No free frames");
					}
					map_frame(ptr_page_directory,frame_allocated,i,PERM_WRITEABLE);
				}
				return (void *)previous_break;
			}
			else{
				if(increment < PAGE_SIZE){
					if(previous_break % PAGE_SIZE != 0){
						segment_break = ROUNDDOWN(segment_break, PAGE_SIZE);
					}
				}
				if(increment > PAGE_SIZE){
					if(previous_break % PAGE_SIZE != 0){
						segment_break = ROUNDUP(segment_break, PAGE_SIZE);
					}
				}
//				page_boundary_tracking_increment += increment;
				int counter=0;
				for(int i=increment; i%PAGE_SIZE!=0; i++){
					counter++;
				}
				segment_break += (increment+counter);
				for(uint32 i=previous_break ;i<segment_break;i+=PAGE_SIZE){
					struct FrameInfo* frame_allocated =NULL;
					allocate_frame(&frame_allocated);
					frame_allocated->va = i;
					if(frame_allocated==NULL){
						panic("No free frames");
					}
					map_frame(ptr_page_directory,frame_allocated,i,PERM_WRITEABLE);
				}
				return (void *)previous_break;
			}
		}
		else if(increment ==0){
			return (void *)segment_break;
		}
		else{
			segment_break +=increment;
			if((increment * -1) < PAGE_SIZE){
				page_boundary_tracking_decrement += increment;
				if(((int)page_boundary_tracking_decrement * -1) > PAGE_SIZE){

					page_boundary_tracking_decrement += PAGE_SIZE;
					uint32 page_removed = segment_break + (page_boundary_tracking_decrement * -1);
					unmap_frame(ptr_page_directory,page_removed);
				}
			}

			else if((increment * -1) > PAGE_SIZE){
				page_boundary_tracking_decrement += increment;
				int noOfPages= (increment/PAGE_SIZE) * -1;
				uint32 *ptr_page_table = NULL;
				uint32 offset = page_boundary_tracking_decrement + (noOfPages * PAGE_SIZE);
				uint32 page_removed = segment_break + (offset * -1);
				for(int i=0;i<noOfPages;i++){
					unmap_frame(ptr_page_directory,page_removed);
					page_removed+=PAGE_SIZE;
				}
			}
			else{
				unmap_frame(ptr_page_directory,previous_break);
			}
			return (void*) segment_break;
		}






}
void* kmalloc(unsigned int size)
{
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	if(isKHeapPlacementStrategyFIRSTFIT() == 1){
		// case size greater than heap
		if(size >= ((uint32)KERNEL_HEAP_MAX) - ((uint32)KERNEL_HEAP_START + (uint32)DYN_ALLOC_MAX_SIZE + (uint32)PAGE_SIZE)){
			return NULL;
		}
		struct BlockMetaData* v_address;
		// case allocator ms1
		if(size <= (uint32)DYN_ALLOC_MAX_BLOCK_SIZE){
			v_address = alloc_block_FF(size % PAGE_SIZE);
			return v_address;
		}
		// case page allocator
		else{
			// get number of frames required
			uint32 number_of_pages;

			if(size % (uint32)PAGE_SIZE == 0){
				number_of_pages = size / (uint32)PAGE_SIZE;
			}
			else{
				number_of_pages = (size / (uint32)PAGE_SIZE) + 1;
			}
			if(size >= (unsigned int)KERNEL_HEAP_MAX - (hard_limit + (unsigned int)PAGE_SIZE) ){
				return NULL;
			}
			if(number_of_pages <= num_of_pages_in_allocator - num_of_pages_allocated){
				uint32 found_address = 0;
				for(int i = 0; i < num_of_pages_in_allocator; i++){
					uint32 start_address;
					start_address = hard_limit + (unsigned int)PAGE_SIZE;
					uint32 *ptr_page_table = NULL;
					struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory,(uint32)start_address + ((uint32)PAGE_SIZE * i),&ptr_page_table);
					uint32 num_of_free_pages = 0;
					if(ptr_frame_info == 0){
						//address free
						num_of_free_pages ++;
						int m = 1;
						while(1 == 1){
							ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)start_address + ((uint32)PAGE_SIZE * i) + ((uint32)PAGE_SIZE * m), &ptr_page_table);
							if(ptr_frame_info == 0){
								m++;
								num_of_free_pages ++;
								if(num_of_free_pages == number_of_pages){
									found_address = (uint32)start_address + ((uint32)PAGE_SIZE * i);
									break;
								}
							}
							else{
								break;
							}

						}
						if(num_of_free_pages < number_of_pages){
							i += m;
							continue;
						}
						else if(num_of_free_pages == number_of_pages){
							found_address = (uint32)start_address + ((uint32)PAGE_SIZE * i);
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
					// allocate and map
					for(int j = 0; j < number_of_pages; j++){
						struct FrameInfo* new_frame = NULL;
						int result = allocate_frame(&new_frame);
						if(result == 0){
							if(j == 0){
								new_frame->size = number_of_pages;
							}
							else{
								new_frame->size = 1;
							}
							new_frame->va = found_address + ((uint32)PAGE_SIZE * j);
							map_frame(ptr_page_directory, new_frame, found_address + ((uint32)PAGE_SIZE * j), PERM_PRESENT|PERM_WRITEABLE);
							num_of_pages_allocated++;
						}

					}
					return (void*)found_address;
				}
				else{
					return NULL;
				}



			}

		}
	}
	return NULL;
}


void kfree(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");
	//check v.add in block allocate || page allocate
	if((uint32)virtual_address < KERNEL_HEAP_START || (uint32)virtual_address >= KERNEL_HEAP_MAX){
		panic("invalid address\n");
	}
	else if((uint32)virtual_address >= KERNEL_HEAP_START && (uint32)virtual_address < KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE){
		free_block(virtual_address);
		return;
	}
	else if((uint32)virtual_address >= KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE && (uint32)virtual_address < KERNEL_HEAP_MAX){
		uint32 *ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
		if(ptr_frame_info == 0){
			panic("NULL address\n");
		}
		else{
			uint32 num_of_pages = ptr_frame_info->size;
			uint32 *ptr_page_table_1 = NULL;
			for(int i = 0; i < num_of_pages; i++){
				ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address + ((uint32)PAGE_SIZE * i), &ptr_page_table_1);
				ptr_frame_info->va = 0;
				unmap_frame(ptr_page_directory, (uint32)virtual_address + ((uint32)PAGE_SIZE * i));
			}
		}
	}

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================
		uint32 *ptr_page_table = NULL;

		uint32 framenum = physical_address >> 12;
		uint32 offset = (physical_address & 0xFFF);

		struct FrameInfo * ptr_frame_info;
		ptr_frame_info = to_frame_info(physical_address);
		if(ptr_frame_info != 0)
		{
			if(ptr_frame_info->va == 0){
				return ptr_frame_info->va;
			}
			uint32 virtual_address = ptr_frame_info->va | offset;
			return virtual_address;
		}
		else
			return 0;

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	//change this "return" according to your answer
	uint32 *ptr_page_table = NULL;
	uint32 offset = (virtual_address & 0xFFF);

	int ret = get_page_table(ptr_page_directory, virtual_address, &ptr_page_table);
	if (ret == TABLE_IN_MEMORY)
	{


		//cprintf("page table found at virtual address:%x\n", ptr_page_table);
		//get frame# of directory table
		uint32 directory_entry = ptr_page_directory[PDX(virtual_address)];

		//get number of page table
		int indexPageTable = directory_entry >> 12; // shift right by 12 bits
		//cprintf("f# of page table = %d\n", indexPageTable) ;

		uint32 table_entry = ptr_page_table [PTX(virtual_address)];
		int framenumber = table_entry >> 12;
		//cprintf("f# of the page = %d\n", framenumber) ;

		uint32 physical_address = (framenumber << 12) | offset;
		return physical_address;

	}
	else
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

	if(new_size == 0 && virtual_address != NULL){
		kfree(virtual_address);
		return NULL;
	}
	else if(new_size == 0 && virtual_address == NULL){
		return NULL;
	}
	else if(new_size != 0 && virtual_address == NULL){
		return kmalloc(new_size);
	}
	else{
		if((uint32)virtual_address >= KERNEL_HEAP_START && (uint32)virtual_address < KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE){
			return realloc_block_FF(virtual_address, new_size);

		}
		else if((uint32)virtual_address >= KERNEL_HEAP_START + DYN_ALLOC_MAX_SIZE + PAGE_SIZE && (uint32)virtual_address < KERNEL_HEAP_MAX){

		uint32 old_size;
		uint32 *ptr_page_table = NULL;
		struct FrameInfo *ptr_frame_info = get_frame_info(ptr_page_directory, (uint32)virtual_address, &ptr_page_table);
		if(ptr_frame_info == 0){
			panic("NULL address\n");
		}
		else{
			old_size = ptr_frame_info->size;
		}
		old_size = ptr_frame_info->size;
		uint32 new_num_of_pages;
		if(new_size % PAGE_SIZE == 0){
			new_num_of_pages = new_size / PAGE_SIZE;
		}
		else{
			new_num_of_pages = (new_size / PAGE_SIZE) + 1;
		}
		if(old_size < new_num_of_pages){
			uint32 requested_size = new_num_of_pages - old_size;
			uint32 end_of_pages = (uint32)virtual_address + ((uint32)PAGE_SIZE * old_size);
			ptr_frame_info = get_frame_info(ptr_page_directory, end_of_pages, &ptr_page_table);
			uint32 num_of_free_pages = 0;
			uint32 found_address = 0;
			if(ptr_frame_info == 0){
				num_of_free_pages++;
				int m = 1;
				while(1 == 1){
					ptr_frame_info = get_frame_info(ptr_page_directory, end_of_pages + ((uint32)PAGE_SIZE * m), &ptr_page_table);
					if(ptr_frame_info == 0){
						m++;
						num_of_free_pages++;
					}
					else{
						break;
					}
					if(num_of_free_pages == requested_size){
						found_address = end_of_pages;
						break;
					}
				}
				if(num_of_free_pages > requested_size){
					found_address = end_of_pages;
				}
				else if(num_of_free_pages == requested_size){
					found_address = end_of_pages;
				}
				else{
					void* new_address = kmalloc(new_size);
					if(new_address != NULL){
						for(int k = 0; k < old_size; k++){
							unmap_frame(ptr_page_directory, (uint32)virtual_address + ((uint32)PAGE_SIZE * k));
						}
						return new_address;
					}
					return virtual_address;
				}
			}
			else{
				void* new_address = kmalloc(new_size);
				if(new_address != NULL){
					for(int k = 0; k < old_size; k++){
						unmap_frame(ptr_page_directory, (uint32)virtual_address + ((uint32)PAGE_SIZE * k));
					}
					return new_address;
				}
				return virtual_address;
			}
			if(found_address != 0){
				for(int k = 0; k < requested_size; k++){
					struct FrameInfo* new_frame = NULL;
					allocate_frame(&new_frame);
					map_frame(ptr_page_directory, new_frame, found_address + ((uint32)PAGE_SIZE * k), PERM_PRESENT|PERM_WRITEABLE);
				}
			}

		}
		else if(old_size == new_num_of_pages){
			return virtual_address;
		}
		else{
			uint32 remaining_size = old_size - new_num_of_pages;
			for(int k = old_size; k > new_num_of_pages; k--){
				unmap_frame(ptr_page_directory, (uint32)virtual_address + ((uint32)PAGE_SIZE  * (k - 1)));
			}
			return virtual_address;
		}
	}


	return NULL;
	//panic("krealloc() is not implemented yet...!!");
	}
}

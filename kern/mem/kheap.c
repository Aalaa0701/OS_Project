#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"


#define num_of_pages_in_allocator (((unsigned int)KERNEL_HEAP_MAX - (hard_limit + (unsigned int)PAGE_SIZE) ) / (unsigned int)PAGE_SIZE)
#define start_of_page_allocator (hard_limit + (unsigned int)PAGE_SIZE) + 16384 - 20480)
uint32 num_of_pages_allocated = 0;
bool is_initialized_page = 0;


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
	uint32 previous_break = segment_break;
		if (increment > 0 && increment <= (segment_break - hard_limit)){
			if (increment % PAGE_SIZE == 0){
				segment_break += increment;
				for(uint32 i=previous_break ;i<segment_break;i+=PAGE_SIZE){
					struct FrameInfo* frame_allocated =NULL;
					allocate_frame(&frame_allocated);
					if(frame_allocated==NULL){
						panic("No free frames");
					}
					map_frame(ptr_page_directory,frame_allocated,i,PERM_WRITEABLE);
				}
				return (void *)previous_break;
			}
			else{
				int counter=0;
				for(int i=increment; i%PAGE_SIZE !=0; i++){
					counter++;
				}
				segment_break += (increment+counter);
				for(uint32 i=previous_break ;i<segment_break;i+=PAGE_SIZE){
					struct FrameInfo* frame_allocated =NULL;
					allocate_frame(&frame_allocated);
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
			if (increment % PAGE_SIZE==0){
				segment_break += increment;
				for(uint32 i=segment_break;i<previous_break;i+=PAGE_SIZE){
					unmap_frame(ptr_page_directory,i);
				}
				return (void *)segment_break;
			}
			else{
				int counter=0;
				for(int i=increment; i%PAGE_SIZE !=0; i--){
					counter--;
				}
				segment_break += (increment+counter);
				for(uint32 i=segment_break;i<previous_break;i+=PAGE_SIZE){
					unmap_frame(ptr_page_directory,i);
				}
				return (void *)segment_break;
			}
		}
		if(increment > (segment_break - hard_limit)|| segment_break == KERNEL_HEAP_MAX){
			panic("exceeded the limit");
		}
	//return (void*)-1;

}



void initialize_page_allocator(){
	uint32 start_address = hard_limit + (unsigned int)PAGE_SIZE ;
	struct BlockMetaData* page_allocator = (struct BlockMetaData*) start_address;
	struct FrameInfo* frame = NULL;
	allocate_frame(&frame);
	map_frame(ptr_page_directory, frame, start_address, PERM_USER|PERM_WRITEABLE);
	page_allocator->is_free = 1;
	page_allocator->size = num_of_pages_in_allocator;
	LIST_INIT(&list_of_pages);
	LIST_INSERT_HEAD(&list_of_pages, page_allocator);

}
void* alloc_page_ff(unsigned int size){
//	if(LIST_SIZE(&list_of_pages) == 2 && LIST_LAST(&list_of_pages)->is_free == 1 && LIST_FIRST(&list_of_pages)->is_free == 0){
//		cprintf("Split\n");
//		// splitting
//		uint32 old_size = LIST_LAST(&list_of_pages)->size;
//		struct BlockMetaData* new_block = (struct BlockMetaData*) ((unsigned int)LIST_LAST(&list_of_pages) + ((unsigned int)PAGE_SIZE * size));
//		cprintf("assign address\n");
//		cprintf("free frames before: %d\n", sys_calculate_free_frames());
//		struct FrameInfo* frame_allocated = NULL;
//		allocate_frame(&frame_allocated);
//		struct FrameInfo* frame_allocated2 = NULL;
//		allocate_frame(&frame_allocated2);
//		map_frame(ptr_page_directory, frame_allocated, (uint32)LIST_LAST(&list_of_pages), PERM_WRITEABLE);
//		map_frame(ptr_page_directory, frame_allocated2, (uint32)new_block, PERM_WRITEABLE);
//		cprintf("free frames after: %d\n", sys_calculate_free_frames());
//		cprintf("allocate block\n");
//		for(int i = 1; i < size; i++){
//			cprintf("in loop\n");
//			cprintf("free frames before: %d\n", sys_calculate_free_frames());
//			struct FrameInfo* new_frame = NULL;
//			allocate_frame(&new_frame);
//			cprintf("alloc frame\n");
//			map_frame(ptr_page_directory, new_frame, (uint32)((unsigned int)LIST_LAST(&list_of_pages) + ((unsigned int)PAGE_SIZE * i)), PERM_WRITEABLE);
//			cprintf("free frames after: %d\n", sys_calculate_free_frames());
//			cprintf("mapped frame\n");
//		}
//		cprintf("exit loop\n");
//		LIST_LAST(&list_of_pages)->is_free = 0;
//		LIST_LAST(&list_of_pages)->size = size;
//		new_block->is_free = 1;
//		new_block->size = old_size - size;
//		LIST_INSERT_AFTER(&list_of_pages, LIST_LAST(&list_of_pages), new_block);
//		cprintf("free size > \n");
//		return new_block->prev_next_info.le_prev;
//
//	}



	struct BlockMetaData* iterator;
	iterator = LIST_FIRST(&list_of_pages);
	cprintf("size of list: %d\n", LIST_SIZE(&list_of_pages));
	for(int i = 0; i < LIST_SIZE(&list_of_pages); i++){
		cprintf("in loop\n");
		if(iterator->is_free == 1 && iterator->size > size){
			for(int i = 0; i < size; i++){
				struct FrameInfo* frame_allocated = NULL;
				allocate_frame(&frame_allocated);
				map_frame(ptr_page_directory, frame_allocated, (unsigned int)((unsigned int)iterator) + PAGE_SIZE * i, PERM_USER|PERM_WRITEABLE);
			}
			uint32 old_size = iterator->size;
			iterator->is_free = 0;
			iterator->size = size;
			struct BlockMetaData* new_block = (struct BlockMetaData*) ((unsigned int)iterator + ((unsigned int)PAGE_SIZE * size));
			struct FrameInfo* frame = NULL;
			allocate_frame(&frame);
			map_frame(ptr_page_directory, frame, (unsigned int)new_block, PERM_USER|PERM_WRITEABLE);
			new_block->is_free = 1;
			new_block->size = old_size - size;
			LIST_INSERT_AFTER(&list_of_pages, iterator, new_block);
			return iterator;

		}
		else if(iterator->is_free == 1 && iterator->size == size){
			for(int i = 0; i < size; i++){
				struct FrameInfo* frame_allocated = NULL;
				allocate_frame(&frame_allocated);
				map_frame(ptr_page_directory, frame_allocated, (unsigned int)((unsigned int)iterator) + PAGE_SIZE * i, PERM_USER|PERM_WRITEABLE);
			}
			iterator->is_free = 0;
			iterator->size = size;
			return iterator;
		}
		else{
			cprintf("not free\n");
			if(iterator != LIST_LAST(&list_of_pages)){
				cprintf("not last\n");
				iterator = iterator->prev_next_info.le_next;
				struct FrameInfo* frame = NULL;
				allocate_frame(&frame);
				cprintf("before map frame\n");
				map_frame(ptr_page_directory, frame, (unsigned int)iterator, PERM_PRESENT);
				cprintf("mapped frame\n");
			}
			continue;
		}
	}

//	LIST_FOREACH(iterator, &list_of_pages){
//		if(iterator->is_free == 1 && iterator->size > size){
//			for(int i = 0; i < size; i++){
//				struct FrameInfo* frame_allocated = NULL;
//				allocate_frame(&frame_allocated);
//				map_frame(ptr_page_directory, frame_allocated, (unsigned int)((unsigned int)iterator) + PAGE_SIZE * i, PERM_USER|PERM_WRITEABLE);
//			}
//			uint32 old_size = iterator->size;
//			iterator->is_free = 0;
//			iterator->size = size;
//			struct BlockMetaData* new_block = (struct BlockMetaData*) ((unsigned int)iterator + ((unsigned int)PAGE_SIZE * size));
//			struct FrameInfo* frame = NULL;
//			allocate_frame(&frame);
//			map_frame(ptr_page_directory, frame, (unsigned int)new_block, PERM_USER|PERM_WRITEABLE);
//			new_block->is_free = 1;
//			new_block->size = old_size - size;
//			return iterator;
//
//		}
//		else if(iterator->is_free == 1 && iterator->size == size){
//			for(int i = 0; i < size; i++){
//				struct FrameInfo* frame_allocated = NULL;
//				allocate_frame(&frame_allocated);
//				map_frame(ptr_page_directory, frame_allocated, (unsigned int)((unsigned int)iterator) + PAGE_SIZE * i, PERM_USER|PERM_WRITEABLE);
//			}
//			iterator->is_free = 0;
//			iterator->size = size;
//			return iterator;
//		}
//		else{
//			continue;
//		}
//	}


//	struct BlockMetaData* iterator;
//	cprintf("list size: %d\n", LIST_SIZE(&list_of_pages));
//	iterator = LIST_FIRST(&list_of_pages);
//	for(int i = 0; i < LIST_SIZE(&list_of_pages); i++){
//		cprintf("in loop\n");
//		if(iterator->is_free == 1 && iterator->size > size){
//			cprintf("free size greater\n");
//			uint32 old_size = iterator->size;
//			struct BlockMetaData* new_block = (struct BlockMetaData*) ((unsigned int)iterator + ((unsigned int)PAGE_SIZE * size));
//			struct FrameInfo* frame_allocated_1 = NULL;
//			struct FrameInfo* frame_allocated_2 = NULL;
//			allocate_frame(&frame_allocated_1);
//			allocate_frame(&frame_allocated_2);
//			map_frame(ptr_page_directory,frame_allocated_1,(uint32)iterator,PERM_USER|PERM_WRITEABLE);
//			map_frame(ptr_page_directory,frame_allocated_2,(uint32)new_block,PERM_USER|PERM_WRITEABLE);
//			cprintf("before smaller loop\n");
//			cprintf("size: %d\n", size);
//			for(int j = 1; j < size; j++){
//				cprintf("in small loop\n");
//				struct FrameInfo* new_frame = NULL;
//				allocate_frame(&new_frame);
//				map_frame(ptr_page_directory,new_frame,(uint32)((unsigned int)iterator + ((unsigned int)PAGE_SIZE * j)) ,PERM_USER|PERM_WRITEABLE);
//			}
//			iterator->is_free = 0;
//			iterator->size = size;
//			new_block->is_free = 1;
//			new_block->size = old_size - size;
//			LIST_INSERT_AFTER(&list_of_pages, iterator, new_block);
//			cprintf("after insert\n");
//			return iterator;
//		}
//		else if(iterator->is_free == 1 && iterator->size == size){
//			cprintf("free size equal\n");
//			iterator->is_free = 0;
//			iterator->size = size;
//			struct FrameInfo* frame_case_equal = NULL;
//			allocate_frame(&frame_case_equal);
//			map_frame(ptr_page_directory,frame_case_equal,(uint32)iterator,PERM_PRESENT);
//			return iterator;
//		}
//		else{
//			if(iterator == LIST_LAST(&list_of_pages)){
//				cprintf("iterator is last\n");
//				return NULL;
//			}
//			cprintf("not free\n");
//			iterator = iterator->prev_next_info.le_next;
//			if(iterator == LIST_LAST(&list_of_pages)){
//				cprintf("next is last\n");
//			}
//			cprintf("after updating iterator\n");
//			struct FrameInfo* frame = NULL;
//			cprintf("frame is null\n");
//			allocate_frame(&frame);
//			cprintf("allocate frame\n");
//			map_frame(ptr_page_directory, frame, (uint32)iterator, PERM_PRESENT);
//			//map_frame(ptr_page_directory,frame,(uint32)iterator,PERM_WRITEABLE);
//			cprintf("map frame for not free\n");
//			continue;
//		}
//	}

	cprintf("not found\n");
	return NULL;
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
			v_address = alloc_block_FF(size);
			return v_address;
		}
		// case page allocator
		else{
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
				uint32 temp1 = number_of_pages;
				uint32 temp2 = num_of_pages_in_allocator;
				if(LIST_FIRST(&list_of_pages) == NULL){
					for(int i = 0; i < number_of_pages; i++){
						if(i == 0){
							struct BlockMetaData* first_page = (struct BlockMetaData*)(hard_limit + (unsigned int)PAGE_SIZE );
						//	cprintf("free frames before: %d\n", sys_calculate_free_frames());
							struct FrameInfo* frame_allocated = NULL;
							allocate_frame(&frame_allocated);
							map_frame(ptr_page_directory, frame_allocated, (unsigned int)first_page, PERM_PRESENT|PERM_WRITEABLE);
							//cprintf("free frames after: %d\n", sys_calculate_free_frames());
							num_of_pages_allocated++;
							first_page->is_free = 0;
							first_page->size = number_of_pages;
							LIST_INSERT_HEAD(&list_of_pages, first_page);
						}
						else{
							struct BlockMetaData* normal_page = (struct BlockMetaData*)((unsigned int)LIST_LAST(&list_of_pages) + (unsigned int)PAGE_SIZE);
							//cprintf("free frames before: %d\n", sys_calculate_free_frames());
							struct FrameInfo* frame_allocated = NULL;
							allocate_frame(&frame_allocated);
							map_frame(ptr_page_directory, frame_allocated, (unsigned int)normal_page, PERM_PRESENT|PERM_WRITEABLE);
							//cprintf("free frames after: %d\n", sys_calculate_free_frames());
							normal_page->is_free = 0;
							normal_page->size = PAGE_SIZE;
							LIST_INSERT_TAIL(&list_of_pages, normal_page);
							num_of_pages_allocated++;
						}
					}
					return LIST_FIRST(&list_of_pages);
				}
				else{
					struct BlockMetaData* iterator = (struct BlockMetaData*)((unsigned int)LIST_LAST(&list_of_pages) + ((unsigned int)PAGE_SIZE));
					for(unsigned int i = 0; i < number_of_pages; i++){
						struct BlockMetaData* before_last_page = (struct BlockMetaData*)((unsigned int)iterator + ((unsigned int)PAGE_SIZE * i));
						//cprintf("free frames before: %d\n", sys_calculate_free_frames());
						struct FrameInfo* new_frame = NULL;
						//cprintf("%d frame allocated\n",i+1);
						//cprintf("%d address2: %x\n", i + 1, before_last_page);
						allocate_frame(&new_frame);
						map_frame(ptr_page_directory, new_frame, (unsigned int)before_last_page, PERM_PRESENT|PERM_WRITEABLE);
						//cprintf("free frames after: %d\n", sys_calculate_free_frames());
						before_last_page->is_free = 0;
						if(i == 0){
							before_last_page->size = number_of_pages;
						}
						else{
							before_last_page->size = PAGE_SIZE;
						}

						if(before_last_page == NULL){
							cprintf("second null: %x\n", (unsigned int)LIST_LAST(&list_of_pages));
							return NULL;
						}
						LIST_INSERT_AFTER(&list_of_pages, LIST_LAST(&list_of_pages), before_last_page);
						num_of_pages_allocated++;
					}
					//cprintf("allocated pages: %d\n", num_of_pages_allocated);
					return iterator;
				}
			}
			else{
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
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	//change this "return" according to your answer
	return 0;
}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

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

/*
 * chunk_operations.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include <kern/trap/fault_handler.h>
#include <kern/disk/pagefile_manager.h>
#include "kheap.h"
#include "memory_manager.h"
#include <inc/queue.h>
#include <kern/tests/utilities.h>

//extern void inctst();
extern uint32 sys_calculate_free_frames() ;

/******************************/
/*[1] RAM CHUNKS MANIPULATION */
/******************************/

//===============================
// 1) CUT-PASTE PAGES IN RAM:
//===============================
//This function should cut-paste the given number of pages from source_va to dest_va on the given page_directory
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, cut-paste the number of pages and return 0
//	ALL 12 permission bits of the destination should be TYPICAL to those of the source
//	The given addresses may be not aligned on 4 KB
int cut_paste_pages(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 num_of_pages)
{
	panic("cut_paste_pages() is not implemented yet...!!");

	return 0 ;
}

//===============================
// 2) COPY-PASTE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	If ANY of the destination pages exists with READ ONLY permission, deny the entire process and return -1.
//	If the page table at any destination page in the range is not exist, it should create it
//	If ANY of the destination pages doesn't exist, create it with the following permissions then copy.
//	Otherwise, just copy!
//		1. WRITABLE permission
//		2. USER/SUPERVISOR permission must be SAME as the one of the source
//	The given range(s) may be not aligned on 4 KB
int copy_paste_chunk(uint32* page_directory, uint32 source_va, uint32 dest_va, uint32 size)
{
	panic("copy_paste_chunk() is not implemented yet...!!");
	return 0;
}

//===============================
// 3) SHARE RANGE IN RAM:
//===============================
//This function should copy-paste the given size from source_va to dest_va on the given page_directory
//	Ranges DO NOT overlapped.
//	It should set the permissions of the second range by the given perms
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, share the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	The given range(s) may be not aligned on 4 KB
int share_chunk(uint32* page_directory, uint32 source_va,uint32 dest_va, uint32 size, uint32 perms)
{
	panic("share_chunk() is not implemented yet...!!");
	return 0;
}

//===============================
// 4) ALLOCATE CHUNK IN RAM:
//===============================
//This function should allocate the given virtual range [<va>, <va> + <size>) in the given address space  <page_directory> with the given permissions <perms>.
//	If ANY of the destination pages exists, deny the entire process and return -1. Otherwise, allocate the required range and return 0
//	If the page table at any destination page in the range is not exist, it should create it
//	Allocation should be aligned on page boundary. However, the given range may be not aligned.
int allocate_chunk(uint32* page_directory, uint32 va, uint32 size, uint32 perms)
{
	panic("allocate_chunk() is not implemented yet...!!");
	return 0;
}

//=====================================
// 5) CALCULATE ALLOCATED SPACE IN RAM:
//=====================================
void calculate_allocated_space(uint32* page_directory, uint32 sva, uint32 eva, uint32 *num_tables, uint32 *num_pages)
{
	panic("calculate_allocated_space() is not implemented yet...!!");
}


//=====================================
// 6) CALCULATE REQUIRED FRAMES IN RAM:
//=====================================
//This function should calculate the required number of pages for allocating and mapping the given range [start va, start va + size) (either for the pages themselves or for the page tables required for mapping)
//	Pages and/or page tables that are already exist in the range SHOULD NOT be counted.
//	The given range(s) may be not aligned on 4 KB
uint32 calculate_required_frames(uint32* page_directory, uint32 sva, uint32 size)
{
	panic("calculate_required_frames() is not implemented yet...!!");
	return 0;
}

//=================================================================================//
//===========================END RAM CHUNKS MANIPULATION ==========================//
//=================================================================================//

/*******************************/
/*[2] USER CHUNKS MANIPULATION */
/*******************************/

//======================================================
/// functions used for USER HEAP (malloc, free, ...)
//======================================================

//=====================================
// 1) ALLOCATE USER MEMORY:
//=====================================
void allocate_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	/*=============================================================================*/
	//TODO: [PROJECT'23.MS2 - #10] [2] USER HEAP - allocate_user_mem() [Kernel Side]
	/*REMOVE THESE LINES BEFORE START CODING */
	/*=============================================================================*/
	uint32* page_table_ptr = NULL;
	for(int i = 0; i < size; i++){
		int result = get_page_table(e->env_page_directory, virtual_address + (i * (unsigned int)PAGE_SIZE), &page_table_ptr);
		if(result == TABLE_NOT_EXIST){
			page_table_ptr = create_page_table(e->env_page_directory, virtual_address + (i * (unsigned int)PAGE_SIZE));
		}
		pt_set_page_permissions(e->env_page_directory, virtual_address + (i * (unsigned int)PAGE_SIZE), PERM_AVAILABLE|PERM_USER|PERM_WRITEABLE, PERM_PRESENT);
	}

//	for(int i = 0; i < size; i++){
//		pt_set_page_permissions(e->env_page_directory, virtual_address + (i * (unsigned int)PAGE_SIZE), PERM_AVAILABLE|PERM_USER|PERM_WRITEABLE, PERM_PRESENT);
//	}
	// Write your code here, remove the panic and write your code


}

//=====================================
// 2) FREE USER MEMORY:
//=====================================
void free_user_mem(struct Env* e, uint32 virtual_address, uint32 size)
{
	/*==========================================================================*/
	//TODO: [PROJECT'23.MS2 - #12] [2] USER HEAP - free_user_mem() [Kernel Side]
	/*REMOVE THESE LINES BEFORE START CODING */
	//inctst();
	//return;
	/*==========================================================================*/

	// Write your code here, remove the panic and write your code
	for(int i = 0; i < size; i++){
		//unmark
		env_page_ws_invalidate(e, virtual_address + (i * (unsigned int)PAGE_SIZE));
		pf_remove_env_page(e, virtual_address + (i * (unsigned int)PAGE_SIZE));
		pt_set_page_permissions(e->env_page_directory, virtual_address + (i * (unsigned int)PAGE_SIZE), 0, PERM_PRESENT|PERM_AVAILABLE|PERM_USER|PERM_WRITEABLE|PERM_USED|PERM_MODIFIED|PERM_BUFFERED);
		unmap_frame(e->env_page_directory, virtual_address + (i * (unsigned int)PAGE_SIZE));
		//remove from memory

	}
	if(e->page_last_WS_element == NULL){
		return;
	}
	int num_of_elements_before_last = 0;
	int list_size = LIST_SIZE(&(e->page_WS_list));
	struct WorkingSetElement* iterator = LIST_FIRST(&(e->page_WS_list));

	for(int i = 0; i < list_size; i++){
		if(iterator->virtual_address == e->page_last_WS_element->virtual_address){
			break;
		}
		else{
			num_of_elements_before_last++;
			iterator = iterator->prev_next_info.le_next;
		}
	}
	for(int i = 0; i < num_of_elements_before_last; i++){
		struct WorkingSetElement* current_iterator = LIST_FIRST(&(e->page_WS_list));
		LIST_REMOVE(&(e->page_WS_list), current_iterator);
		LIST_INSERT_TAIL(&(e->page_WS_list), current_iterator);
	}
	//TODO: [PROJECT'23.MS2 - BONUS#2] [2] USER HEAP - free_user_mem() IN O(1): removing page from WS List instead of searching the entire list

}

//=====================================
// 2) FREE USER MEMORY (BUFFERING):
//=====================================
void __free_user_mem_with_buffering(struct Env* e, uint32 virtual_address, uint32 size)
{
	// your code is here, remove the panic and write your code
	panic("__free_user_mem_with_buffering() is not implemented yet...!!");
}

//=====================================
// 3) MOVE USER MEMORY:
//=====================================
void move_user_mem(struct Env* e, uint32 src_virtual_address, uint32 dst_virtual_address, uint32 size)
{
	//your code is here, remove the panic and write your code
	panic("move_user_mem() is not implemented yet...!!");

	// This function should move all pages from "src_virtual_address" to "dst_virtual_address"
	// with the given size
	// After finished, the src_virtual_address must no longer be accessed/exist in either page file
	// or main memory
}

//=================================================================================//
//========================== END USER CHUNKS MANIPULATION =========================//
//=================================================================================//

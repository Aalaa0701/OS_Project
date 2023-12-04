/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"

extern int sys_calculate_free_frames();
uint32 num_of_replacements = 0;
//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
		struct WorkingSetElement *victimWSElement = NULL;
		uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
		int iWS =curenv->page_last_WS_index;
		uint32 wsSize = env_page_ws_get_size(curenv);
#endif
cprintf("in page fault handler\n");
	if(isPageReplacmentAlgorithmFIFO())
	{
		cprintf("FIFO\n");

		//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
		// Write your code here, remove the panic and write your code
		uint32 *ptr_page_table = NULL;
		uint32 new_va = ROUNDDOWN(fault_va, PAGE_SIZE);
		cprintf("fault address: %x\n",new_va);
		unsigned int permissions = pt_get_page_permissions(curenv->env_page_directory, fault_va);

		cprintf("current size: %d\n", wsSize);
		cprintf("max size: %d\n", curenv->page_WS_max_size);
		if(wsSize < (curenv->page_WS_max_size)){
			cprintf("Placement\n");
			int readc = pf_read_env_page(curenv,(void*)new_va);
			if (readc == E_PAGE_NOT_EXIST_IN_PF){
			  if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
				  if(permissions == 0 || permissions == -1){
					  sched_kill_env(curenv->env_id);
				  }
				struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
				if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
				else{
					curenv->page_last_WS_element = NULL;
				}
				struct FrameInfo* ptr_frame_info = NULL;
				int allocnewframe = allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);

			  }
			  else if (new_va >= (uint32)USTACKBOTTOM && new_va <= (uint32)USTACKTOP){
				struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
				if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
				else{
					curenv->page_last_WS_element = NULL;
				}
				struct FrameInfo* ptr_frame_info = NULL;
				int allocnewframe = allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
			  }
			  else {
				sched_kill_env(curenv->env_id);
			  }
			}
			else{
				struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
				if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
				else{
					curenv->page_last_WS_element = NULL;
				}
				struct FrameInfo* ptr_frame_info = NULL;
				int allocnewframe = allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
			}
		}
		//replacement
		else{
			num_of_replacements++;
			cprintf("replacement num: %d\n", num_of_replacements);

//			uint32 replaced_va = curenv->page_last_WS_element->virtual_address;
//			curenv->page_last_WS_element->virtual_address = new_va;
//			curenv->page_last_WS_element = curenv->page_last_WS_element->prev_next_info.le_next;
//			unmap_frame(curenv->env_page_directory, replaced_va);
//			struct FrameInfo* ptr_frame_info = NULL;
//			int new_frame = allocate_frame(&ptr_frame_info);
//			map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
//			cprintf("free frames: %d\n", sys_calculate_free_frames());



//			uint32 replaced_va = curenv->page_last_WS_element->virtual_address;
//			uint32 replaced_index = curenv->page_last_WS_index;
//			env_page_ws_invalidate(curenv, replaced_va);
//			pt_set_page_permissions(curenv->env_page_directory, replaced_va, 0, PERM_PRESENT|PERM_AVAILABLE|PERM_USER|PERM_WRITEABLE|PERM_USED|PERM_MODIFIED|PERM_BUFFERED);
//			unmap_frame(curenv->env_page_directory, replaced_va);
//			struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
//			struct WorkingSetElement* prev_element = curenv->page_last_WS_element->prev_next_info.le_prev;
//			if(prev_element == NULL){
//				LIST_INSERT_HEAD(&(curenv->page_WS_list), new_element);
//			}
//			else{
//				LIST_INSERT_AFTER(&(curenv->page_WS_list), prev_element, new_element);
//			}
//			struct FrameInfo* ptr_frame_info = NULL;
//			int new_frame = allocate_frame(&ptr_frame_info);
//			map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
//


			struct WorkingSetElement* element_replaced = curenv->page_last_WS_element;
			curenv->page_last_WS_element = LIST_NEXT(element_replaced);
			struct WorkingSetElement* next_element = curenv->page_last_WS_element->prev_next_info.le_next;
			uint32 replaced_va = element_replaced->virtual_address;
			struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
			LIST_INSERT_AFTER(&(curenv->page_WS_list), element_replaced, new_element);
			env_page_ws_invalidate(curenv, element_replaced->virtual_address);
			pt_set_page_permissions(curenv->env_page_directory, replaced_va, 0, PERM_PRESENT|PERM_AVAILABLE|PERM_USER|PERM_WRITEABLE|PERM_USED|PERM_MODIFIED|PERM_BUFFERED);
			unmap_frame(curenv->env_page_directory, replaced_va);
			struct FrameInfo* ptr_frame_info = NULL;
			int new_frame = allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
			cprintf("out of replacement\n");
			cprintf("free frames: %d\n", sys_calculate_free_frames());
		}
	}
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX)){
		cprintf("in LRU\n");
		uint32 max_size = curenv->ActiveListSize + curenv->SecondListSize;
		uint32 active_list_size_max = curenv->ActiveListSize;
		uint32 second_list_size_max = curenv->SecondListSize;
		uint32 current_active_list_size = LIST_SIZE(&(curenv->ActiveList));
		uint32 current_second_list_size = LIST_SIZE(&(curenv->SecondList));
		uint32 *ptr_page_table = NULL;
		uint32 new_va = ROUNDDOWN(fault_va, PAGE_SIZE);
		unsigned int permissions = pt_get_page_permissions(curenv->env_page_directory, fault_va);
//		int readc = pf_read_env_page(curenv,(void*)new_va);
//		if (readc == E_PAGE_NOT_EXIST_IN_PF){
//			if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
//				if(permissions == 0 || permissions == -1){
//					sched_kill_env(curenv->env_id);
//				}
//			}
//			else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){
//
//			}
//			else{
//				sched_kill_env(curenv->env_id);
//			}
//		}
		//placement
		if(current_active_list_size + current_second_list_size < max_size){
			cprintf("placement\n");
			//case list1 not full
			if(current_active_list_size < active_list_size_max){
				struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
				LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
				struct FrameInfo* ptr_frame_info = NULL;
				int allocnewframe = allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
			}
			//case list1 full
			else{

				if(current_second_list_size == 0){
					struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
					struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
					pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
					LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
					LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
					LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
					struct FrameInfo* ptr_frame_info = NULL;
					int allocnewframe = allocate_frame(&ptr_frame_info);
					map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
				}
				else{
					struct WorkingSetElement* iterator = LIST_FIRST(&(curenv->SecondList));
					uint8 found = 0;
					for(int i = 0; i < current_second_list_size; i++){
						if(new_va == iterator->virtual_address){
							found = 1;
							break;
						}
						else{
							iterator = iterator->prev_next_info.le_next;
							continue;
						}
					}
					if(found == 0){
						struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
						struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
						pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
						LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
						LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
						LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
						struct FrameInfo* ptr_frame_info = NULL;
						int allocnewframe = allocate_frame(&ptr_frame_info);
						map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
					}
					else{
						cprintf("found in second list\n");
						struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
						LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
						LIST_REMOVE(&(curenv->SecondList), iterator);
						pt_set_page_permissions(curenv->env_page_directory, iterator->virtual_address, PERM_PRESENT, 0);
						LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
						LIST_INSERT_HEAD(&(curenv->ActiveList), iterator);
					}
				}

			}
		}
		//replacement
		else{
			//look in second list
			uint8 found = 0;
			struct WorkingSetElement* iterator = LIST_FIRST(&(curenv->SecondList));
			for(int i = 0; i < second_list_size_max; i++){
				if(new_va == iterator->virtual_address){
					found = 1;
					break;
				}
				else{
					iterator = iterator->prev_next_info.le_next;
					continue;
				}
			}
			if(found == 0){
				struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
				struct WorkingSetElement* victim_element = LIST_LAST(&(curenv->SecondList));
				int check_for_modified = pt_get_page_permissions(curenv->env_page_directory, victim_element->virtual_address);
				if((check_for_modified & PERM_MODIFIED) == PERM_MODIFIED){
					struct FrameInfo* modified_frame = get_frame_info(curenv->env_page_directory, victim_element->virtual_address, &ptr_page_table);
					pf_update_env_page(curenv, victim_element->virtual_address, modified_frame);
				}
				struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
				LIST_REMOVE(&(curenv->SecondList), victim_element);
				LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
				LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
				LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
				struct FrameInfo* ptr_frame_info = NULL;
				int allocnewframe = allocate_frame(&ptr_frame_info);
				map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
				unmap_frame(curenv->env_page_directory, victim_element->virtual_address);
			}
			else{
				pt_set_page_permissions(curenv->env_page_directory, new_va, PERM_PRESENT, 0);
				struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
				struct WorkingSetElement* MRU_element = iterator;
				LIST_REMOVE(&(curenv->SecondList), MRU_element);
				LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
				LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
				LIST_INSERT_HEAD(&(curenv->ActiveList), MRU_element);
			}
		}
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}




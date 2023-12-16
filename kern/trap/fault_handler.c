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
#include "../mem/kheap.h"

extern int sys_calculate_free_frames();
extern int sys_pf_calculate_allocated_pages();
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
	if(isPageReplacmentAlgorithmFIFO())
	{
		cprintf("in FIFO \n");
		//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
		// Write your code here, remove the panic and write your code
		uint32 *ptr_page_table = NULL;
		uint32 new_va = ROUNDDOWN(fault_va, PAGE_SIZE);
		unsigned int permissions = pt_get_page_permissions(curenv->env_page_directory, fault_va);
		if(wsSize < (curenv->page_WS_max_size)){
			struct FrameInfo* ptr_frame_info = NULL;
			int allocnewframe = allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
			int readc = pf_read_env_page(curenv,(void*)new_va);
			if (readc == E_PAGE_NOT_EXIST_IN_PF){
			  if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
				  if(permissions == 0 || permissions == -1){
					  sched_kill_env(curenv->env_id);
				  }
				struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
				if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
					if(curenv->page_last_WS_element == NULL)
						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}

			  }
			  else if (new_va >= (uint32)USTACKBOTTOM && new_va <= (uint32)USTACKTOP){
				struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
				if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
			  }
			  else {
				  cprintf("new va: %x\n",new_va);
				  cprintf("not in stack or heap\n");
				sched_kill_env(curenv->env_id);
			  }
			}
			else{
				struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
				LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
				if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
					if(curenv->page_last_WS_element == NULL)
						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
			}
		}
		//replacement
		else{
			struct FrameInfo* ptr_frame_info = NULL;
			int allocnewframe = allocate_frame(&ptr_frame_info);
			map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
			int readc = pf_read_env_page(curenv,(void*)new_va);
			if (readc == E_PAGE_NOT_EXIST_IN_PF){
				if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
					if(permissions == 0 || permissions == -1){
						sched_kill_env(curenv->env_id);
					}

					uint32 replaced_va = curenv->page_last_WS_element->virtual_address;
					int victim_permissions = pt_get_page_permissions(curenv->env_page_directory, replaced_va);
					struct FrameInfo* victim_frame = get_frame_info(curenv->env_page_directory, replaced_va, &ptr_page_table);
					struct WorkingSetElement* next_pointer = curenv->page_last_WS_element->prev_next_info.le_next;
					curenv->page_last_WS_element->virtual_address = new_va;
					if(next_pointer == NULL){
						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
					}
					else
						curenv->page_last_WS_element = next_pointer;
					if((victim_permissions & PERM_MODIFIED) == PERM_MODIFIED){
						pf_update_env_page(curenv, replaced_va, victim_frame);
					}
					unmap_frame(curenv->env_page_directory, replaced_va);

				}
				else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){

					uint32 replaced_va = curenv->page_last_WS_element->virtual_address;

					int victim_permissions = pt_get_page_permissions(curenv->env_page_directory, replaced_va);
					struct FrameInfo* victim_frame = get_frame_info(curenv->env_page_directory, replaced_va, &ptr_page_table);
					curenv->page_last_WS_element->virtual_address = new_va;
					struct WorkingSetElement* next_pointer = curenv->page_last_WS_element->prev_next_info.le_next;
					if(next_pointer == NULL){
						curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
					}
					else
						curenv->page_last_WS_element = next_pointer;
					struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
					if((victim_permissions & PERM_MODIFIED) == PERM_MODIFIED){
						pf_update_env_page(curenv, replaced_va, victim_frame);
					}
					unmap_frame(curenv->env_page_directory, replaced_va);
				}
				else{
					sched_kill_env(curenv->env_id);
				}
			}
			else{
				uint32 replaced_va = curenv->page_last_WS_element->virtual_address;
				int victim_permissions = pt_get_page_permissions(curenv->env_page_directory, replaced_va);
				struct FrameInfo* victim_frame = get_frame_info(curenv->env_page_directory, replaced_va, &ptr_page_table);
				struct WorkingSetElement* next_pointer = curenv->page_last_WS_element->prev_next_info.le_next;
				curenv->page_last_WS_element->virtual_address = new_va;
				if(next_pointer == NULL){
					curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
				}
				else
					curenv->page_last_WS_element = next_pointer;
				if((victim_permissions & PERM_MODIFIED) == PERM_MODIFIED){
					pf_update_env_page(curenv, replaced_va, victim_frame);
				}
				unmap_frame(curenv->env_page_directory, replaced_va);
			}
		}
	}
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX)){
			uint32 max_size = curenv->ActiveListSize + curenv->SecondListSize;
			uint32 active_list_size_max = curenv->ActiveListSize;
			uint32 second_list_size_max = curenv->SecondListSize;
			uint32 current_active_list_size = LIST_SIZE(&(curenv->ActiveList));
			uint32 current_second_list_size = LIST_SIZE(&(curenv->SecondList));
			uint32 *ptr_page_table = NULL;
			uint32 new_va = ROUNDDOWN(fault_va, PAGE_SIZE);
			unsigned int permissions = pt_get_page_permissions(curenv->env_page_directory, fault_va);
			//placement
			if(current_active_list_size + current_second_list_size < max_size){
				//case list1 not full
				if(current_active_list_size < active_list_size_max){

					struct FrameInfo* ptr_frame_info = NULL;
					int allocnewframe = allocate_frame(&ptr_frame_info);
					map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
					int readc = pf_read_env_page(curenv,(void*)new_va);
					if (readc == E_PAGE_NOT_EXIST_IN_PF){
						if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
							if(permissions == 0 || permissions == -1){
								sched_kill_env(curenv->env_id);
							}
						}
						else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){

						}
						else{
							sched_kill_env(curenv->env_id);
						}
					}

					struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
					LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
					ptr_frame_info->va = (uint32)new_element;

				}
				//case list1 full
				else{
					if(current_second_list_size == 0){
						struct FrameInfo* ptr_frame_info = NULL;
						int allocnewframe = allocate_frame(&ptr_frame_info);
						map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
						int readc = pf_read_env_page(curenv,(void*)new_va);
						if (readc == E_PAGE_NOT_EXIST_IN_PF){
							if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
								if(permissions == 0 || permissions == -1){
									sched_kill_env(curenv->env_id);
								}
							}
							else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){

							}
							else{
								sched_kill_env(curenv->env_id);
							}
						}

						struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
						struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
						pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
						LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
						LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
						LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
					}
					else{
						int fault_permissions = pt_get_page_permissions(curenv->env_page_directory, new_va);
						if(fault_permissions == 0){
							struct FrameInfo* ptr_frame_info = NULL;
							int allocnewframe = allocate_frame(&ptr_frame_info);
							map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
							int readc = pf_read_env_page(curenv,(void*)new_va);
							if (readc == E_PAGE_NOT_EXIST_IN_PF){
								if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
									if(permissions == 0 || permissions == -1){
										sched_kill_env(curenv->env_id);
									}
								}
								else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){

								}
								else{
									sched_kill_env(curenv->env_id);
								}
							}
							struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
							struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
							pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
							LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
							LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
							LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
						}
						else{
							struct WorkingSetElement* iterator = LIST_FIRST(&(curenv->SecondList));
							for(int i = 0; i < current_second_list_size; i++){
								if(new_va == iterator->virtual_address){
									break;
								}
								else{
									iterator = iterator->prev_next_info.le_next;
									continue;
								}
							}
							if(iterator == 0){
								struct FrameInfo* ptr_frame_info = NULL;
								int allocnewframe = allocate_frame(&ptr_frame_info);
								map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
								int readc = pf_read_env_page(curenv,(void*)new_va);
								if (readc == E_PAGE_NOT_EXIST_IN_PF){
									if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
										if(permissions == 0 || permissions == -1){
											sched_kill_env(curenv->env_id);
										}
									}
									else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){

									}
									else{
										sched_kill_env(curenv->env_id);
									}
								}
								struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
								struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
								pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
								LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
								LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
								LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
								return;
							}
							struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
							LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
							LIST_REMOVE(&(curenv->SecondList), iterator);
							pt_set_page_permissions(curenv->env_page_directory, iterator->virtual_address, PERM_PRESENT, 0);
							pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
							LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
							LIST_INSERT_HEAD(&(curenv->ActiveList), iterator);
						}
						return;
					}

				}
			}
			//replacement
			else{
				//look in second list
				int fault_permissions = pt_get_page_permissions(curenv->env_page_directory, new_va);
				if(fault_permissions == 0){
					struct FrameInfo* ptr_frame_info = NULL;
					int allocnewframe = allocate_frame(&ptr_frame_info);
					map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
					int readc = pf_read_env_page(curenv,(void*)new_va);
					if (readc == E_PAGE_NOT_EXIST_IN_PF){
						if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
							if(permissions == 0 || permissions == -1){
								sched_kill_env(curenv->env_id);
							}
						}
						else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){

						}
						else{
							sched_kill_env(curenv->env_id);
						}
					}
					pt_set_page_permissions(curenv->env_page_directory, new_va, PERM_PRESENT, 0);
					struct WorkingSetElement* victim_element = LIST_LAST(&(curenv->SecondList));
					struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
					struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
					uint32 victim_va = victim_element->virtual_address;
					int victim_permission = pt_get_page_permissions(curenv->env_page_directory, victim_element->virtual_address);
					struct FrameInfo* victim_frame = get_frame_info(curenv->env_page_directory, victim_va, &ptr_page_table);
					LIST_REMOVE(&(curenv->SecondList), victim_element);
					LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
					LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
					pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
					LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
					if((victim_permission & PERM_MODIFIED) == PERM_MODIFIED){
						pf_update_env_page(curenv, victim_va, victim_frame);
					}
					unmap_frame(curenv->env_page_directory, victim_va);
				}
				else{
					struct WorkingSetElement* iterator = LIST_FIRST(&(curenv->SecondList));
					for(int i = 0; i < second_list_size_max; i++){
						if(new_va == iterator->virtual_address){

							break;
						}
						else{
							iterator = iterator->prev_next_info.le_next;
							continue;
						}
					}
					if(iterator == 0){
						struct FrameInfo* ptr_frame_info = NULL;
						int allocnewframe = allocate_frame(&ptr_frame_info);
						map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
						int readc = pf_read_env_page(curenv,(void*)new_va);
						if (readc == E_PAGE_NOT_EXIST_IN_PF){
							if(new_va >= (uint32)USER_HEAP_START && new_va < (uint32)USER_HEAP_MAX){
								if(permissions == 0 || permissions == -1){
									sched_kill_env(curenv->env_id);
								}
							}
							else if(new_va >= (uint32)USTACKBOTTOM && new_va < (uint32)USTACKTOP){

							}
							else{
								sched_kill_env(curenv->env_id);
							}
						}
						pt_set_page_permissions(curenv->env_page_directory, new_va, PERM_PRESENT, 0);
						struct WorkingSetElement* victim_element = LIST_LAST(&(curenv->SecondList));
						struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, new_va);
						struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
						uint32 victim_va = victim_element->virtual_address;
						int victim_permission = pt_get_page_permissions(curenv->env_page_directory, victim_element->virtual_address);
						struct FrameInfo* victim_frame = get_frame_info(curenv->env_page_directory, victim_va, &ptr_page_table);
						LIST_REMOVE(&(curenv->SecondList), victim_element);
						LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
						LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
						pt_set_page_permissions(curenv->env_page_directory, overflowed_element->virtual_address, 0, PERM_PRESENT);
						LIST_INSERT_HEAD(&(curenv->ActiveList), new_element);
						if((victim_permission & PERM_MODIFIED) == PERM_MODIFIED){
							pf_update_env_page(curenv, victim_va, victim_frame);
						}
						unmap_frame(curenv->env_page_directory, victim_va);
					}
					else{
						struct WorkingSetElement* overflowed_element = LIST_LAST(&(curenv->ActiveList));
						uint32 overflowed_address = overflowed_element->virtual_address;
						struct WorkingSetElement* MRU_element = iterator;
						LIST_REMOVE(&(curenv->SecondList), MRU_element);
						LIST_REMOVE(&(curenv->ActiveList), overflowed_element);
						LIST_INSERT_HEAD(&(curenv->SecondList), overflowed_element);
						pt_set_page_permissions(curenv->env_page_directory, overflowed_address, 0, PERM_PRESENT);
						LIST_INSERT_HEAD(&(curenv->ActiveList), MRU_element);
						pt_set_page_permissions(curenv->env_page_directory, iterator->virtual_address, PERM_PRESENT, 0);
					}

				}

			}
		}

}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}




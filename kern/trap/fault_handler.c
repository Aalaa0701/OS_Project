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

	if(wsSize < (curenv->page_WS_max_size))
	{
		cprintf("placement\n");
		cprintf("fault address: %x\n");
		//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
		//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
		uint32 *ptr_page_table = NULL;
		uint32 new_va = ROUNDDOWN(fault_va, PAGE_SIZE);
		struct FrameInfo* ptr_frame_info = NULL;
		int allocnewframe = allocate_frame(&ptr_frame_info);
		map_frame(curenv->env_page_directory, ptr_frame_info, new_va, PERM_WRITEABLE|PERM_USER|PERM_PRESENT);
		// permission
		unsigned int permissions = pt_get_page_permissions(curenv->env_page_directory, fault_va);
		int readc = pf_read_env_page(curenv,(void*)new_va);
		if (readc == E_PAGE_NOT_EXIST_IN_PF){
		  if(new_va >= (uint32)USER_HEAP_START && new_va <= (uint32)USER_HEAP_MAX){
			  cprintf("in user area\n");
			  if((permissions & PERM_AVAILABLE) != PERM_AVAILABLE){
				  cprintf("will kill in user\n");
					 sched_kill_env(curenv->env_id);
				 }
			struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
			LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
			if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
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
			  cprintf("not in stack or heap\n");
			  unmap_frame(curenv->env_page_directory, new_va);
			sched_kill_env(curenv->env_id);
		  }
		}
		else{
			struct WorkingSetElement* element = env_page_ws_list_create_element(curenv,new_va);
			LIST_INSERT_TAIL(&(curenv->page_WS_list), element);
			if(LIST_SIZE(&(curenv->page_WS_list)) == curenv->page_WS_max_size){
				curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			}
		}

		//refer to the project presentation and documentation for details
	}
	else
	{
		//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
		//refer to the project presentation and documentation for details
		if(isPageReplacmentAlgorithmFIFO())
		{
			//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
			// Write your code here, remove the panic and write your code
			panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
		}
		if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
		{
			//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
			// Write your code here, remove the panic and write your code
			panic("page_fault_handler() LRU Replacement is not implemented yet...!!");

			//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
		}
	}
}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}




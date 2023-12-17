#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>


uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");

	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP
	//TODO: [PROJECT'23.MS3 - #4] [2] BSD SCHEDULER - sched_init_BSD
	//Your code is here
	//Comment the following line

    num_of_ready_queues = numOfLevels;
    env_ready_queues = kmalloc(num_of_ready_queues * sizeof(struct Env_Queue));
    quantums = kmalloc(num_of_ready_queues * sizeof(uint8));
    int zerovar = 0;
    fixed_point_t fixedzero = fix_int(zerovar);
    loadAVG = fixedzero;
    for(int i = 0;i < numOfLevels ; i++){
        quantums[i] = quantum;
        kclock_set_quantum(quantums[i]);
        init_queue(&(env_ready_queues[i]));
    }

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif
}


//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD
	//Your code is here
	//Comment the following line
//		cprintf("scheduler is called\n");
		if(curenv != NULL){
			int curenv_priority = curenv->priority;
			enqueue(&env_ready_queues[PRI_MAX - curenv_priority], curenv);
		}
		uint8 found = 0;
	    for(int i = 0; i < num_of_ready_queues; i++){
	        if(env_ready_queues[i].size != 0){
	        	struct Env* current_env = dequeue(&env_ready_queues[i]);
	        	//LIST_REMOVE(&env_ready_queues[i], current_env);
	        	if(current_env == NULL){
	        		found = 0;
	//        		cprintf("null env\n");
	        		continue;
	        	}
	        	found = 1;
	//        	cprintf("dequeued\n");
	//			cprintf("new priority in loop %d\n", current_env->priority);
	//			cprintf("id %d\n", current_env->env_id);
	//			cprintf("current cpu %d\n", current_env->recentCPU);
				kclock_set_quantum(quantums[0]);
	        	//enqueue(&env_ready_queues[PRI_MAX - current_env->priority], current_env);
	            return current_env;
	        }
	        else{
	        	found = 0;
	            continue;
	        }
	    }
	    if(found == 0){
	        int zerovar = 0;
	        fixed_point_t fixedzero = fix_int(zerovar);
	    	loadAVG = fixedzero;
	    }
	    return NULL;
}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================
void clock_interrupt_handler()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	{
		uint8 quantum = quantums[0];
		uint32 quantamised_ticks_current = quantum * ticks;
		uint32 quantamised_ticks_before = quantum * (ticks - 1);
		uint32 current_division = quantamised_ticks_current / 1000;
		uint32 before_division = quantamised_ticks_before / 1000;
		uint32 rounded_current = ROUNDDOWN(current_division, 1);
		uint32 rounded_before;
		if((int)quantamised_ticks_before > 0){
			rounded_before = ROUNDDOWN(before_division, 1);
		}
		else{
			rounded_before = 0;
		}

		//every tick
		//recent cpu for running process
		fixed_point_t prev_cpu = curenv->recentCPU;
		int one_var = 1;
		fixed_point_t fixed_one = fix_int(one_var);
		curenv->recentCPU = fix_add(prev_cpu, fixed_one);
		if(ticks % 4 == 0 && ticks > 0){
			for(int i = 0; i < num_of_ready_queues; i++){
				for(int j = 0; j < queue_size(&(env_ready_queues[i])); j++){
					struct Env* dequeued_env = dequeue(&(env_ready_queues[i]));
					if(dequeued_env == NULL){
						continue;
					}
					fixed_point_t recent_cpu_division = fix_unscale(dequeued_env->recentCPU, 4);
					int nice_multiplication = dequeued_env->nice * 2;
					int recent_cpu_int = fix_trunc(recent_cpu_division);
					int new_priority = PRI_MAX - recent_cpu_int - nice_multiplication;
					if(new_priority > PRI_MAX){
						new_priority = PRI_MAX;
					}
					if(new_priority < PRI_MIN){
						new_priority = PRI_MIN;
					}
					dequeued_env->priority = new_priority;
					enqueue(&env_ready_queues[PRI_MAX - new_priority], dequeued_env);
				}
			}
		}
		//every second
		if(rounded_current != rounded_before && rounded_before > 0){
			//recent cpu of every process & load avg
			//running process
			//load avg = loadavg * 59/60 + 1/60 * ready
			uint32 num_of_ready = 1;
			for(int i = 0; i < num_of_ready_queues; i++){
				num_of_ready += queue_size(&env_ready_queues[i]);
			}
			fixed_point_t prev_load = loadAVG;
			int first_int = 59 / 60;
			fixed_point_t first = fix_int(first_int);
			int second_int = 1 / 60;
			fixed_point_t second = fix_int(second_int);
			fixed_point_t first_multiplication = fix_mul(first, prev_load);
			fixed_point_t second_multiplication = fix_scale(second, num_of_ready);
			loadAVG = fix_add(first_multiplication, second_multiplication);
			//recent cpu for every process -> (((loadavg * 2)/(loadavg * 2)+1)*recent cpu )+ nice;
			//running
			int two_var = 2;
			fixed_point_t fixed_two = fix_int(two_var);
			fixed_point_t avg_multiplied = fix_mul(loadAVG, fixed_two);
			fixed_point_t avg_multiplied_added = fix_add(avg_multiplied, fixed_one);
			fixed_point_t division = fix_div(avg_multiplied, avg_multiplied_added);
			fixed_point_t new_prev_cpu = curenv->recentCPU;
			fixed_point_t coefficient_multiplication = fix_mul(division, new_prev_cpu);
			int nice_var = curenv->nice;
			fixed_point_t fixed_nice = fix_int(nice_var);
			curenv->recentCPU = fix_add(coefficient_multiplication, fixed_nice);
			//ready
			for(int i = 0; i < num_of_ready_queues; i++){
				for(int j = 0; j < queue_size(&env_ready_queues[i]); j++){
					struct Env* edited_env = dequeue(&env_ready_queues[i]);
					if(edited_env == NULL){
						continue;
					}
					fixed_point_t loop_prev_cpu = edited_env->recentCPU;
					fixed_point_t new_coefficient_multiplication = fix_mul(division, loop_prev_cpu);
					int loop_nice_var = edited_env->nice;
					fixed_point_t fixed_loop_nice = fix_int(loop_nice_var);
					edited_env->recentCPU = fix_add(new_coefficient_multiplication, fixed_loop_nice);
					enqueue(&env_ready_queues[i], edited_env);
				}
			}
			//blocked

		}





//		uint8 quantum = quantums[0];
//		uint32 quantamised_ticks_current = quantum * ticks;
//		uint32 quantamised_ticks_before = quantum * (ticks - 1);
//		uint32 current_division = quantamised_ticks_current / 1000;
//		uint32 before_division = quantamised_ticks_before / 1000;
//		uint32 rounded_current = ROUNDDOWN(current_division, 1);
//		uint32 rounded_before;
//		if((int)quantamised_ticks_before > 0){
//			rounded_before = ROUNDDOWN(before_division, 1);
//		}
//		else{
//			rounded_before = 0;
//		}
//
//		//every tick
//		//recent cpu for running process
//		fixed_point_t prev_cpu = curenv->recentCPU;
//	    int one_var = 1;
//	    fixed_point_t fixed_one = fix_int(one_var);
//		curenv->recentCPU = fix_add(prev_cpu, fixed_one);
//		//every 4th tick
//		if(ticks % 4 == 0 && ticks > 0){
//			if (scheduler_method == SCH_RR){
//
//			}
//			else{
////				fixed_point_t curenv_recent_cpu_division = fix_unscale(curenv->recentCPU, 4);
////				int curenv_nice_multiplication = curenv->nice * 2;
////				int curenv_recent_cpu_int = fix_trunc(curenv_recent_cpu_division);
////				int curenv_new_priority = PRI_MAX - curenv_recent_cpu_int - curenv_nice_multiplication;
////				curenv->priority = curenv_new_priority;
////				enqueue(&env_ready_queues[PRI_MAX - curenv_new_priority], curenv);
////				cprintf("4th tick\n");
////				cprintf("num of ready queues: %d\n", num_of_ready_queues);
//			}
//
//			for(int i = 0; i < num_of_ready_queues; i++){
//				if(env_ready_queues[i].size == 0){
//					continue;
//				}
//				if (scheduler_method == SCH_RR){
//
//				}
//				else{
////					for(int j = 0; j < env_ready_queues[i].size; j++){
////	//					cprintf("size of queue: %d\n", env_ready_queues[i].size);
////						struct Env* ready_env = dequeue(&env_ready_queues[i]);
////						if(ready_env == NULL){
////							continue;
////						}
////						fixed_point_t recent_cpu_division = fix_unscale(ready_env->recentCPU, 4);
////						int nice_multiplication = ready_env->nice * 2;
////						int recent_cpu_int = fix_trunc(recent_cpu_division);
////						int new_priority = PRI_MAX - recent_cpu_int - nice_multiplication;
////						if(new_priority > PRI_MAX){
////							new_priority = PRI_MAX;
////						}
////						if(new_priority < PRI_MIN){
////							new_priority = PRI_MIN;
////						}
////
////						ready_env->priority = new_priority;
////	//					cprintf("new priority in loop %d\n", new_priority);
////	//					cprintf("id %d\n", ready_env->env_id);
////						enqueue(&env_ready_queues[PRI_MAX - new_priority], ready_env);
//					//}
//				}
////				fixed_point_t recent_cpu_division = fix_scale(curenv->recentCPU, 4);
////				int nice_multiplication = curenv->nice * 2;
////				int recent_cpu_int = fix_trunc(recent_cpu_division);
////				int new_priority = PRI_MAX - recent_cpu_int - nice_multiplication;
////				if(new_priority > PRI_MAX){
////					new_priority = PRI_MAX;
////				}
////				if(new_priority < PRI_MIN){
////					new_priority = PRI_MIN;
////				}
////	//			cprintf("new priority after loop%d\n", new_priority);
////	//			cprintf("id %d\n", curenv->env_id);
////				curenv->priority = new_priority;
////				enqueue(&env_ready_queues[PRI_MAX - new_priority], curenv);
//				}
//
//		}
//		//every second
//		if(rounded_current != rounded_before && rounded_before > 0){
//			cprintf("second passed\n");
//			//recent cpu of every process & load avg
//			//running process
//			//load avg = loadavg * 59/60 + 1/60 * ready
//			uint32 num_of_ready = 1;
//			for(int i = 0; i < num_of_ready_queues; i++){
//				num_of_ready += queue_size(&env_ready_queues[i]);
//			}
//			fixed_point_t prev_load = loadAVG;
//			int first_int = 59 / 60;
//			fixed_point_t first = fix_int(first_int);
//			int second_int = 1 / 60;
//			fixed_point_t second = fix_int(second_int);
//			fixed_point_t first_multiplication = fix_mul(first, prev_load);
//			fixed_point_t second_multiplication = fix_scale(second, num_of_ready);
//			loadAVG = fix_add(first_multiplication, second_multiplication);
//			//recent cpu for every process -> (((loadavg * 2)/(loadavg * 2)+1)*recent cpu )+ nice;
//			//running
//		    int two_var = 2;
//		    fixed_point_t fixed_two = fix_int(two_var);
//		    fixed_point_t avg_multiplied = fix_mul(loadAVG, fixed_two);
//		    fixed_point_t avg_multiplied_added = fix_add(avg_multiplied, fixed_one);
//		    fixed_point_t division = fix_div(avg_multiplied, avg_multiplied_added);
//		    fixed_point_t new_prev_cpu = curenv->recentCPU;
//		    fixed_point_t coefficient_multiplication = fix_mul(division, new_prev_cpu);
//		    int nice_var = curenv->nice;
//		    fixed_point_t fixed_nice = fix_int(nice_var);
//			curenv->recentCPU = fix_add(coefficient_multiplication, fixed_nice);
//			//ready
//			for(int i = 0; i < num_of_ready_queues; i++){
//				for(int j = 0; j < queue_size(&env_ready_queues[i]); j++){
//					struct Env* edited_env = dequeue(&env_ready_queues[i]);
//					fixed_point_t loop_prev_cpu = edited_env->recentCPU;
//					fixed_point_t new_coefficient_multiplication = fix_mul(division, loop_prev_cpu);
//				    int loop_nice_var = edited_env->nice;
//				    fixed_point_t fixed_loop_nice = fix_int(loop_nice_var);
//				    edited_env->recentCPU = fix_add(new_coefficient_multiplication, fixed_loop_nice);
//				    enqueue(&env_ready_queues[i], edited_env);
//				}
//			}
//			//blocked
//
//		}


	}


	/********DON'T CHANGE THIS LINE***********/
	ticks++ ;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}

//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}


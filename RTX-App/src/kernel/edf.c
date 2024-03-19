#include "edf.h"
#include "ready_queue.h"
#include "common_ext.h"
#include "k_inc.h"


int timeval_to_tick (TIMEVAL time_val){

	if (time_val.usec % RTX_TICK_SIZE != 0){
		return -1;
	}

	int ret = time_val.usec / RTX_TICK_SIZE;
	ret += time_val.sec * 2000;

	return ret;
}

int missed_deadline(TCB * task)
{
	return task->job_id < get_num_jobs(task);
}

int get_num_jobs(TCB * task)
{
	int total_ticks_passed = g_timer_count - task->set_time;
	//return number of periods passed since set
	return total_ticks_passed / timeval_to_tick(task->period);
}



#ifndef EDF_H
#define EDF_H

#include "k_task.h"

int timeval_to_tick (TIMEVAL time_val);
int missed_deadline(TCB * task);
int get_num_jobs(TCB * task);
#endif

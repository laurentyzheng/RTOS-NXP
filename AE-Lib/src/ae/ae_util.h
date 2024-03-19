#ifndef AE_UTIL_H_
#define AE_UTIL_H_
#include "ae.h"
#include "ae_inc.h"

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */

void test_abort(int test_id, int index);
void test_exit(void);

void print_results(AE_CASE *p_ae_cases);
int  print_log(int test_id, int index, int result, char *msg); 
int  process_sub_result(int test_id, int index, int result);
void print_summary(void);

char *strcpy(char* dest, const char* source);

#endif // ! AE_UTIL_H_


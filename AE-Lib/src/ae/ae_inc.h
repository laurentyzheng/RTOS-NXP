#ifndef AE_INC_H_
#define AE_INC_H_
#include "ae.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */

#define MAX_LEN_MSG         128     // test message in bytes including '\0'
#define MAX_LEN_SEQ         32      // max test case execution sequence record length
/*
 *===========================================================================
 *                             TYPEDEFS AND STRUCTURES
 *===========================================================================
 */


/**
 * @brief   test case data object
 * @note    each test suite contains num_tests cases, where each test case
 *          contains some sub cases.
 * @details this structure is used by all test case objects
 *          only one test case object can use at a time
 */
typedef struct ae_xtest
{
    U8    test_id;                  /**< test case ID */
    U8    index;                    /**< sub test index in a specific test case*/
    U8    num_tests;                /**< number of test cases/functions in a test suite*/
    U8    num_tests_run;            /**< number of completed test cases/functions */
    char  msg[MAX_LEN_MSG];         /**< testing message buffer */
} AE_XTEST;

/**
 * @brief   generic test case object
 */

typedef struct ae_case 
{
    U32         results;            /**< test results, each test has sub test for each bit */
    U8          test_id;            /**< test ID **/
    U8          num_bits;           /**< results are saved in bits[0:num-1] */
} AE_CASE;  

/**
 * @briekf  task management test case object
 */

typedef struct ae_case_tsk
{
    AE_CASE     *p_ae_case;                 /**< points to generic case object */    
    U8          len;                        /**< length of seq_exec array */
    U8          pos;                        /**< next free spot position in the seq array*/
    U8          pos_expt;                   /**< expected next free spot position in seq_expt when test finishes */
    task_t      seq[MAX_LEN_SEQ];           /**< actual task execution order   */
    task_t      seq_expt[MAX_LEN_SEQ];      /**< expected task exectuion order */
} AE_CASE_TSK;

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

// The following globals are defined in the Test Suite .c file
extern const char   PREFIX[];       // test output file prefix
extern const char   PREFIX_LOG[];   // test output file log prefix
extern AE_XTEST     g_ae_xtest;     // test data, re-use by each test case
extern AE_CASE      g_ae_cases[];   // points to the array of testing cases
extern AE_CASE_TSK  g_tsk_cases[];  // array of testing cases


/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */

#endif // ! AE_INC_H_


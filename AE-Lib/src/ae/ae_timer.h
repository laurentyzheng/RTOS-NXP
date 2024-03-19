#ifndef _AE_TIMER_H_
#define _AE_TIMER_H_
#include <timer.h>

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */
 
#define BIT(X) ( 1 << (X) )

/*
 *===========================================================================
 *                             STRUCTURES
 *===========================================================================
 */
 

/**
 * @brief   data strucure to represent time in seconds and milliseconds
 * @note    Example: to represent 3.004 seconds, set
 *          sec = 3 and msec = 4
 */
struct ae_time {
    uint32_t sec;       /*< seconds             */
    uint32_t msec;      /*< milliseconds        */
};

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */

uint32_t ae_timer_init_100MHZ(uint8_t n_timer);
int      ae_get_tick_diff    (struct ae_time *tm, TM_TICK *tk1, TM_TICK *tk2);
void     ae_spin             (uint32_t msec);

#endif /* ! _AE_TIMER_H_ */

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

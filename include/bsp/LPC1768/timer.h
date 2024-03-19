#ifndef _TIMER_H_
#define _TIMER_H_

#include <LPC17xx.h>

/*
 *===========================================================================
 *                            MACROS
 *===========================================================================
 */

#define TIMER0  0
#define TIMER1  1
#define TIMER2  2
#define TIMER3  3

/*
 *===========================================================================
 *                            STRUCTURES AND TYPEDEFS
 *===========================================================================
 */

typedef struct tm_tick {
    uint32_t tc;  /**< TC register value, for TIMER1 it is in seconds */
    uint32_t pc;  /**< PC register value, for TIMER1 it is in tens of nano seconds */
} TM_TICK;

/*
 *===========================================================================
 *                            FUNCTION PROTOTYPES
 *===========================================================================
 */
 

extern uint32_t timer_irq_init      (uint8_t n_timer);  /* interrupt-driven */
extern uint32_t timer_freerun_init  (uint8_t n_timer);  /* free running     */
extern int      get_tick            (TM_TICK *tk, uint8_t n_timer); 

#endif /* ! _TIMER_H_ */

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
 

#define WORD_SIZE       32                           /* word size in bits    */
#define IROM1_BASE      0x0
#define IROM1_SIZE      0x80000                      /* size in bytes        */
#define IROM_BASE       IROM1_BASE

#define IRAM1_BASE      0x10000000                   /* IRAM1 start addr     */
#define IRAM1_SIZE      0x8000                       /* size in bytes        */
#define IRAM1_SIZE_LOG2 0xF                          /* log2(IRAM1_SIZE)     */
#define IRAM2_BASE      0x2007C000
#define IRAM2_SIZE      0x8000                       /* size in bytes        */
#define IRAM2_SIZE_LOG2 0xF                          /* log2(IRAM2_SIZE)     */

#define RTX_IMG_END     (Image$$RW_IRAM1$$ZI$$Limit) /* linker-defined symbol*/
#define RAM1_START_RT   (U32)(&RTX_IMG_END)             
#define RAM1_SIZE       0x1000                       /* RAM1 size in bytres  */
#define RAM1_SIZE_LOG2  0xC                          /* log2(RAM1_SIZE)      */
#define RAM1_START      (IRAM1_BASE + IRAM1_SIZE - RAM1_SIZE)
#define RAM1_END        (IRAM1_BASE + IRAM1_SIZE - 1)

#define RAM2_START      (IRAM2_BASE)
#define RAM2_END        (IRAM2_BASE + IRAM2_SIZE - 1)
#define RAM2_SIZE       IRAM2_SIZE                   /* RAM2 size in bytes   */   
#define RAM2_SIZE_LOG2  IRAM2_SIZE_LOG2              /* log2(RAM2_SIZE)      */


/*
 *===========================================================================
 *                            GLOBAL VARIABLES
 *===========================================================================
 */
 
extern unsigned int     RTX_IMG_END;

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */


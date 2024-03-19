/**
 * @brief The Wall Clock Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "common_ext.h"
#include "printf.h"

static volatile int alive = 1;

static U8 hours = 0;
static U8 minutes = 0;
static U8 seconds = 0;

static volatile U8 doDisplayClock = 1;

static size_t strlen( char * str )
{
    size_t iterator;
    for( iterator = 0; str[iterator]; iterator++ );
    return iterator;
}

// returns dest's end address
static char * strcpy( char * dest, char * src )
{
    // single equals is intentional
    while( ( *dest = *src ) != '\0' )
    {
        dest++;
        src++;
    }
    return dest;
}

int findEnd( char * str )
{
    int iterator;
    for( iterator = 0; str[iterator]; iterator++ );
    return iterator;
}

static int isNumChar( char c )
{
    return ( '0' <= c ) && ( c <= '9' );
}

static void reset_clock()
{
    hours = 0;
    minutes = 0;
    seconds = 0;
}

static void tick()
{
    seconds++;
    if( seconds >= 60 )
    {
        seconds = 0;
        minutes++;
        if( minutes >= 60 )
        {
            minutes = 0;
            hours++;
            if( hours >= 24 )
            {
                hours = 0;
            }
        }
    }
}


/*
 * Handle commands
 */

int doResetArrows()
{
    char txBuff[ MSG_HDR_SIZE ];
    RTX_MSG_HDR * leHead = (RTX_MSG_HDR*) txBuff;
    leHead->length = MSG_HDR_SIZE;
    leHead->sender_tid = TID_KCD;
    leHead->type = DISPLAY;
    
    int result = send_msg_nb( TID_CON, txBuff );
    return result;
}

static int doWT( char * msg, size_t msgLen )
{
    // ignore all arguments
    doDisplayClock = !doDisplayClock;
    return RTX_OK;
}

static int doWR( char * msg, size_t msgLen )
{
    reset_clock();
    return RTX_OK;
}


static int validateArgFormatWS( char * msg, size_t msgLen )
{
    if( msgLen < 11 )
        return RTX_ERR;
    
    int result = 1;
    result &= isNumChar( msg[3] );
    result &= isNumChar( msg[4] );
    result &= msg[5] == ':';
    result &= isNumChar( msg[6] );
    result &= isNumChar( msg[7] );
    result &= msg[8] == ':';
    result &= isNumChar( msg[9] );
    result &= isNumChar( msg[10] );
    
    return result ? RTX_OK : RTX_ERR;
}

static int doWS( char * msg, size_t msgLen )
{
    int result = validateArgFormatWS( msg, msgLen );
    if( result == RTX_ERR )
        return RTX_ERR;
    
    int newHours = ( msg[3] - '0' )*10 + ( msg[4] - '0' );
    int newMinutes = ( msg[6] - '0' )*10 + ( msg[7] - '0' );
    int newSeconds = ( msg[9] - '0' )*10 + ( msg[10] - '0' );
    
    if( ( 0 > newHours ) || ( newHours > 23 ) ||
        ( 0 > newMinutes ) || ( newMinutes > 59 ) ||
        ( 0 > newSeconds ) || ( newSeconds > 59 ) )
    {
        return RTX_ERR;
    }
    
    hours = newHours;
    minutes = newMinutes;
    seconds = newSeconds;
    
    return RTX_OK;
}

/*
 * Mail handling stuff
 */

static char getClockCommand( char * msg, size_t msgSize )
{
    if( msgSize >= 2 )
    {
        return msg[1];
    }
    else
        return 0;
}

static int handleMail( int mbx_id, int mailboxFilledSize )
{
    void * mailBuffer = mem_alloc( mailboxFilledSize );
    if( mailBuffer == NULL )
        return RTX_ERR;
    
    int result = RTX_OK;
    // The comma operator once again demonstrating that it's useful
    while( result = recv_msg_nb( mailBuffer, mailboxFilledSize ), result != RTX_ERR )
    {
        RTX_MSG_HDR * leHeader = (RTX_MSG_HDR*) mailBuffer;
        char * msg = ((char*) mailBuffer) + MSG_HDR_SIZE;
        size_t msgSize = leHeader->length - MSG_HDR_SIZE;
        
        char command = getClockCommand( msg, msgSize );
        switch( command )
        {
            case 'R':
            {
                result = doWR( msg, msgSize );
                break;
            }
            case 'S':
            {
                result = doWS( msg, msgSize );
                break;
            }
            case 'T':
            {
                result = doWT( msg, msgSize );
                break;
            }
            default:
            {
                // throw error
                result = RTX_ERR;
                break;
            }
        }
    }
    
    mem_dealloc( mailBuffer );
    mailBuffer = NULL;
    
    return doResetArrows();
}

/*
 * Clock Text stuff
 */

// assumes output looks like this XX:XX:XX, '\0'
void setClockText( char * output )
{
    if( doDisplayClock )
    {
        output[0] = ( hours / 10 ) + '0';
        output[1] = ( hours % 10 ) + '0';
        output[2] = ':';
        output[3] = ( minutes / 10 ) + '0';
        output[4] = ( minutes % 10 ) + '0';
        output[5] = ':';
        output[6] = ( seconds / 10 ) + '0';
        output[7] = ( seconds % 10 ) + '0';
    }
    
    else
    {
        int i;
        for( i = 0; i < 8; i++ )
            output[i] = ' ';
    }
    
}

int doDisplay()
{
    static char msgBuff[] = "00:00:00";
    setClockText( msgBuff );
    // printf("%s\n\r", msgBuff);
    // reference:
    // https://www.lihaoyi.com/post/BuildyourownCommandLinewithANSIescapecodes.html
    // this sequence should:
    // -save current position
    // -set position to top left
    // -move cursor to top right
    // -move cursor 8 spaces left
    // -display time
    // -restores last saved cursor
    static char * prefixControlSequence = "\033[s\033[0;92H";
    //static char * prefixControlSequence = "EE";
    size_t prefixSize = strlen( prefixControlSequence );
    static char * postfixControlSequence = "\033[u";
    //static char * postfixControlSequence = "EE";
    size_t postfixSize = strlen( postfixControlSequence );

    size_t msgLength = MSG_HDR_SIZE + 8 + prefixSize + postfixSize + 1;
    void * txBuff = mem_alloc( msgLength );
    
    if( txBuff == NULL )
        return RTX_ERR;
    
    RTX_MSG_HDR * leHead = (RTX_MSG_HDR*) txBuff;
    leHead->length = msgLength;
    leHead->sender_tid = TID_WCLCK;
    leHead->type = DISPLAY;
    
    char * msgBegin = MSG_HDR_SIZE + (char*) txBuff;
    char * msgTimePos = strcpy( msgBegin, prefixControlSequence );
    char * msgPostfixPos = strcpy( msgTimePos, msgBuff );
    char * msgEnd = strcpy( msgPostfixPos, postfixControlSequence );
    *msgEnd = 0;
    
    int result = send_msg_nb( TID_CON, txBuff );
		if (result == RTX_ERR){
			printf("WALL CLOCK COULDN\'T SEND TO TID_CON \r\n");
		}
    
    mem_dealloc( txBuff );
    txBuff = NULL;
    msgBegin = NULL;
    msgTimePos = NULL;
    msgPostfixPos = NULL;
    msgEnd = NULL;
    
    return result;
}

void task_wall_clock(void)
{
    alive = 1;
    reset_clock();
    
    // setup mailbox
    int mbx_id = mbx_create( WCLK_MBX_SIZE );
    
    if( mbx_id == RTX_ERR )
    {
        tsk_exit();
    }
    
    // register the W command with KCD
    void * regWithKCDMsg = mem_alloc( MSG_HDR_SIZE + 1 );
    RTX_MSG_HDR * kcdMsgHdr = (RTX_MSG_HDR*) regWithKCDMsg;
    kcdMsgHdr->length = MSG_HDR_SIZE + 1;
    kcdMsgHdr->sender_tid = TID_WCLCK;
    kcdMsgHdr->type = KCD_REG;
    char * letter = ( (char*) regWithKCDMsg ) + MSG_HDR_SIZE;
    *letter = 'W';
    
    if( send_msg_nb( TID_KCD, regWithKCDMsg ) == RTX_ERR )
    {
        mem_dealloc( regWithKCDMsg );
        tsk_exit();
    }
    
    mem_dealloc( regWithKCDMsg );
    
    // elevate this to a real time task
    
    TIMEVAL leTime = {.sec = 1, .usec = 0};
    int result = rt_tsk_set( &leTime );
    
    if( result == RTX_ERR )
    {
        tsk_exit();
    }
    
    while( alive )
    {
		//printf("Wall clk about to sus;)");
        result = rt_tsk_susp();
		//printf("Wall clk come back from sus");
        if( result == RTX_ERR )
        {
            alive = 0;
            break;
        }
        // handle commands
        int mailboxFilledSize = WCLK_MBX_SIZE - mbx_get( TID_WCLCK );
        if( mailboxFilledSize > 0 )
        {
            handleMail( mbx_id, mailboxFilledSize );
        }
        
        // handle ticking
        tick();
        
        // handle display
        doDisplay();
        
    }

    tsk_exit();
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */


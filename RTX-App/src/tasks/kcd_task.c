/** 
 * @brief The KCD Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "printf.h"

//#define DEBUG

#define DECODE_OFFSET 1

#ifdef DEBUG
#define PrintDebug( x ) printf( x )
#else
#define PrintDebug( x )
#endif

U8 tidTable[62] = {0xff};
char msgBuff[ KCD_CMD_BUF_SIZE ] = {0};
size_t msgBuffSize;

U8 alphaNumToOffset( char C )
{
    if( 'a' <= C && C <= 'z' )
        return C - 'a';
    else if( 'A' <= C && C <= 'Z' )
        return C - 'A' + 26;
    else if( '0' <= C && C <= '9' )
        return C - '0' + 52;
    else
        return 0xff;
}

char * stateToChar( U8 state )
{
    // space for dormant
    // i for idle (ready)
    // r for running
    // S for blocked on send
    // R for blocked on receive
    // . for suspended, which is not used right now.
    static char * table[6] = { "", "RD", "RN", "BS", "BR", "SP" };
    return table[state];
}
// string length WITHOUT null character
static size_t strlen( char * str )
{
    size_t iterator;
    for( iterator = 0; str[iterator]; iterator++ );
    return iterator;
}

// finds the address of the null character of a string.
static char * findEnd( char * str )
{
    for( ; *str; str++ );
    
    return str;
}

//
// Functions associated with registerring
//

// Assume command is a string containing just the command portion of the string.
void registerTask( char command, task_t tid )
{
    U8 offset = alphaNumToOffset( command );
    tidTable[offset] = (U8) tid;
}

task_t obtainTask( char command )
{
    U8 offset = alphaNumToOffset( command );
    return tidTable[offset];
}

int validateCommandWRTRegister( char leCommand )
{
    int result = leCommand != 'L';

    result &= ( 'A' <= leCommand && leCommand <= 'Z' )
            ||( 'a' <= leCommand && leCommand <= 'z' )
            ||( '0' <= leCommand && leCommand <= '9' );
    // past this point, iterator is now the length of the list.
    return result;
}

int processRegister( U8 * leBuffer )
{
    RTX_MSG_HDR * leHeader = (RTX_MSG_HDR*) leBuffer;
    task_t tid = leHeader->sender_tid;
    char * leChar = (char*) (leBuffer + MSG_HDR_SIZE);

    if (leHeader->length != MSG_HDR_SIZE + 1 || !validateCommandWRTRegister( *leChar ))
        return RTX_ERR;
    
    registerTask( *leChar, tid );
    return RTX_OK;
}

void send_error(int invalid){
		char* bad_cmd_msg = mem_alloc(MSG_HDR_SIZE + 25);
		if( bad_cmd_msg == NULL ){
			return;
		}
		char * msg = (invalid) ? "Invalid Command.\n\r\0" : "Command Not Found.\n\r\0";
		RTX_MSG_HDR * header = (RTX_MSG_HDR *) bad_cmd_msg;
		header->length = MSG_HDR_SIZE + strlen(msg);
		header->sender_tid = TID_KCD;
		header->type = DISPLAY;
		sprintf(bad_cmd_msg + 6, msg);
		send_msg_nb (TID_CON, bad_cmd_msg);
		mem_dealloc(bad_cmd_msg);
}

//
// Functions associated with KEY_IN
//

int validateCommand( char * msg, size_t length )
{
    int result = 1;
    
    result &= length >= 2 && length <= ( KCD_CMD_BUF_SIZE - MSG_HDR_SIZE );
    result &= ( msg[0] == '%' );
    
    return result;
}

int processLCommand( char * leString )
{
    if( strlen( leString ) < 3 )
        return RTX_ERR;
    
    char arg = leString[2];
    switch( arg )
    {
        case 'T':
        case 'M':
        {
            task_t * taskBuffer = mem_alloc( 9*sizeof( task_t ) );
            if( taskBuffer == NULL )
            {
                return RTX_ERR;
            }
            // This is the only distinction between T and M.
            int numTasks = (arg == 'T') ? tsk_ls( taskBuffer, 9 ) : mbx_ls( taskBuffer, 9 );
            char * stringHeader = (arg == 'T') ? "NON-DORMANT Tasks: \r\n" : "Tasks with mailboxes: \r\n" ;
            char * outputFormat = (arg == 'T') ? "%d-%s\r\n" : "%d-%s:%d\r\n";
						
            // stringHeader, extra null character == 1, max size of output line == 14
            size_t msgBufferSize = MSG_HDR_SIZE + 20*numTasks + sizeof(stringHeader) + 1;
            char * msg = mem_alloc( msgBufferSize );
            if( msg == NULL )
            {
                mem_dealloc( taskBuffer );
                taskBuffer = NULL;
                return RTX_ERR;
            }
            
            RTX_MSG_HDR * leReturnHeader = (RTX_MSG_HDR*) msg;
            
            // Create string
            char * msgIterator = msg + MSG_HDR_SIZE;
            sprintf( msgIterator, stringHeader );
            msgIterator += strlen( stringHeader );
            size_t iterator;
            for( iterator = 0; iterator < numTasks; iterator++ )
            {
                task_t tid = taskBuffer[iterator];
                RTX_TASK_INFO leInfo;
                tsk_get( tid, &leInfo );
                
                msgIterator = findEnd( msgIterator );
								if (arg == 'T'){
									sprintf( msgIterator, outputFormat, tid, stateToChar( leInfo.state ) );
								} else {
									sprintf( msgIterator, outputFormat, tid, stateToChar( leInfo.state ), mbx_get(tid) );
								}
            }
            
            msgIterator = findEnd( msgIterator );
            // end address + 1 - start address.
            size_t messageSize = (size_t) ( msgIterator + 1 - msg );
            
            leReturnHeader->length = messageSize;
            leReturnHeader->sender_tid = TID_KCD;
            leReturnHeader->type = DISPLAY;
            
            int result = send_msg_nb( TID_CON, (void*) msg );
            
            // deallocate stuff.
            
            mem_dealloc( msg );
            msg = NULL;
            msgIterator = NULL;
            leReturnHeader = NULL;
            
            mem_dealloc( taskBuffer );
            taskBuffer = NULL;
            
            return result;
        }
        default:
        {
            send_error(0);
            return RTX_ERR;
        }
    }
}

int processString( char * leString )
{
    size_t leStringLength = strlen( leString );
		PrintDebug(leString);
		
    if( !validateCommand( leString, leStringLength ) )
    {
        send_error(1);
        return RTX_ERR;
    }
    
    char leCommand = leString[1];
    if( leCommand == 'L' )
    {
        return processLCommand( leString );
    }

    // Get rid of the %
    size_t outgoingMessageSize = MSG_HDR_SIZE + leStringLength - 1;
    U8 * outgoingMessage = mem_alloc( outgoingMessageSize );
    if( outgoingMessage == NULL )
    {
        return RTX_ERR;
    }
    RTX_MSG_HDR * leOutgoingHeader = (RTX_MSG_HDR*) outgoingMessage;
    
    leOutgoingHeader->sender_tid = TID_KCD;
    leOutgoingHeader->type = KCD_CMD;
    leOutgoingHeader->length = outgoingMessageSize;
    

    size_t iterator;
    for( iterator = 0; iterator < leStringLength - 1; iterator++ )
    {
        // MSG_HDR_SIZE because it is copying to past the header part.
        outgoingMessage[MSG_HDR_SIZE + iterator] = leString[1 + iterator];
    }
    
    U8 taskOfCommand = obtainTask( leCommand );
    RTX_TASK_INFO leInfo;
    tsk_get( (task_t)taskOfCommand, &leInfo );
    
    int result = RTX_ERR;
    if( taskOfCommand == TID_NULL || taskOfCommand == TID_UNK || leInfo.state == DORMANT)
    {
        send_error(0);
    } else {
        result = send_msg_nb( taskOfCommand, (void*) outgoingMessage );
    }
    
    mem_dealloc( outgoingMessage );
    outgoingMessage = NULL;
    leOutgoingHeader = NULL;
    
    return result;
}

int processKeyIn( U8 * msgBuffer )
{
    RTX_MSG_HDR * leHeader = (RTX_MSG_HDR*) msgBuffer;
    if( leHeader->sender_tid != TID_UART )
    {
        // handle error.
        return RTX_ERR;
    }
    
    char * leChars = (char*) (msgBuffer + MSG_HDR_SIZE);
    size_t leStringSize = leHeader->length - MSG_HDR_SIZE;
    
    int doProcessString = 0;
    int doEcho = 1;
    if( *leChars == CHAR_BACKSPACE )
    {
        if( msgBuffSize > 0 )
        {
            msgBuffSize--;
            msgBuff[msgBuffSize] = 0;
        }
    }
    else if( *leChars == CHAR_TERMINATE )
    {
        doProcessString = 1;
    }
    else if( leStringSize + msgBuffSize <= KCD_CMD_BUF_SIZE )
    {
        size_t iterator;
        for( iterator = 0; iterator < leStringSize; iterator++ )
        {
            msgBuff[ msgBuffSize + iterator ] = leChars[ iterator ];
        }
        msgBuff[msgBuffSize + leStringSize] = 0;
        msgBuffSize += leStringSize;
    }
    else
    {
        doEcho = 0;
    }
    
    int result = RTX_OK;
    
    if( doEcho )
    {
        // reuse the header to echo.
        leHeader->sender_tid = TID_KCD;
        leHeader->type = DISPLAY;
        result |= send_msg( TID_CON, (void*) msgBuffer );
    }
    
    if( doProcessString )
    {   
        result |= processString( msgBuff );
        msgBuffSize = 0;
        msgBuff[0] = 0;
    }
    
    return result;
}


void task_kcd(void)
{
    // Assume that UART0 will not be registerring for outputs.
    int alive = 1;
    int result = mbx_create( KCD_MBX_SIZE );
    
    if( result == RTX_ERR )
    {
        PrintDebug( "Mailbox not created for kcd.\r\n" );
        alive = 0;
    }

    while( alive )
    {
        size_t leBufferSize = MSG_HDR_SIZE + 2;
        void *  mailboxBuffer = mem_alloc( leBufferSize );

        if( mailboxBuffer == NULL )
        {
            PrintDebug( "receiving buffer not created for kcd.\r\n" );
            break;
        }

        result = recv_msg( mailboxBuffer, leBufferSize ); 
				
        RTX_MSG_HDR * leHeader = (RTX_MSG_HDR*) mailboxBuffer;
        
        
        switch( leHeader->type )
        {
            case KCD_REG:
            {
                //PrintDebug( "KCD obtained REG data.\r\n" );
                result = processRegister( (U8*) mailboxBuffer );
                break;
            }
            case KEY_IN:
            {
                //PrintDebug( "KCD obtained data from UART0.\r\n" );
                result = processKeyIn( (U8*) mailboxBuffer );
                break;
            }
            default:
            {
                // Text is automatically cleared out of the buffer.
                break;
            }
        }
        
        mem_dealloc( mailboxBuffer );
        mailboxBuffer = NULL;
        leBufferSize = 0;
        leHeader = NULL;
        
    }

    tsk_exit();
    
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */


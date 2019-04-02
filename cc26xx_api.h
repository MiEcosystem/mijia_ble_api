/******************************************************************************

 @file  cc26xx_api.h

 @brief porting layer for the MiBLE API, for the CC26XX familly devices of TI.

 Group: WCS, BTS
 Target Device: cc13x2_26x2

 ******************************************************************************
 
 Copyright (c) 2018-2019, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 
 
 *****************************************************************************/

#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Task.h>
#include <UartLog.h>  // Comment out if using xdc Log
#include "mible_api.h"
//#include <Board.h>
                //uint8_t str[] = "ASSERT: in '"__FUNCTION__ "' ,'" #cond "' failed"; \
                //const uint8_t str[] = "test" __FUNCTION__; \
                //uint8_t str[] = "test"; \

#ifdef MIBLE_ASSERT

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define MYASSERT "ASSERT: in "__func__" because "STRINGIFY(cond)" failed"
#define MUASSERT2 "ASSERT: in %s/%dCondition: %s", (uintptr_t)__FUNCTION__, (uintptr_t)__LINE__, (uintptr_t)#cond
//                UART_write(UART_open(Board_UART0, NULL), MYASSERT, sizeof(MYASSERT)); \
                //Log_error0(MYASSERT); \


#define MIBLE_PORT_ASSERT(cond) if (!(cond)) { \
                Log_error3("ASSERT: in %s/%d Condition: %s", (uintptr_t)__FUNCTION__, (uintptr_t)__LINE__, (uintptr_t)#cond); \
                Task_sleep(1000000); \
                while(1); \
}

#else /* MIBLE_ASSERT */
#define MIBLE_PORT_ASSERT(cond) { \
    (void) (cond); \
}
#endif /* MIBLE_ASSERT */

#ifdef MIBLE_PORT_LOG
#define MIBLE_PORT_LOG_INFO0(fmt, arg...)  Log_info0(fmt, ##arg)
#define MIBLE_PORT_LOG_INFO1(fmt, arg...)  Log_info1(fmt, ##arg)
#define MIBLE_PORT_LOG_INFO2(fmt, arg...)  Log_info2(fmt, ##arg)
#define MIBLE_PORT_LOG_INFO3(fmt, arg...)  Log_info3(fmt, ##arg)
#define MIBLE_PORT_LOG_INFO4(fmt, arg...)  Log_info4(fmt, ##arg)
#define MIBLE_PORT_LOG_INFO5(fmt, arg...)  Log_info5(fmt, ##arg)

#define MIBLE_PORT_LOG_WAR0(fmt, arg...)   Log_warning0(fmt, ##arg)
#define MIBLE_PORT_LOG_WAR1(fmt, arg...)   Log_warning1(fmt, ##arg)
#define MIBLE_PORT_LOG_WAR2(fmt, arg...)   Log_warning2(fmt, ##arg)
#define MIBLE_PORT_LOG_WAR3(fmt, arg...)   Log_warning3(fmt, ##arg)
#define MIBLE_PORT_LOG_WAR4(fmt, arg...)   Log_warning4(fmt, ##arg)
#define MIBLE_PORT_LOG_WAR5(fmt, arg...)   Log_warning5(fmt, ##arg)

#define MIBLE_PORT_LOG_ERR0(fmt, arg...)   Log_error0(fmt, ##arg)
#define MIBLE_PORT_LOG_ERR1(fmt, arg...)   Log_error1(fmt, ##arg)
#define MIBLE_PORT_LOG_ERR2(fmt, arg...)   Log_error2(fmt, ##arg)
#define MIBLE_PORT_LOG_ERR3(fmt, arg...)   Log_error3(fmt, ##arg)
#define MIBLE_PORT_LOG_ERR4(fmt, arg...)   Log_error4(fmt, ##arg)
#define MIBLE_PORT_LOG_ERR5(fmt, arg...)   Log_error5(fmt, ##arg)

#else /* MIBLE_PORT_LOG */

#define MIBLE_PORT_DBG0(fmt, arg...)
#define MIBLE_PORT_DBG1(fmt, arg...)
#define MIBLE_PORT_DBG2(fmt, arg...)
#define MIBLE_PORT_DBG3(fmt, arg...)
#define MIBLE_PORT_DBG4(fmt, arg...)

#define MIBLE_PORT_LOG_INFO0(fmt, arg...)
#define MIBLE_PORT_LOG_INFO1(fmt, arg...)
#define MIBLE_PORT_LOG_INFO2(fmt, arg...)
#define MIBLE_PORT_LOG_INFO3(fmt, arg...)
#define MIBLE_PORT_LOG_INFO4(fmt, arg...)
#define MIBLE_PORT_LOG_INFO5(fmt, arg...)

#define MIBLE_PORT_LOG_WAR0(fmt, arg...)
#define MIBLE_PORT_LOG_WAR1(fmt, arg...)
#define MIBLE_PORT_LOG_WAR2(fmt, arg...)
#define MIBLE_PORT_LOG_WAR3(fmt, arg...)
#define MIBLE_PORT_LOG_WAR4(fmt, arg...)
#define MIBLE_PORT_LOG_WAR5(fmt, arg...)

#define MIBLE_PORT_LOG_ERR0(fmt, arg...)
#define MIBLE_PORT_LOG_ERR1(fmt, arg...)
#define MIBLE_PORT_LOG_ERR2(fmt, arg...)
#define MIBLE_PORT_LOG_ERR3(fmt, arg...)
#define MIBLE_PORT_LOG_ERR4(fmt, arg...)
#define MIBLE_PORT_LOG_ERR5(fmt, arg...)

#endif /* MIBLE_PORT_LOG */

#ifdef MIBLE_PORT_DBG
#define MIBLE_PORT_DBG0(fmt, arg...)  Log_info1("[\x1b[34;1mDBG: %s\x1b[0m] "fmt, (uintptr_t)__FUNCTION__, ##arg)
#define MIBLE_PORT_DBG1(fmt, arg...)  Log_info2("[\x1b[34;1mDBG: %s\x1b[0m] "fmt, (uintptr_t)__FUNCTION__, ##arg)
#define MIBLE_PORT_DBG2(fmt, arg...)  Log_info3("[\x1b[34;1mDBG: %s\x1b[0m] "fmt, (uintptr_t)__FUNCTION__, ##arg)
#define MIBLE_PORT_DBG3(fmt, arg...)  Log_info4("[\x1b[34;1mDBG: %s\x1b[0m] "fmt, (uintptr_t)__FUNCTION__, ##arg)
#define MIBLE_PORT_DBG4(fmt, arg...)  Log_info5("[\x1b[34;1mDBG: %s\x1b[0m] "fmt, (uintptr_t)__FUNCTION__, ##arg)
#else // MIBLE_PORT_DBG
#define MIBLE_PORT_DBG0(fmt, arg...)
#define MIBLE_PORT_DBG1(fmt, arg...)
#define MIBLE_PORT_DBG2(fmt, arg...)
#define MIBLE_PORT_DBG3(fmt, arg...)
#define MIBLE_PORT_DBG4(fmt, arg...)
#endif // MIBLE_PORT_DBG

#ifdef HEAPMGR_METRICS
extern uint32_t heapmgrMemAlo;
#define MIBLE_PORT_LOG_HEAP() Log_info2("[\x1b[35;1mHEAP Used: %s\x1b[0m] %d", (uintptr_t)__FUNCTION__, (uintptr_t) heapmgrMemAlo);
#define MIBLE_PORT_LOG_HEAP_START() uint32_t heapStart = (uintptr_t) heapmgrMemAlo;
#define MIBLE_PORT_LOG_HEAP_END() \
{ \
    extern void ICall_heapGetStats(ICall_heapStats_t *stats); \
    ICall_heapStats_t stats; \
    ICall_heapGetStats(&stats); \
    Log_info4("[\x1b[35;1mHEAP Allocated in function %s\x1b[0m] %d,  Heap Left: %d / %d, ", (uintptr_t)__FUNCTION__, (uintptr_t) heapmgrMemAlo - heapStart, stats.totalFreeSize, stats.totalSize); \
}
#else /* HEAPMGR_METRICS */
#define MIBLE_PORT_LOG_HEAP()
#define MIBLE_PORT_LOG_HEAP_START()
#define MIBLE_PORT_LOG_HEAP_END()
#endif /* HEAPMGR_METRICS */

#ifndef LINKDB_CONNHANDLE_INVALID
#define LINKDB_CONNHANDLE_INVALID CONNHANDLE_INVALID
#endif // LINKDB_CONNHANDLE_INVALID



typedef enum {
    MI_APP_PORT_EVT_ADV_REPORT = 1,      // ADV_IND
    MI_APP_PORT_EVT_CONNECTED,           // ADV_SCAN_IND
    MI_APP_PORT_EVT_DISCONNECTED,        // ADV_NONCONN_INC
    MI_APP_PORT_EVT_CONN_PARAM_UPDATED,  // ADV_NONCONN_INC
    MI_APP_PORT_EVT_TIMER,               // Timer Event
} miapi_port_gap_event_t;

typedef struct mible_port_cb_s
{
	/** @brief initialization of the MIBle porting layer
	 *
	 *  This API needs to be call as soom as the TI BLE stack has initialize.
	 *  this is known when the event GAP_DEVICE_INIT_DONE_EVENT is received
	 *
	 *  @param sync   Object to syncrhnozie the application (Sysbios Event) 
     *  @param taskId iCall Task ID to get indication confirmation event
     *  @param enqueueFunc Function to enqueue event to be process in Application context.
	 */
	void (*init)(Event_Handle sync, uint8_t taskId);

	/** @brief Function to forward GAP event to the MIBle porting layer
	 *
	 *  This API will forward all GAP event received by the application from the stack.
	 *  This function is called in the application thread context.
	 *
	 *  @param event GAP event to process
	 *  @param *data pointer to data associated to the event.
	 */
	void (*stackEvent)(uint8_t event, void * data);

	/** @brief Tell the MiBLE portinglayer it can execute any pending task.
	 *
	 *  This is being call by teh application with low priority.
	 *  This function is called in the application thread context.
	 *
	 *  no parameters.
	 */
	void (*taskExec)(void);
}mible_port_cb_t;


void miapi_port_initialization(struct mible_port_cb_s **cb);

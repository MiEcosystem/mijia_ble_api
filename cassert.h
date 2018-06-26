

#ifndef __CASSERT_H__
#define __CASSERT_H__



#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ke_mem.h"
#include "uart.h"
#include "arch_console.h"



int COMPrintf_hexdump(uint8_t *parr,uint8_t len);

//如果使用同步打印
#ifdef CFG_SYNC_PRINTF
		int COMPrintf(const char* fmt, ...);
		#undef arch_printf
		int arch_printf(const char *fmt, ...);
#else		//异步打印或者不打印
		#ifndef CFG_PRINTF
				#undef arch_printf
		#endif
		#define COMPrintf    arch_printf
		//不打印方式(由于lib库中默认的是调用 arch_printf ，所以arch_printf必须以函数的方式实现)
		#ifndef CFG_PRINTF_UART2
				int arch_printf(const char *fmt, ...);
		#endif
		
#endif





#endif


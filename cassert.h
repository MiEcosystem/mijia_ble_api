

#ifndef __CASSERT_H__
#define __CASSERT_H__



#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ke_mem.h"
#include "uart.h"
#include "arch_console.h"



int COMPrintf_hexdump(uint8_t *parr,uint8_t len);

#ifdef CFG_SYNC_PRINTF
int COMPrintf(const char* fmt, ...);
#else
#define COMPrintf    arch_printf
#endif





#endif


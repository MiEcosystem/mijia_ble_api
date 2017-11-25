

#ifndef __CASSERT_H__
#define __CASSERT_H__



#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include "ke_mem.h"
#include "uart.h"




#ifdef CFG_SYNC_PRINTF
int COMPrintf(const char* fmt, ...);
int COMPrintf_hexdump(uint8_t *parr,uint8_t len);
#else
#define COMPrintf(fmt, args...) {}
#define COMPrintf_hexdump(parr,len) {}
#endif





#endif


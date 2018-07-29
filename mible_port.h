#ifndef MIBLE_PORT_H__
#define MIBLE_PORT_H__

// Copyright [2017] [Beijing Xiaomi Mobile Software Co., Ltd]
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <stdbool.h>
#include <stdint.h>

#ifndef NULL
#define NULL 0
#endif

#if defined(__CC_ARM)
#pragma anon_unions
#elif defined(__ICCARM__)
#pragma language = extended
#elif defined(__GNUC__)
/* anonymous unions are enabled by default */
#endif

#if defined ( __CC_ARM )

    #ifndef __ASM
        #define __ASM               __asm
    #endif

    #ifndef __INLINE
        #define __INLINE            __inline
    #endif

    #ifndef __WEAK
        #define __WEAK              __weak
    #endif

    #ifndef __ALIGN
        #define __ALIGN(n)          __align(n)
    #endif

    #ifndef __PACKED
        #define __PACKED            __packed
    #endif

    #define GET_SP()                __current_sp()

#elif defined ( __ICCARM__ )

    #ifndef __ASM
        #define __ASM               __asm
    #endif

    #ifndef __INLINE
        #define __INLINE            inline
    #endif

    #ifndef __WEAK
        #define __WEAK              __weak
    #endif

    #ifndef __ALIGN
        #define STRING_PRAGMA(x) _Pragma(#x)
        #define __ALIGN(n) STRING_PRAGMA(data_alignment = n)
    #endif

    #ifndef __PACKED
        #define __PACKED            __packed
    #endif
    
    #define GET_SP()                __get_SP()

#elif defined   ( __GNUC__ )

    #ifndef __ASM
        #define __ASM               __asm
    #endif

    #ifndef __INLINE
        #define __INLINE            inline
    #endif

    #ifndef __WEAK
        #define __WEAK              __attribute__((weak))
    #endif

    #ifndef __ALIGN
        #define __ALIGN(n)          __attribute__((aligned(n)))
    #endif

    #ifndef __PACKED
        #define __PACKED           __attribute__((packed)) 
    #endif

    #define GET_SP()                gcc_current_sp()

    static inline unsigned int gcc_current_sp(void)
    {
        register unsigned sp __ASM("sp");
        return sp;
    }

#endif

#include "em_device.h"
#include "em_core.h"


#define CRITICAL_SECTION_ENTER()		  \
	{                                     \
		CORE_DECLARE_IRQ_STATE;           \
		CORE_ENTER_ATOMIC();
#define CRITICAL_SECTION_EXIT()			  \
		CORE_EXIT_ATOMIC();               \
	}

#include "third_party/SEGGER_RTT/SEGGER_RTT.h"
#define MI_PRINTF(...)     SEGGER_RTT_printf(0, __VA_ARGS__)
#define MI_HEXDUMP(array_base, array_size)                                         \
	do {                                                                           \
        for (int i = 0; i<(array_size); i++)                                       \
            SEGGER_RTT_printf(0, (i+1)%16?"%02X ":"%02X\n", ((char*)(array_base))[i]);\
		if (array_size%16) SEGGER_RTT_printf(0,"\n");                               \
	} while(0)


#ifdef DEBUG
#include "em_gpio.h"
#define TRACE_INIT(x)   GPIO_PinModeSet(gpioPortA, 1, gpioModeWiredAndPullUp, 0)
#define TRACE_ENTER(x)  GPIO_PinOutSet(gpioPortA, 1)
#define TRACE_EXIT(x)   GPIO_PinOutClear(gpioPortA, 1)
#else
#define TRACE_INIT(x)
#define TRACE_ENTER(x)
#define TRACE_EXIT(x)
#endif
#endif // MIBLE_PORT_H__

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
#include <ti/sysbios/hal/Hwi.h>
#include <ti/sysbios/knl/Swi.h>

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

#elif defined( __GNUC__ )

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
#elif (defined __TI_COMPILER_VERSION__)

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

    #define GET_SP()               gcc_current_sp()

    static inline unsigned int gcc_current_sp(void)
    {
        void* p = NULL;
        return((int)&p);
    }
#endif

#define CRITICAL_SECTION_ENTER()    uint_least16_t swikey  = (uint_least16_t) Swi_disable(); \
                                    uint_least16_t hwikey = (uint_least16_t) Hwi_disable(); 

#define CRITICAL_SECTION_EXIT()    Hwi_restore((UInt) hwikey); \
                                   Swi_restore((UInt) swikey);


//#define MI_LOG_ENABLED

#ifdef MI_LOG_ENABLED 
#include <UartLog.h>  // Comment out if using xdc Log
    // Precompiler define for CCS to get only filename: uartlog_FILE="\"${InputFileName}\""
    // IAR project should have extra option --no_path_in_file_macros to only have base file name.
    #if !defined(uartlog_FILE)
    #  define uartlog_FILE __FILE__
    #endif
#define MI_LOG_PRINTF(...) UartLog_log(uartlog_FILE, __LINE__, LEVEL_INFO, __VA_ARGS__);  // UARTLOG_ENABLE Need to be enable at project level.
#define MI_LOG_HEXDUMP(...)            
#else
#define MI_LOG_PRINTF(...)
#define MI_LOG_HEXDUMP(...)
#endif

//#include "cmsis_compiler.h"

#endif // MIBLE_PORT_H__

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
#include "platform_diagnose.h"
#include "platform_os.h"

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

#define CRITICAL_SECTION_ENTER()    plt_critical_enter()
#define CRITICAL_SECTION_EXIT()     plt_critical_exit(0)

#define MI_LOG_ENABLED

#ifdef MI_LOG_ENABLED
#define MI_PRINTF(...)                   LOG_PRINT(MM_ID, LEVEL_ERROR, __VA_ARGS__)
#define MI_HEXDUMP(base_addr, bytes)     LOG_DUMP(MM_ID, LEVEL_ERROR, base_addr, bytes)
#else
#define MI_PRINTF(...)
#define MI_HEXDUMP(base_addr, bytes)
#endif

#define TRACE_INIT(pin)
#define TRACE_ENTER(pin)
#define TRACE_EXIT(pin)

#endif // MIBLE_PORT_H__

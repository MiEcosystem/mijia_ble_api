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

#if defined(__CC_ARM)
#pragma anon_unions
#elif defined(__ICCARM__)
#pragma language = extended
#elif defined(__GNUC__)
/* anonymous unions are enabled by default */
#endif

typedef bool BOOLEAN;

#ifdef MI_LOG_ENABLED

/* 
  Log error   level    :1
  Log warning level    :2
  Log info    level    :3
  Log debug   level    :4
*/

#define MI_LOG_LEVEL              4
#define MI_LOG_COLORS_ENABLE      1
#define MI_LOG_PRINTF             printf

#include "mible_log_internal.h"

#define MI_LOG_ERROR(...)                     MI_LOG_INTERNAL_ERROR(__VA_ARGS__)
#define MI_LOG_WARNING(...)                   MI_LOG_INTERNAL_WARNING( __VA_ARGS__)
#define MI_LOG_INFO(...)                      MI_LOG_INTERNAL_INFO( __VA_ARGS__)
#define MI_LOG_DEBUG(...)                     MI_LOG_INTERNAL_DEBUG( __VA_ARGS__)
#else
#define MI_LOG_ERROR(...)
#define MI_LOG_WARNING(...)
#define MI_LOG_INFO(...)
#define MI_LOG_DEBUG(...)
#endif // MI_LOG_ENABLED

#endif

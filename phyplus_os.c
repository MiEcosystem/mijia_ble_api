/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_os.c
* @brief     xiaomi ble os api
* @details   OS data types and functions.
* @author    hector_huang
* @date      2018-1-4
* @version   v1.0
* *********************************************************************************************************
*/
#include "mible_type.h"

#include "platform_types.h"
#define MI_LOG_MODULE_NAME "PPLUS_OS"
#include "mible_log.h"

#define MAX_TIMER_CONTEXT       15


typedef struct
{
    bool used;
    int  id;  /*0~14*/
    mible_timer_mode    mode;
    mible_timer_handler timeout_handler;
    void *pcontext;
} timer_context_t;

static timer_context_t timer_context[MAX_TIMER_CONTEXT];


static uint8_t timer_app_task_id = 0;
void timer_app_init(uint8 task_id)
{
  int i;
  timer_app_task_id = task_id;
  memset(&timer_context[0], 0 , sizeof(timer_context));
  for(i = 0; i< MAX_TIMER_CONTEXT; i++)
  {
    timer_context[i].id = BIT(i);
  }
}

uint16 timer_app_process_event(uint8 task_id, uint16 events)
{
  int i = 0;
  for(i = 0; i< 15; i++){
    if (events & BIT(i))
    {
      if(timer_context[i].used)
        timer_context[i].timeout_handler(timer_context[i].pcontext);
      return (events ^ BIT(i));
    }
  }
}


mible_status_t mible_timer_create(void **p_timer_id,
                                  mible_timer_handler timeout_handler, mible_timer_mode mode)
{
  if (NULL == p_timer_id)
  {
    return MI_ERR_INVALID_PARAM;
  }

  uint8_t idx;
  for (idx = 0; idx < MAX_TIMER_CONTEXT; ++idx)
  {
    if (timer_context[idx].used == false)
    {
          break;
    }
  }

  if (idx >= MAX_TIMER_CONTEXT)
  {
      return MI_ERR_NO_MEM;
  }

  *p_timer_id = (void *)(&(timer_context[idx]));
  
  timer_context[idx].timeout_handler = timeout_handler;
  timer_context[idx].mode = mode;
  timer_context[idx].used = true;

  return MI_SUCCESS;
}

mible_status_t mible_user_timer_create(void **p_timer_id,
                    mible_timer_handler timeout_handler, mible_timer_mode mode)
{
    return mible_timer_create(p_timer_id, timeout_handler, mode);
}

mible_status_t mible_timer_delete(void *timer_id)
{
  timer_context_t* ptmctx = (timer_context_t*)(timer_id);
  
  if (ptmctx == NULL)
  {
    return MI_ERR_INVALID_PARAM;
  }

  if(ptmctx->used == false)
  {
    return MI_ERR_INVALID_STATE;
  }

  osal_stop_timerEx(timer_app_task_id, ptmctx->id);
  ptmctx->mode = 0;
  ptmctx->used = false;
  ptmctx->timeout_handler = NULL;
  ptmctx->pcontext = NULL;

  return MI_SUCCESS;
}

mible_status_t mible_timer_start(void *timer_id, uint32_t timeout_value,
                                 void *p_context)
{
  timer_context_t* ptmctx = (timer_context_t*)(timer_id);
  
  if (ptmctx == NULL)
  {
    return MI_ERR_INVALID_PARAM;
  }

  if(ptmctx->used == false)
  {
    return MI_ERR_INVALID_STATE;
  }

  if(ptmctx->mode == MIBLE_TIMER_SINGLE_SHOT)
  {
    osal_start_reload_timer(timer_app_task_id, ptmctx->id, timeout_value);
  }
  else
  {
    osal_start_timerEx(timer_app_task_id, ptmctx->id, timeout_value);
  }

  return MI_SUCCESS;
}

mible_status_t mible_timer_stop(void *timer_id)
{
  timer_context_t* ptmctx = (timer_context_t*)(timer_id);

  if (ptmctx == NULL)
  {
    return MI_ERR_INVALID_PARAM;
  }

  if(ptmctx->used == false)
  {
    return MI_ERR_INVALID_STATE;
  }
  osal_stop_timerEx(timer_app_task_id, ptmctx->id);
  
  return MI_SUCCESS;
}


mible_status_t mible_systime_utc_set(uint32_t utc_time)
{
    return MI_SUCCESS;
}

uint32_t mible_systime_utc_get(void)
{
    return 0;
}

mible_status_t mible_systime_timer_stop(void)
{
    return MI_SUCCESS;
}

uint64_t mible_mesh_get_exact_systicks(void)
{
    return (uint64_t)0;
}

/*
 * mible_mcu.c
 *
 *  Created on: 2020Äê10ÔÂ26ÈÕ
 *      Author: mi
 */
#include <string.h>
#include "bg_types.h"
#include "mible_mcu.h"
#include "mible_log.h"

/* add your own code here*/
#define MCU_INIT_VERSION    "0001"
#define MCU_UPDATE_VERSION  "0004"
int read_cnt = 0;
PT_THREAD(mible_mcu_get_info(pt_t * pt, uint8_t *buf))
{
    PT_BEGIN(pt);

    read_cnt++;
    if(read_cnt==1){
        memcpy(buf,MCU_INIT_VERSION,strlen(MCU_INIT_VERSION));
    }else if(read_cnt==2){
        goto EXIT;
    }else{
        memcpy(buf,MCU_UPDATE_VERSION,strlen(MCU_UPDATE_VERSION));
    }

    PT_END(pt);

EXIT:
    PT_EXIT(pt);
}

PT_THREAD(mible_mcu_read_dfuinfo(pt_t * pt, uint8_t* m_dfu_info, uint8_t len))
{
    PT_BEGIN(pt);
    /* add your own code here*/
    MI_LOG_ERROR("mible_mcu_read_dfuinfo fail !!!\n");
    goto EXIT;

    PT_END(pt);

EXIT:
    PT_EXIT(pt);
}

PT_THREAD(mible_mcu_write_dfuinfo(pt_t * pt, uint8_t* m_dfu_info, uint8_t len))
{
    PT_BEGIN(pt);
    /* add your own code here*/
    MI_LOG_ERROR("mible_mcu_write_dfuinfo succ !!!\n");

    PT_END(pt);

EXIT:
    PT_EXIT(pt);
}

PT_THREAD(mible_mcu_upgrade_firmware(pt_t * pt))
{
    PT_BEGIN(pt);
    /* add your own code here*/
    MI_LOG_ERROR("mible_mcu_upgrade_firmware succ !!!\n");
    //MI_LOG_ERROR("mible_mcu_upgrade_firmware fail !!!\n");
    //goto EXIT;

    PT_END(pt);

EXIT:
    PT_EXIT(pt);
}

PT_THREAD(mible_mcu_verify_firmware(pt_t * pt))
{
    PT_BEGIN(pt);
    /* add your own code here*/
    MI_LOG_DEBUG("mible_mcu_verify_firmware succ !!!\n");

    PT_END(pt);

EXIT:
    PT_EXIT(pt);
}

PT_THREAD(mible_mcu_nvm_write(pt_t * pt, void * p_data, uint32_t length, uint32_t address))
{
    PT_BEGIN(pt);
    /* add your own code here*/
    MI_LOG_DEBUG("mible_mcu_nvm_write addr %08x, len %d\n", address, length);

    PT_END(pt);

EXIT:
    PT_EXIT(pt);
}

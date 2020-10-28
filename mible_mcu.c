/*
 * mible_mcu.c
 *
 *  Created on: 2020Äê10ÔÂ26ÈÕ
 *      Author: mi
 */
#include "mible_mcu.h"

__WEAK PT_THREAD(mible_mcu_get_info(pt_t * pt, uint8_t *buf))
{
    PT_BEGIN(pt);
    goto EXIT;
    PT_END(pt);
EXIT:
    PT_EXIT(pt);
}

__WEAK PT_THREAD(mible_mcu_read_dfuinfo(pt_t * pt, uint8_t* m_dfu_info, uint8_t len))
{
    PT_BEGIN(pt);
    goto EXIT;
    PT_END(pt);
EXIT:
    PT_EXIT(pt);
}

__WEAK PT_THREAD(mible_mcu_write_dfuinfo(pt_t * pt, uint8_t* m_dfu_info, uint8_t len))
{
    PT_BEGIN(pt);
    goto EXIT;
    PT_END(pt);
EXIT:
    PT_EXIT(pt);
}

__WEAK PT_THREAD(mible_mcu_upgrade_firmware(pt_t * pt))
{
    PT_BEGIN(pt);
    goto EXIT;
    PT_END(pt);
EXIT:
    PT_EXIT(pt);
}

__WEAK PT_THREAD(mible_mcu_verify_firmware(pt_t * pt))
{
    PT_BEGIN(pt);
    goto EXIT;
    PT_END(pt);
EXIT:
    PT_EXIT(pt);
}

__WEAK PT_THREAD(mible_mcu_nvm_write(pt_t * pt, void * p_data, uint32_t length, uint32_t address))
{
    PT_BEGIN(pt);
    goto EXIT;
    PT_END(pt);
EXIT:
    PT_EXIT(pt);
}

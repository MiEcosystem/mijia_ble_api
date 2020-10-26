/*
 * mible_mcu.h
 *
 *  Created on: 2020Äê10ÔÂ26ÈÕ
 *      Author: mi
 */

#ifndef MIJIA_BLE_API_MIBLE_MCU_H_
#define MIJIA_BLE_API_MIBLE_MCU_H_

#include "mible_port.h"
#include "mible_type.h"
#include "third_party/pt/pt.h"

PT_THREAD(mible_mcu_get_info(pt_t * pt, uint8_t *buf));

PT_THREAD(mible_mcu_read_dfuinfo(pt_t * pt, uint8_t* m_dfu_info, uint8_t len));

PT_THREAD(mible_mcu_write_dfuinfo(pt_t * pt, uint8_t* m_dfu_info, uint8_t len));

PT_THREAD(mible_mcu_upgrade_firmware(pt_t* pt));

PT_THREAD(mible_mcu_nvm_write(pt_t * pt, void * p_data, uint32_t length, uint32_t address));

PT_THREAD(mible_mcu_verify_firmware(pt_t * pt));

#endif /* MIJIA_BLE_API_MIBLE_MCU_H_ */

/**
*********************************************************************************************************
*               Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file     rtk_i2c.h
* @brief
* @details
* @author   astor
* @date     2020-02-11
* @version  v1.0
*********************************************************************************************************
*/

#ifndef __IO_I2C_H
#define __IO_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "rtl876x_i2c.h"
#include "rtl876x_nvic.h"
#include "rtl876x_pinmux.h"
#include "rtl876x_rcc.h"
#include "trace.h"
#include "board.h"
#include "mible_api.h"
#include "app_msg.h"

#define PERIPHERAL_I2C_ADDRESS  0x50

/* GDMA channel for receive data */
#define GDMA_CHANNEL_REV            GDMA_Channel1
#define GDMA_CHANNEL_REV_NUM        1
#define GDMA_CHANNEL_REV_IRQ        GDMA0_CHANNEL1_IRQ
#define GDMAReceiveIntrHandler      Gdma0Ch1IntrHandler
/* GDMA channel for send data */
#define GDMA_CHANNEL_SEND           GDMA_Channel0
#define GDMA_CHANNEL_SEND_NUM       0

#define I2C_TX_MAX                  512

typedef enum
{
    IO_MSG_I2C_RX                     = 1,
    IO_MSG_I2C_TX                     = 2,
} T_IO_MSG_I2C;

typedef enum
{
    IO_STAT_I2C_IDLE = 0,
    IO_STAT_I2C_BUSY = 1,
} T_IO_STAT_I2C;

void board_i2c_init(const iic_config_t *p_config);
void driver_i2c_init(const iic_config_t *p_config);

#ifdef __cplusplus
}
#endif

#endif


/**************************************************************************//**
* @file i2c_master_ldma.h
* @brief LDMA I2C Master Example
* @version 1.00
******************************************************************************
* # License
* <b>Copyright 2018 Silicon Labs, Inc. http://www.silabs.com</b>
*******************************************************************************
*
* This file is licensed under the Silabs License Agreement. See the file
* "Silabs_License_Agreement.txt" for details. Before using this software for
* any purpose, you must agree to the terms of that agreement.
*
******************************************************************************/

#ifndef I2C_MASTER_LDMA_H
#define I2C_MASTER_LDMA_H

#include "mible_api.h"

#define I2C_SELECT      0       // Device with two I2C, can be 0 or 1

// I2C peripheral corresponds to the I2C bus used on the hardware.
#if (I2C_COUNT == 1)
#define I2C_NUM         I2C0
#define I2C_CLK         cmuClock_I2C0
#define I2C_IRQ         I2C0_IRQn
#define I2C_ISR         I2C0_IRQHandler
#define I2C_DMA_TXREQ   ldmaPeripheralSignal_I2C0_TXBL
#define I2C_DMA_RXREQ   ldmaPeripheralSignal_I2C0_RXDATAV
#endif

#if (I2C_COUNT == 2)
#if (I2C_SELECT == 0)
#define I2C_NUM         I2C0
#define I2C_CLK         cmuClock_I2C0
#define I2C_IRQ         I2C0_IRQn
#define I2C_ISR         I2C0_IRQHandler
#define I2C_DMA_TXREQ   ldmaPeripheralSignal_I2C0_TXBL
#define I2C_DMA_RXREQ   ldmaPeripheralSignal_I2C0_RXDATAV
#else
#define I2C_NUM         I2C1
#define I2C_CLK         cmuClock_I2C1
#define I2C_IRQ         I2C1_IRQn
#define I2C_ISR         I2C1_IRQHandler
#define I2C_DMA_TXREQ   ldmaPeripheralSignal_I2C1_TXBL
#define I2C_DMA_RXREQ   ldmaPeripheralSignal_I2C1_RXDATAV
#endif
#endif

// GPIO for I2C SDA and SCL
#if defined(EFR32MG13P732F512GM48)      // EFR32MG13 WSTK
#define I2C_SCL_PORT    gpioPortC       // PC10 I2C0_SCL#14
#define I2C_SCL_PIN     10        
#define I2C_SCL_LOC     14
#define I2C_SDA_PORT    gpioPortC       // PC11 I2C0_SDA#16
#define I2C_SDA_PIN     11
#define I2C_SDA_LOC     16
#endif

#if defined(EFM32PG12B500F1024GL125)    // EFM32PG12 STK
#define I2C_SCL_PORT    gpioPortC       // PC11 I2C0_SCL#15
#define I2C_SCL_PIN     11
#define I2C_SCL_LOC     15
#define I2C_SDA_PORT    gpioPortC       // PC10 I2C0_SDA#15
#define I2C_SDA_PIN     10
#define I2C_SDA_LOC     15
#endif

#define SYNC_SET        0x80            // Bit for DMA synchronization
#define SYNC_CLEAR      0x80
#define MATCH_VALUE     0x80
#define DMA_CH_I2C_TX   0               // DMA channel for TX
#define DMA_CH_I2C_RX   1               // DMA channel for RX
#define TX_DESC_SIZE    4
#define RX_DESC_SIZE    6

#define I2C_SLAVE_ADDR  0x80            // I2C slave address
#define I2C_READ_BIT    0x01            // I2C address R/W bit
#define TX_BUFFER_SIZE  512
#define RX_BUFFER_SIZE  16

// Default I2C configuration
#define I2C_CONF_DEFAULT        \
{                               \
  I2C_SCL_PORT,                 \
  I2C_SCL_PIN,                  \
  I2C_SCL_LOC,                  \
  I2C_SDA_PORT,                 \
  I2C_SDA_PIN,                  \
  I2C_SDA_LOC,                  \
  IIC_100K                      \
}

typedef mible_timer_handler cbFunc;

void transferComplete(void *data);
void waitUntilTransferDone(void);
mible_status_t mible_iic_tx_rx(uint8_t addr, uint8_t *p_out, uint8_t outlen, uint8_t * p_in, uint16_t inlen);

#endif

/**
*********************************************************************************************************
*               Copyright(c) 2020, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file     rtk_i2c.c
* @brief
* @details
* @author   astor
* @date     2020-02-11
* @version  v1.0
*********************************************************************************************************
*/

/* Includes ------------------------------------------------------------------*/
#include "rtk_i2c.h"
#include "mible_api.h"
#include "app_task.h"
#include "rtl876x_gdma.h"
#include "mible_log.h"
#define I2C0_GDMA_Channel_Handler      GDMA0_Channel0_Handler
/* Globals ------------------------------------------------------------------*/
static uint8_t scl_pin, sda_pin;
static I2C_TypeDef *I2C_port;
static IRQn_Type I2C_int_channel;
static mible_handler_t i2c_callback_func;
uint16_t I2C_SendBuf[I2C_TX_MAX];
uint8_t *I2C_ReadBuf;
bool read = false;
/**
  * @brief  Initialization of pinmux settings and pad settings.
  * @param  No parameter.
  * @return void
*/
void board_i2c_init(const iic_config_t *p_config)
{
    scl_pin = p_config->scl_pin;
    sda_pin = p_config->sda_pin;
    Pad_Config(scl_pin, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    Pad_Config(sda_pin, PAD_PINMUX_MODE, PAD_IS_PWRON, PAD_PULL_UP, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    switch (p_config->scl_port)
    {
    case 1:
        {
            Pinmux_Config(scl_pin, I2C1_CLK);
            Pinmux_Config(sda_pin, I2C1_DAT);
            break;
        }
    default:
        {
            Pinmux_Config(scl_pin, I2C0_CLK);
            Pinmux_Config(sda_pin, I2C0_DAT);
            break;
        }
    }
}

void GDMA_SendInit(void)
{
    RCC_PeriphClockCmd(APBPeriph_GDMA, APBPeriph_GDMA_CLOCK, ENABLE);
    /* Initialize GDMA to send data */
    GDMA_InitTypeDef GDMA_InitStruct;
    GDMA_StructInit(&GDMA_InitStruct);
    GDMA_InitStruct.GDMA_ChannelNum          = 0;
    GDMA_InitStruct.GDMA_DIR                 = GDMA_DIR_MemoryToPeripheral;
    GDMA_InitStruct.GDMA_BufferSize          = 0;
    GDMA_InitStruct.GDMA_SourceInc           = DMA_SourceInc_Inc;
    GDMA_InitStruct.GDMA_DestinationInc      = DMA_DestinationInc_Fix;
    GDMA_InitStruct.GDMA_SourceDataSize      = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_DestinationDataSize = GDMA_DataSize_HalfWord;
    GDMA_InitStruct.GDMA_SourceMsize         = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_DestinationMsize    = GDMA_Msize_1;
    GDMA_InitStruct.GDMA_SourceAddr          = (uint32_t)NULL;
    GDMA_InitStruct.GDMA_DestinationAddr     = (uint32_t)(&(I2C_port->IC_DATA_CMD));
    GDMA_InitStruct.GDMA_DestHandshake       = GDMA_Handshake_I2C0_TX;
    GDMA_InitStruct.GDMA_ChannelPriority     = 0;
    GDMA_Init(GDMA_Channel0, &GDMA_InitStruct);
    
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel         = GDMA0_Channel0_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelCmd      = (FunctionalState)ENABLE;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_Init(&NVIC_InitStruct);

    GDMA_INTConfig(0, GDMA_INT_Transfer, ENABLE);
}


/**
  * @brief  Initialize i2c peripheral.
  * @param  No parameter.
  * @return void
  */
void driver_i2c_init(const iic_config_t *p_config)
{
    if (p_config->scl_port == 1)
    {
        RCC_PeriphClockCmd(APBPeriph_I2C1, APBPeriph_I2C1_CLOCK, ENABLE);
        I2C_port = I2C1;
        I2C_int_channel = I2C1_IRQn;
    }
    else
    {
        RCC_PeriphClockCmd(APBPeriph_I2C0, APBPeriph_I2C0_CLOCK, ENABLE);
        I2C_port = I2C0;
        I2C_int_channel = I2C0_IRQn;
    }
    I2C_InitTypeDef  I2C_InitStruct;
    I2C_StructInit(&I2C_InitStruct);

    if (p_config->freq == IIC_100K)
    {
        I2C_InitStruct.I2C_ClockSpeed    = 100000;
    }
    else if (p_config->freq == IIC_400K)
    {
        I2C_InitStruct.I2C_ClockSpeed    = 400000;
    }
    I2C_InitStruct.I2C_DeviveMode    = I2C_DeviveMode_Master;
    I2C_InitStruct.I2C_AddressMode   = I2C_AddressMode_7BIT;
    I2C_InitStruct.I2C_SlaveAddress  = PERIPHERAL_I2C_ADDRESS;
    I2C_InitStruct.I2C_Ack           = I2C_Ack_Enable;
    I2C_InitStruct.I2C_TxDmaEn       = ENABLE;
    I2C_InitStruct.I2C_RxThresholdLevel = 8;
    I2C_InitStruct.I2C_TxWaterlevel  = 23;
    I2C_Init(I2C_port, &I2C_InitStruct);

    I2C_ClearINTPendingBit(I2C_port, I2C_INT_RX_FULL);
    I2C_INTConfig(I2C0, I2C_INT_RX_FULL, ENABLE);
    I2C_ClearINTPendingBit(I2C_port, I2C_INT_STOP_DET);
    I2C_INTConfig(I2C_port, I2C_INT_STOP_DET, ENABLE);
    I2C_ClearINTPendingBit(I2C_port, I2C_INT_TX_ABRT);
    I2C_INTConfig(I2C_port, I2C_INT_TX_ABRT, ENABLE);
    /* Config I2C interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = I2C_int_channel;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStruct);
    GDMA_SendInit();
    I2C_Cmd(I2C_port, ENABLE);
}


void I2C_SendByGDMA(uint8_t *pWriteBuf, uint16_t Writelen, bool no_stop)
{
    I2C_ClearINTPendingBit(I2C_port, I2C_INT_STOP_DET);
    I2C_INTConfig(I2C_port, I2C_INT_STOP_DET, ENABLE);
    GDMA_SendInit();

    /* Configure send data store in send buffer */
    for (int i = 0; i < Writelen; i++)
    {
        I2C_SendBuf[i] = *pWriteBuf++;
    }
    I2C_SendBuf[Writelen - 1] = I2C_SendBuf[Writelen - 1] | (1 << 9);

    /* Configure I2C total size of data which want to send */
    GDMA_SetSourceAddress(GDMA_CHANNEL_SEND, (uint32_t)I2C_SendBuf);
    GDMA_SetBufferSize(GDMA_CHANNEL_SEND, Writelen);

    /* Start write */
    GDMA_Cmd(0, ENABLE);
}

void I2C_ReadByGDMA(uint8_t *pReadBuf, uint16_t ReadLen)
{
    I2C_ClearINTPendingBit(I2C_port, I2C_INT_STOP_DET);
    I2C_INTConfig(I2C_port, I2C_INT_STOP_DET, ENABLE);
    GDMA_SendInit();

    /* Configure send data store in send buffer */
    for (int i = 0; i < ReadLen - 1; i++)
    {
        I2C_SendBuf[i] = (0x0001 << 8);
    }
    I2C_SendBuf[ReadLen - 1] = BIT(8) | BIT(9);
    I2C_ReadBuf = pReadBuf;
    /* Configure I2C total size of data which want to send */
    GDMA_SetSourceAddress(GDMA_CHANNEL_SEND, (uint32_t)I2C_SendBuf);
    GDMA_SetBufferSize(GDMA_CHANNEL_SEND, ReadLen);
    /* Start write */
    GDMA_Cmd(0, ENABLE);
}


void DataTrans_I2C_ReceiveBuf(I2C_TypeDef *I2Cx, uint8_t *pBuf, uint16_t len)
{
    /* Check the parameters */
    assert_param(IS_I2C_ALL_PERIPH(I2Cx));

    while (len--)
    {
        *pBuf++ = (uint8_t)I2Cx->IC_DATA_CMD;
    }
}
bool xfer_abort = true;
uint16_t I2C_ReadBuf_offset = 0;
/**
  * @brief  I2C0 interrupt handle function.
  * @param  None.
  * @return None.
*/
void I2C0_Handler(void)
{
    iic_event_t event;
    if (I2C_GetINTStatus(I2C_port, I2C_INT_STOP_DET) == SET)
    {
        if (read) {
            uint32_t len = I2C_GetRxFIFOLen(I2C_port);
            DataTrans_I2C_ReceiveBuf(I2C0, I2C_ReadBuf + I2C_ReadBuf_offset, len);
            I2C_ReadBuf_offset = 0;
        }
        event = IIC_EVT_XFER_DONE;
        I2C_ClearINTPendingBit(I2C_port, I2C_INT_STOP_DET);
        i2c_callback_func(&event);
    }
    else if (I2C_GetINTStatus(I2C_port, I2C_INT_TX_ABRT) == SET)
    {
        I2C_INTConfig(I2C_port, I2C_INT_STOP_DET, DISABLE);
        if (I2C_CheckEvent(I2C_port, ABRT_7B_ADDR_NOACK)) {
            MI_LOG_ERROR("[IIC] ADDR_NACK");
            event = IIC_EVT_ADDRESS_NACK;
        } else {
            MI_LOG_ERROR("[IIC] DATA_NACK");
            event = IIC_EVT_DATA_NACK;
        }
        I2C_ClearINTPendingBit(I2C_port, I2C_INT_TX_ABRT);
        i2c_callback_func(&event);
    }
    else if (I2C_GetINTStatus(I2C_port, I2C_INT_RX_FULL) == SET)
    {
        uint32_t len = I2C_GetRxFIFOLen(I2C_port);
        if (len)
        {
            DataTrans_I2C_ReceiveBuf(I2C0, I2C_ReadBuf + I2C_ReadBuf_offset, len);
            I2C_ReadBuf_offset += len;
        }
        I2C_ClearINTPendingBit(I2C_port, I2C_INT_RX_FULL);
    }
    else
    {
        I2C_ClearAllINT(I2C_port);
    }
}

mible_status_t mible_iic_init(const iic_config_t *p_config, mible_handler_t handler)
{
    if (p_config != NULL && handler != NULL)
    {
        board_i2c_init(p_config);
        driver_i2c_init(p_config);
        i2c_callback_func = handler;
        return MI_SUCCESS;
    }
    else
    {
        return MI_ERR_INVALID_PARAM;
    }
}

void mible_iic_uninit(void)
{
    I2C_DeInit(I2C_port);
    Pinmux_Config(scl_pin, IDLE_MODE);
    Pinmux_Config(sda_pin, IDLE_MODE);
}

mible_status_t mible_iic_tx(uint8_t addr, uint8_t *p_out, uint16_t len,
                            bool no_stop)
{
    no_stop = false;
    I2C_SetSlaveAddress(I2C_port, addr);
    if (p_out == NULL || len > I2C_TX_MAX)
    {
        return MI_ERR_INVALID_PARAM;
    }
    if (I2C_GetFlagState(I2C_port, I2C_FLAG_ACTIVITY) == SET)
    {
        return MI_ERR_BUSY;
    }
    I2C_SendByGDMA(p_out, len, no_stop);
    read = false;
    return MI_SUCCESS;
}

mible_status_t mible_iic_rx(uint8_t addr, uint8_t *p_in, uint16_t len)
{
    if (I2C_GetFlagState(I2C_port, I2C_FLAG_ACTIVITY) == SET)
    {
        return MI_ERR_BUSY;
    }
    I2C_SetSlaveAddress(I2C_port, addr);
    if (p_in == NULL || len > I2C_TX_MAX)
    {
        return MI_ERR_INVALID_PARAM;
    }
    I2C_ReadByGDMA(p_in, len);
    read = true;
    return MI_SUCCESS;
}

int mible_iic_scl_pin_read(uint8_t port, uint8_t pin)
{
    if (I2C_GetFlagState(I2C_port, I2C_FLAG_ACTIVITY) == SET)
    {
        return IO_STAT_I2C_BUSY;
    }
    else
    {
        return IO_STAT_I2C_IDLE;
    }
}

void I2C0_GDMA_Channel_Handler(void)
{
    //Add user code here
    GDMA_ClearINTPendingBit(0, GDMA_INT_Transfer);
}

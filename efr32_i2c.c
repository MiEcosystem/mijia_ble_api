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

#include "mible_api.h"
#include "mible_port.h"
#include "mible_type.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_i2c.h"
#include "em_ldma.h"
#include "i2c_master_ldma.h"

static cbFunc i2cCallback;
static const iic_config_t *m_p_config;
uint8_t txData[TX_BUFFER_SIZE];         // TX buffer (half word align)
uint8_t rxData[RX_BUFFER_SIZE];         // RX buffer (half word align)
static volatile uint8_t slaveAddrW;     // Slave address write buffer
static volatile uint8_t slaveAddrR;     // Slave address read buffer
static volatile uint8_t ackCnt;         // ACK counter
static volatile I2C_TransferReturn_TypeDef transferResult;      // Transfer flag

static const LDMA_TransferCfg_t xferCfg = LDMA_TRANSFER_CFG_PERIPHERAL(0);
static const LDMA_Descriptor_t p2mLink = LDMA_DESCRIPTOR_LINKREL_P2M_BYTE(NULL, NULL, 1UL, 1UL);
static const LDMA_Descriptor_t m2pSingle = LDMA_DESCRIPTOR_SINGLE_M2P_BYTE(NULL, NULL, 1UL);
static const LDMA_Descriptor_t m2pLink = LDMA_DESCRIPTOR_LINKREL_M2P_BYTE(NULL, NULL, 1UL, 1UL);
static const LDMA_Descriptor_t wriSingle = LDMA_DESCRIPTOR_SINGLE_WRITE(NULL, NULL);
static const LDMA_Descriptor_t wriLink = LDMA_DESCRIPTOR_LINKREL_WRITE(NULL, NULL, 1UL);
static const LDMA_Descriptor_t syncLink = LDMA_DESCRIPTOR_LINKREL_SYNC(0, SYNC_CLEAR, MATCH_VALUE, MATCH_VALUE, 1);

static LDMA_Descriptor_t descLinkTx[TX_DESC_SIZE];      // Linked list for DMA TX transfer
static LDMA_Descriptor_t descLinkRx[RX_DESC_SIZE];      // Linked list for DMA RX transfer

/***************************************************************************//**
 * @brief
 *  Interrupt handler for LDMA.
 ******************************************************************************/
void LDMA_IRQHandler( void )
{
  uint32_t pending;
  iic_event_t transferStatus;   // Transfer status

  pending = LDMA->IFC;  // Read and clear interrupt source
  if ( pending & LDMA_IF_ERROR )
  {
    // DMA error during I2C transfer
    pending = (LDMA->STATUS & ~_LDMA_STATUS_CHERROR_MASK) >> _LDMA_STATUS_CHERROR_SHIFT;
    LDMA_StopTransfer (pending);
    
    transferStatus = IIC_EVT_ADDRESS_NACK;
    transferResult = i2cTransferSwFault;
    i2cCallback(&transferStatus);
  }
}

/*****************************************************************************
 * @brief  Handles various I2C events and errors
 * 
 * When a STOP condition has been successfully sent, the MSTOP
 * interrupt is triggered and the transfer is marked as complete. 
 * 
 * The three errors: ARBLOST, BUSERR, and NACK are handled here. 
 * In all cases, the current transfer is aborted, and the error
 * flag is set to inform the main loop that an error occured
 * during the transfer. 
 *****************************************************************************/
void I2C_ISR(void)
{
  uint32_t flags = I2C_NUM->IFC;        // Read and clear IF
  iic_event_t transferStatus;           // Transfer status

  if (flags & I2C_IF_ARBLOST)
  {
    transferStatus = IIC_EVT_ADDRESS_NACK;
    transferResult = i2cTransferArbLost;
    I2C_NUM->CMD = I2C_CMD_ABORT;
    i2cCallback(&transferStatus);
  }
  else if (flags & I2C_IF_BUSERR)
  {
    transferStatus = IIC_EVT_ADDRESS_NACK;
    transferResult = i2cTransferBusErr;
    I2C_NUM->CMD = I2C_CMD_ABORT;
    i2cCallback(&transferStatus);
  }
  else if (flags & I2C_IF_NACK)
  {
    transferStatus = IIC_EVT_ADDRESS_NACK;
    if (flags & I2C_IF_ACK)
    {
      transferStatus = IIC_EVT_DATA_NACK;
    }
    transferResult = i2cTransferNack;
    I2C_NUM->CMD = I2C_CMD_ABORT;
    i2cCallback(&transferStatus);
  }
  else if ( flags & I2C_IF_MSTOP )
  {
    // Stop condition has been sent. Transfer is complete.
    I2C_IntDisable(I2C_NUM, I2C_IEN_MSTOP + I2C_IEN_NACK);
    // Clear AUTOSE for I2C TX
    I2C_NUM->CTRL &= ~I2C_CTRL_AUTOSE;

    transferStatus = IIC_EVT_XFER_DONE;
    i2cCallback(&transferStatus);
  }
  else if ( flags & I2C_IF_ACK )
  {
    // Repeated START and DMA SYNC after receiving ACKs for I2C address & data
    if (--ackCnt == 0)
    {
      I2C_IntDisable(I2C_NUM, I2C_IEN_ACK);
      I2C_NUM->CMD = I2C_CMD_START;
      LDMA->SYNC = SYNC_SET;
    }
  }
}

/*****************************************************************************
 * @brief  Called when I2C transfer is complete
 *****************************************************************************/
void transferComplete(void *data)
{
  // Add your application code here
  if (*(iic_event_t *)data == IIC_EVT_XFER_DONE)
  {
    transferResult = i2cTransferDone;
  }
}

/*****************************************************************************
 * @brief  Wait until I2C transfer is done
 *****************************************************************************/
void waitUntilTransferDone(void)
{
  // Exit if transfer error or done
  while(transferResult == i2cTransferInProgress)
  {
    ;
  }  
}
         
/**
 *        IIC APIs
 */

/*
 * @brief 	Function for initializing the IIC driver instance.
 * @param 	[in] p_config: Pointer to the initial configuration.
 * 			[in] handler: Event handler provided by the user. 
 * @return 	MI_SUCCESS 				Initialized successfully.
 * 			MI_ERR_INVALID_PARAM    p_config or handler is a NULL pointer.
 * 				
 * */
mible_status_t mible_iic_init(iic_config_t * p_config, mible_handler_t handler)
{
  if ((p_config == NULL) || (handler == NULL))
  {
    return MI_ERR_INVALID_PARAM;
  }
  m_p_config = p_config;
  // Init DMA and I2C
  LDMA_Init_t init = LDMA_INIT_DEFAULT;
  LDMA_Init( &init );

  CMU_ClockEnable(I2C_CLK, true);
  CMU_ClockEnable(cmuClock_GPIO, true);

  GPIO_PinModeSet((GPIO_Port_TypeDef)(m_p_config->scl_port), m_p_config->scl_pin, gpioModeWiredAnd, 1);
  GPIO_PinModeSet((GPIO_Port_TypeDef)(m_p_config->sda_port), m_p_config->sda_pin, gpioModeWiredAnd, 1);
  I2C_NUM->ROUTEPEN  = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
  I2C_NUM->ROUTELOC0 = (m_p_config->sda_extra_conf << _I2C_ROUTELOC0_SDALOC_SHIFT)
                     | (m_p_config->scl_extra_conf << _I2C_ROUTELOC0_SCLLOC_SHIFT);
  
  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;
  if (p_config->freq == IIC_100K)
  {
    i2cInit.freq = I2C_FREQ_STANDARD_MAX;
  }
  else
  {
    i2cInit.freq = I2C_FREQ_FAST_MAX;
  }
  I2C_Init(I2C_NUM, &i2cInit);
  
  // Exit the BUSY state. The I2C will be in this state out of RESET.
  if (I2C_NUM->STATE & I2C_STATE_BUSY)
  {
    I2C_NUM->CMD = I2C_CMD_ABORT;
  }
  
  // Enable the Clock Low Timeout counter
  I2C_NUM->CTRL = (I2C_NUM->CTRL & ~_I2C_CTRL_CLTO_MASK) | I2C_CTRL_CLTO_160PCC;
  
  // Enable error interrupts
  I2C_IntEnable(I2C_NUM, I2C_IEN_ARBLOST + I2C_IEN_BUSERR);
  
  // Enable interrupts in NVIC
  NVIC_EnableIRQ(I2C_IRQ);
  return MI_SUCCESS;
}

/*
 * @brief 	Function for uninitializing the IIC driver instance.
 * 
 * 				
 * */
void mible_iic_uninit(void)
{
  NVIC_DisableIRQ(I2C_IRQ);
  GPIO_PinModeSet(m_p_config->scl_port, m_p_config->scl_pin, gpioModeDisabled, 0);
  GPIO_PinModeSet(m_p_config->sda_port, m_p_config->sda_pin, gpioModeDisabled, 0);

  I2C_NUM->CTRL = _I2C_CTRL_RESETVALUE;
  I2C_NUM->SADDR = _I2C_SADDR_RESETVALUE;
  I2C_NUM->SADDRMASK = _I2C_SADDRMASK_RESETVALUE;
  I2C_NUM->CLKDIV = _I2C_CLKDIV_RESETVALUE;
  I2C_NUM->IEN = _I2C_IEN_RESETVALUE;   
  I2C_NUM->IFC = _I2C_IFC_MASK;
  I2C_NUM->ROUTEPEN  = _I2C_ROUTEPEN_RESETVALUE;
  I2C_NUM->ROUTELOC0 = _I2C_ROUTELOC0_RESETVALUE;
  CMU_ClockEnable(I2C_CLK, false);
}

/*
 * @brief 	Function for sending data to a IIC slave.
 * @param 	[in] addr:   Address of a specific slave device (only 7 LSB).
 * 			[in] p_out:  Pointer to tx data
 * 			[in] len:    Data length
 * 			[in] no_stop: If set, the stop condition is not generated on the bus
 *          after the transfer has completed successfully (allowing for a repeated start in the next transfer).
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_out is not vaild address.
 * @note  	This function should be implemented in non-blocking mode.
 * 			When tx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument. 
 * */
mible_status_t mible_iic_tx(uint8_t addr, uint8_t * p_out, uint16_t len, bool no_stop)
{
  LDMA_TransferCfg_t xferTx;
  LDMA_Descriptor_t *descTx = &descLinkTx[0];
  
  if (transferResult == i2cTransferInProgress)
  {
    return MI_ERR_BUSY;
  }

  if (p_out == NULL)
  {
    return MI_ERR_INVALID_PARAM;
  }
  
  // Automatically generate a STOP condition when there is no more data to send.
  I2C_NUM->CTRL |= I2C_CTRL_AUTOSE;
  
  // Enable MSTOP and NACK interrupt
  I2C_IntEnable(I2C_NUM, I2C_IEN_MSTOP + I2C_IEN_NACK);

  // Clear all pending interrupts prior to starting transfer.
  I2C_NUM->IFC = _I2C_IFC_MASK;
  NVIC_ClearPendingIRQ(I2C_IRQ);

  // Set transfer is active.
  transferResult = i2cTransferInProgress;
  
  // Set peripheral transfer request
  xferTx = xferCfg;
  xferTx.ldmaReqSel = I2C_DMA_TXREQ;

  // LINK descriptors for DMA TX
  slaveAddrW = addr;
  *descTx = m2pLink;
  descTx->xfer.srcAddr = (uint32_t)(&slaveAddrW);
  descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDATA);
  
  if (len & 0x01)
  {
    if (len == 1)               // Only single byte to transfer
    {
      descTx++;
      *descTx = m2pSingle;
      descTx->xfer.srcAddr = (uint32_t)(p_out);
      descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDATA);
    }
    else
    {
      descTx++;                 // Length is odd
      *descTx = m2pLink;        
      descTx->xfer.srcAddr = (uint32_t)(p_out);
      descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDOUBLE);
      descTx->xfer.xferCnt = (len - 1)/2 - 1;
      descTx->xfer.ignoreSrec = 1;
      descTx->xfer.size = ldmaCtrlSizeHalf; 
      
      descTx++;
      *descTx = m2pSingle;
      descTx->xfer.srcAddr = (uint32_t)(p_out + len - 1);
      descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDATA);
    }
  }
  else
  {
    descTx++;                   // Length is even
    *descTx = m2pSingle;        
    descTx->xfer.srcAddr = (uint32_t)(p_out);
    descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDOUBLE);
    descTx->xfer.xferCnt = len/2 - 1;
    descTx->xfer.ignoreSrec = 1;
    descTx->xfer.size = ldmaCtrlSizeHalf; 
  }
  
  // Activate DMA TX
  LDMA_StartTransfer(DMA_CH_I2C_TX, (void*)&xferTx, (void*)&descLinkTx);
  I2C_NUM->CMD = I2C_CMD_START;

  // No interrupt for DMA TX channel, xfer.doneIfs don't care
  LDMA_IntDisable((1 << DMA_CH_I2C_TX));
  return MI_SUCCESS;
}

/*
 * @brief 	Function for reciving data to a IIC slave.
 * @param 	[in] addr:   Address of a specific slave device (only 7 LSB).
 * 			[out] p_in:  Pointer to rx data
 * 			[in] len:    Data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_in is not vaild address.
 * @note  	This function should be implemented in non-blocking mode.
 * 			When rx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument. 
 * */
mible_status_t mible_iic_rx(uint8_t addr, uint8_t * p_in, uint16_t len)
{
  LDMA_TransferCfg_t xferTx;
  LDMA_TransferCfg_t xferRx;
  LDMA_Descriptor_t *descTx = &descLinkTx[0];
  LDMA_Descriptor_t *descRx = &descLinkRx[0];

  if (transferResult == i2cTransferInProgress)
  {
    return MI_ERR_BUSY;
  }
  
  if (p_in == NULL)
  {
    return MI_ERR_INVALID_PARAM;
  }

  // Automatically ACK received bytes
  I2C_NUM->CTRL |= I2C_CTRL_AUTOACK;

  // Clear all pending interrupts prior to starting transfer.
  I2C_NUM->IFC = _I2C_IFC_MASK;
  NVIC_ClearPendingIRQ(I2C_IRQ);

  // Set transfer is active.
  transferResult = i2cTransferInProgress;
  
  // Set peripheral transfer request
  xferTx = xferCfg;
  xferTx.ldmaReqSel = I2C_DMA_TXREQ;

  xferRx = xferCfg;
  xferRx.ldmaReqSel = I2C_DMA_RXREQ;

  // LINK descriptor for DMA TX
  slaveAddrR = addr | I2C_READ_BIT;     // Slave address for read
  *descTx = m2pSingle;
  descTx->xfer.srcAddr = (uint32_t)(&slaveAddrR);
  descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDATA);
  
  // LINK descriptors for DMA RX
  if ((len - 1) & 0x01)
  {
    if (len == 2)               // Length - 1 is odd
    {
      *descRx = p2mLink;        // Length is 2 bytes
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
      descRx->xfer.dstAddr = (uint32_t)(p_in);
    }
    else
    {
      *descRx = p2mLink;        // Length > 2 bytes   
      descRx->xfer.xferCnt = (len - 2)/2 - 1;
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDOUBLE);
      descRx->xfer.dstAddr = (uint32_t)(p_in);
      descRx->xfer.ignoreSrec = 1;
      descRx->xfer.size = ldmaCtrlSizeHalf; 
      
      descRx++;
      *descRx = p2mLink;
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
      descRx->xfer.dstAddr = (uint32_t)(p_in + len - 2);
    }
  }
  else
  {
    if (len != 1)               // Length - 1 is even
    {
      *descRx = p2mLink;        // Lenght > 1 byte    
      descRx->xfer.xferCnt = (len - 1)/2 - 1;
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDOUBLE);
      descRx->xfer.dstAddr = (uint32_t)(p_in);
      descRx->xfer.ignoreSrec = 1;
      descRx->xfer.size = ldmaCtrlSizeHalf; 
    }
  }
  
  if (len == 1)               // Lenght = 1 byte = last byte
  {
    I2C_NUM->CTRL &= ~I2C_CTRL_AUTOACK;         // Clear AUTOACK
    I2C_IntEnable(I2C_NUM, I2C_IEN_MSTOP);      // Enable MSTOP

    *descRx = p2mLink;  
    descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
    descRx->xfer.dstAddr = (uint32_t)(p_in);
  }
  else
  {
    descRx++;           // Clear AUTOACK for last byte RX
    *descRx = wriLink;
    descRx->wri.immVal = I2C_CTRL_CLTO_160PCC + I2C_CTRL_EN;       
    descRx->wri.dstAddr = (uint32_t)(&I2C_NUM->CTRL);
    
    descRx++;           // Enable MSTOP
    *descRx = wriLink;
    descRx->wri.immVal = I2C_IEN_ARBLOST + I2C_IEN_BUSERR + I2C_IEN_MSTOP;       
    descRx->wri.dstAddr = (uint32_t)(&I2C_NUM->IEN);
    
    descRx++;           // RX last byte
    *descRx = p2mLink;
    descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
    descRx->xfer.dstAddr = (uint32_t)(p_in + len - 1);
  }
  
  descRx++;             // Send NACK & STOP
  *descRx = wriSingle;
  descRx->wri.immVal = I2C_CMD_NACK + I2C_CMD_STOP;       
  descRx->wri.dstAddr = (uint32_t)(&I2C_NUM->CMD);

  // Activate DMA TX and RX
  LDMA_StartTransfer(DMA_CH_I2C_TX, (void*)&xferTx, (void*)&descLinkTx);
  LDMA_StartTransfer(DMA_CH_I2C_RX, (void*)&xferRx, (void*)&descLinkRx);
  I2C_NUM->CMD = I2C_CMD_START;

  // No interrupt for DMA TX and RX channel, xfer.doneIfs don't care
  LDMA_IntDisable((1 << DMA_CH_I2C_TX) + (1 << DMA_CH_I2C_RX));
  return MI_SUCCESS;
}

/*
 * @brief 	Function for reciving data (repeated START) to a IIC slave.
 * @param 	[in] addr:   Address of a specific slave device (only 7 LSB).
 * 			[in] p_out:  Pointer to tx data
 * 			[in] outlen: Tx data length
 * 			[out] p_in:  Pointer to rx data
 * 			[in] inlen:  Rx data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_in is not vaild address.
 * @note  	This function should be implemented in non-blocking mode.
 * 			When rx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument. 
 * */
mible_status_t mible_iic_tx_rx(uint8_t addr, uint8_t *p_out, uint8_t outlen, uint8_t * p_in, uint16_t inlen)
{ 
  LDMA_TransferCfg_t xferTx;
  LDMA_TransferCfg_t xferRx;
  LDMA_Descriptor_t *descTx = &descLinkTx[0];
  LDMA_Descriptor_t *descRx = &descLinkRx[0];

  if (transferResult == i2cTransferInProgress)
  {
    return MI_ERR_BUSY;
  }
  
  if ((p_out == NULL) || (p_in == NULL))
  {
    return MI_ERR_INVALID_PARAM;
  }

  // Automatically ACK received bytes
  I2C_NUM->CTRL |= I2C_CTRL_AUTOACK;

  I2C_IntEnable(I2C_NUM, I2C_IEN_NACK);
  if (outlen != 0)
  {
    // Enable ACK interrupt to send repeat START for read operation
    ackCnt = outlen + 1;
    I2C_IntEnable(I2C_NUM, I2C_IEN_ACK);
  }

  // Clear all pending interrupts prior to starting transfer.
  I2C_NUM->IFC = _I2C_IFC_MASK;
  NVIC_ClearPendingIRQ(I2C_IRQ);

  // Set transfer is active.
  transferResult = i2cTransferInProgress;
  
  // Set peripheral transfer request
  xferTx = xferCfg;
  xferTx.ldmaReqSel = I2C_DMA_TXREQ;

  xferRx = xferCfg;
  xferRx.ldmaReqSel = I2C_DMA_RXREQ;

  // LINK descriptors for DMA TX
  if (outlen != 0)
  {
    slaveAddrW = addr;    // Slave address for write
    *descTx = m2pLink;
    descTx->xfer.srcAddr = (uint32_t)(&slaveAddrW);
    descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDATA);
    
    descTx++;
    *descTx = m2pLink;          
    descTx->xfer.srcAddr = (uint32_t)(p_out);
    descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDATA);
    descTx->xfer.xferCnt = outlen - 1;
    
    descTx++;             // Wait trigger from ACK
    *descTx = syncLink;
    
    descTx++;             
  }  
  *descTx = m2pSingle;
  slaveAddrR = addr | I2C_READ_BIT;     // Slave address for read
  descTx->xfer.srcAddr = (uint32_t)(&slaveAddrR);
  descTx->xfer.dstAddr = (uint32_t)(&I2C_NUM->TXDATA);
  
  // LINK descriptors for DMA RX
  if ((inlen - 1) & 0x01)
  {
    if (inlen == 2)             // Length - 1 is odd
    {
      *descRx = p2mLink;        // Length is 2 bytes
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
      descRx->xfer.dstAddr = (uint32_t)(p_in);
    }
    else
    {
      *descRx = p2mLink;        // Length > 2 bytes   
      descRx->xfer.xferCnt = (inlen - 2)/2 - 1;
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDOUBLE);
      descRx->xfer.dstAddr = (uint32_t)(p_in);
      descRx->xfer.ignoreSrec = 1;
      descRx->xfer.size = ldmaCtrlSizeHalf; 
      
      descRx++;
      *descRx = p2mLink;
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
      descRx->xfer.dstAddr = (uint32_t)(p_in + inlen - 2);
    }
  }
  else
  {
    if (inlen != 1)             // Length - 1 is even
    {
      *descRx = p2mLink;        // Length > 1 byte    
      descRx->xfer.xferCnt = (inlen - 1)/2 - 1;
      descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDOUBLE);
      descRx->xfer.dstAddr = (uint32_t)(p_in);
      descRx->xfer.ignoreSrec = 1;
      descRx->xfer.size = ldmaCtrlSizeHalf; 
    }
  }
  
  if (inlen == 1)               // Length = 1 byte = last byte
  {
    I2C_NUM->CTRL &= ~I2C_CTRL_AUTOACK;         // Clear AUTOACK
    I2C_IntEnable(I2C_NUM, I2C_IEN_MSTOP);      // Enable MSTOP

    *descRx = p2mLink;  
    descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
    descRx->xfer.dstAddr = (uint32_t)(p_in);
  }
  else
  {
    descRx++;           // Clear AUTOACK for last byte RX
    *descRx = wriLink;
    descRx->wri.immVal = I2C_CTRL_CLTO_160PCC + I2C_CTRL_EN;       
    descRx->wri.dstAddr = (uint32_t)(&I2C_NUM->CTRL);
    
    descRx++;           // Enable MSTOP
    *descRx = wriLink;
    descRx->wri.immVal = I2C_IEN_ARBLOST + I2C_IEN_BUSERR + I2C_IEN_MSTOP;       
    descRx->wri.dstAddr = (uint32_t)(&I2C_NUM->IEN);
    
    descRx++;           // RX last byte
    *descRx = p2mLink;
    descRx->xfer.srcAddr = (uint32_t)(&I2C_NUM->RXDATA);
    descRx->xfer.dstAddr = (uint32_t)(p_in + inlen - 1);
  }
  
  descRx++;             // Send NACK & STOP
  *descRx = wriSingle;
  descRx->wri.immVal = I2C_CMD_NACK + I2C_CMD_STOP;       
  descRx->wri.dstAddr = (uint32_t)(&I2C_NUM->CMD);

  // Activate DMA TX and RX
  LDMA_StartTransfer(DMA_CH_I2C_TX, (void*)&xferTx, (void*)&descLinkTx);
  LDMA_StartTransfer(DMA_CH_I2C_RX, (void*)&xferRx, (void*)&descLinkRx);
  I2C_NUM->CMD = I2C_CMD_START;

  // No interrupt for DMA TX and RX channel, xfer.doneIfs don't care
  LDMA_IntDisable((1 << DMA_CH_I2C_TX) + (1 << DMA_CH_I2C_RX));
  return MI_SUCCESS;
}

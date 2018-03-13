/**************************************************************************//**
* @file i2c_master_main.c
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

#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "i2c_master_ldma.h"

extern uint8_t txData[TX_BUFFER_SIZE];    
extern uint8_t rxData[RX_BUFFER_SIZE];   


int main2(void)
{
  uint32_t i;
  
  CHIP_Init();
  
#if defined(_EMU_DCDCCTRL_MASK)  
  // Init DCDC regulator if available
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;
  EMU_DCDCInit(&dcdcInit);
#endif

#if defined(MSC_CTRL_IFCREADCLEAR)  
  // Enable atomic read-clear operation on reading IFC register
  MSC->CTRL |= MSC_CTRL_IFCREADCLEAR;
#endif

  // Setup HFLCK
  CMU_HFRCOBandSet(cmuHFRCOFreq_38M0Hz);  
  
  // Set up I2C
//  i2cCallback = transferComplete;
//  iic_config_t i2cConfig = I2C_CONF_DEFAULT;
//  mible_iic_init(&i2cConfig, i2cCallback);

  // Enable Si7021 sensor isolation switch on STK for testing
#if defined(EFR32MG13P732F512GM48)
  GPIO_PinModeSet(gpioPortD, 15, gpioModePushPull, 1);
#endif

#if defined(EFM32PG12B500F1024GL125)
  GPIO_PinModeSet(gpioPortB, 10, gpioModePushPull, 1);
#endif
  
  // Wait sensor power up
  for (i=0; i<1000000; i++)
    ;
  
  txData[0] = 0xfa;             // Read electronic ID 1st byte
  txData[1] = 0x0f;
  mible_iic_tx_rx(I2C_SLAVE_ADDR, (uint8_t *)txData, 2, (uint8_t *)rxData, 8);
  waitUntilTransferDone();      // Wait until the I2C transfer is complete

  txData[0] = 0xfc;             // Read electronic ID 2nd byte
  txData[1] = 0xc9;
  mible_iic_tx_rx(I2C_SLAVE_ADDR, (uint8_t *)txData, 2, (uint8_t *)rxData, 6);
  waitUntilTransferDone();      // Wait until the I2C transfer is complete
  
  while (1)
    ;
}

/*		filename   uart2_sync.c*/
#include <stddef.h>     // standard definition
#include <stdint.h>
#include "uart.h"       // uart definition
#include "reg_uart.h"   // uart register


void uart2_send (unsigned char txdata)
{
    SetWord16(UART2_RBR_THR_DLL_REG,txdata);
}

void uart2_wait_end_of_tx(void)
{
    while (!(GetWord16(UART2_LSR_REG)&0x40));
}


void uart2_putc(unsigned char letter) 
{
    uart2_send (letter);
    uart2_wait_end_of_tx();
}

void uart2_write_str(uint8_t *bufptr)
{
		if(bufptr != NULL)
		{
				while(*bufptr)
					uart2_putc(*(bufptr++));
		}

}

void uart2_write_nbyte(uint8_t *bufptr,uint8_t len)
{
		uint8_t pos = 0;
		if(bufptr == NULL || len == 0)
			return;
		for(int i=0;i<len;i++)
		{
				uart2_putc(bufptr[pos++]);
		}
}

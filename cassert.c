#include "cassert.h"
#include "uart2_sync.h"
#include "app.h"



#define PRINT_BUF  256

int COMPrintf_hexdump(uint8_t *parr,uint8_t len)
{
		char my_buf[PRINT_BUF];
		if(len < ((PRINT_BUF-1) /5))
		{
				memset(my_buf,0,sizeof(my_buf));
				for(int i=0;i<len;i++)
				{
						if((i+1)!=len)
						    sprintf(my_buf+5*i,"0x%02x ",parr[i]);
						else
							sprintf(my_buf+5*i,"0x%02x \n",parr[i]);
				}

				COMPrintf(my_buf);
		}  
		return len;
}

#ifdef CFG_SYNC_PRINTF
//´®¿Ú´òÓ¡º¯Êý
static uint8_t printf_buffer[PRINT_BUF] = {0};
int COMPrintf(const char* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt );
	memset(printf_buffer,'\0',sizeof(printf_buffer));
	uint16_t len = vsprintf((char *)printf_buffer,fmt,ap);
	va_end( ap );
	

	uart2_write_str(printf_buffer);
	return len;   
}

int arch_printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt );
	memset(printf_buffer,'\0',sizeof(printf_buffer));
	uint16_t len = vsprintf((char *)printf_buffer,fmt,ap);
	va_end( ap );
	
	uart2_write_str(printf_buffer);
	return len; 
}

#endif


#ifndef CFG_PRINTF_UART2
#undef arch_printf
int arch_printf(const char *fmt, ...)
{
		return 0;
}
#endif

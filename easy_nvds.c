/*****************************************************************************
File Name:    easy_nvds.c
Description: 非易失性数据管理
History:
Date                Author                   Description
2017-11-16         Lucien                    Creat
****************************************************************************/
#include "spi_flash.h"
#include "mible_type.h"
#include "easy_nvds.h"

//最多支持16个记录   当
#define RECORD_ID_MAX_NUM		16
#define RECORD_CHUNK_SIZE		32

#define INVAILAD_ID         0xff

typedef struct _tag_db_info
{
		//每一位表示一个内存块是否使用了  bit = 0  可以分配  bit = 1  已被用
		uint32_t mem_allo_bit;	
		uint8_t available;
		uint8_t len[RECORD_ID_MAX_NUM];
		uint16_t record_id[RECORD_ID_MAX_NUM];
		uint32_t mem_used_bit[RECORD_ID_MAX_NUM];
		uint8_t  used[RECORD_ID_MAX_NUM];

}db_info_t;


//static uint8_t db_temp[RECORD_ID_MAX_NUM][RECORD_CHUNK_SIZE];

static db_info_t s_db_info;
void db_spi_flash_init(void)
{
    static int8_t dev_id;

    dev_id = spi_flash_enable(SPI_EN_GPIO_PORT, SPI_EN_GPIO_PIN);
    if (dev_id == SPI_FLASH_AUTO_DETECT_NOT_DETECTED)
    {
        // The device was not identified. The default parameters are used.
        // Alternatively, an error can be asserted here.
        spi_flash_init(SPI_FLASH_DEFAULT_SIZE, SPI_FLASH_DEFAULT_PAGE);
    }
}


static int8_t db_erase_flash_sectors(uint16_t erase_size)
{
    uint32_t sector_nb;
    uint32_t offset;
    int8_t ret;
    int i;

    // Calculate the starting sector offset
    offset = (APP_DB_DATA_OFFSET / SPI_SECTOR_SIZE) * SPI_SECTOR_SIZE;

    // Calculate the numbers of sectors to erase
    sector_nb = (erase_size / SPI_SECTOR_SIZE);
    if (erase_size % SPI_SECTOR_SIZE)
        sector_nb++;

    // Erase flash sectors
    for (i = 0; i < sector_nb; i++)
    {
        ret = spi_flash_block_erase(offset, SECTOR_ERASE);
        offset += SPI_SECTOR_SIZE;
        if (ret != ERR_OK)
            break;
    }
    return ret;
}


void db_clear_flash(void)
{
    int8_t ret;

    db_spi_flash_init();

    ret = db_erase_flash_sectors(RECORD_ID_MAX_NUM*RECORD_CHUNK_SIZE);
    ASSERT_WARNING(ret == ERR_OK);

    spi_release();
}



static void read_db_info(db_info_t *pinfo)
{
		if(pinfo != NULL)
		{
				db_spi_flash_init();
		    spi_flash_read_data((uint8_t*)(pinfo), APP_DB_INFO_OFFSET, sizeof(db_info_t));
		    spi_release();
		}
}

static void write_db_info(db_info_t *pinfo)
{
		if(pinfo != NULL)
		{
				db_spi_flash_init();
				spi_flash_block_erase(APP_DB_INFO_OFFSET, SECTOR_ERASE);
		    spi_flash_write_data((uint8_t*)(pinfo), APP_DB_INFO_OFFSET, sizeof(db_info_t));
		    spi_release();
		}
}

static void db_info_reset(db_info_t *pinfo)
{
		if(pinfo != NULL)
		{
				memset(pinfo,0,sizeof(db_info_t));
				pinfo->available = 0x55;
				write_db_info(pinfo);
		}
}


//申请FALSH  存储  申请成功  return true    
static bool alloc_flash_mem(uint8_t len,uint32_t *ret_mem_bit)
{
		bool ret = false;
		uint32_t mem_bit = s_db_info.mem_allo_bit;
		uint8_t need_bit = (len / RECORD_CHUNK_SIZE)+1;
		uint8_t count_bit = 0;
		int j = 0;
		for(j=0;(j<(sizeof(uint32_t)*8)) && (j<RECORD_ID_MAX_NUM);j++)
		{
				if(((1 << j) & mem_bit) == 0)
					count_bit++;
				else
					count_bit = 0;
				
				if(count_bit == need_bit)
				{
						break;
				}	
		}

		if(count_bit == need_bit)
		{
				for(int k=j;k>=0 && count_bit != 0;k--)
				{
					  *ret_mem_bit |= (1<<k);
						mem_bit |= (1 << k);
						count_bit--;
				}
				ret = true;
				s_db_info.mem_allo_bit |= mem_bit;
		}

		return ret;
}

mible_status_t easy_record_create(uint16_t record_id, uint8_t len)
{
		if(len == 0)
				return MI_ERR_INVALID_PARAM;

		//查找是否已经创建 如果已经创建返回成功
		for(int i=0;i<RECORD_ID_MAX_NUM;i++)
		{
				if(s_db_info.record_id[i] == record_id && s_db_info.used[i])
				{
						if(s_db_info.len[i] == len)
							return MI_SUCCESS;
						else
							return MI_ERR_INVALID_PARAM;
				}
		}

		//还没有创建此ID  申请创建
		for(int i=0;i<RECORD_ID_MAX_NUM;i++)
		{
				if(!s_db_info.used[i])
				{
						uint32_t ret_mem_bit = 0;
						if(alloc_flash_mem(len,&ret_mem_bit))
						{
								s_db_info.len[i] = len;
								s_db_info.mem_used_bit[i] = ret_mem_bit;
								s_db_info.record_id[i] = record_id;
								s_db_info.used[i] = true;
								write_db_info(&s_db_info);
								return MI_SUCCESS;
						}
						else
								return MI_ERR_NO_MEM;
				}
		}

		return MI_ERR_NO_MEM;
}


static void delet_record(uint8_t id_handle)
{
		if(s_db_info.used[id_handle] == true)
		{
				s_db_info.len[id_handle] = 0;
				s_db_info.mem_allo_bit &= (~s_db_info.mem_used_bit[id_handle]);
				s_db_info.mem_used_bit[id_handle] = 0;
				s_db_info.used[id_handle] = false;
				s_db_info.record_id[id_handle] = 0;
				write_db_info(&s_db_info);
		}
}

mible_status_t easy_record_delete(uint16_t record_id)
{
		mible_status_t ret =  MI_ERR_INVALID_PARAM;
		for(int i=0;i<RECORD_ID_MAX_NUM;i++)
		{
				if(s_db_info.record_id[i] == record_id)
				{
						delet_record(record_id);
						ret = MI_SUCCESS;
						break;
				}
		}
		return ret;
}


mible_status_t easy_record_read(uint16_t record_id, uint8_t* p_data,
    uint8_t len)
{
		uint8_t db_temp[RECORD_ID_MAX_NUM*RECORD_CHUNK_SIZE];
		db_spi_flash_init();
    spi_flash_read_data((uint8_t*)db_temp, APP_DB_DATA_OFFSET, sizeof(db_temp));
    spi_release();

		uint16_t pos_addr = 0;
		for(int i=0;i<RECORD_ID_MAX_NUM;i++)
		{
				if(s_db_info.record_id[i] == record_id && s_db_info.used[i] == true)
				{
						uint32_t use_bit = s_db_info.mem_used_bit[i];
						for(int j=0;j<RECORD_ID_MAX_NUM;j++)
						{
								if(((1<<j)&use_bit)>0)
								{
										pos_addr = (j*RECORD_CHUNK_SIZE);
										memcpy(p_data,(uint8_t*)(db_temp)+pos_addr,len);
										return MI_SUCCESS;
								}
						}
				}
		}

		return MI_ERR_INTERNAL;
}


mible_status_t easy_record_write(uint16_t record_id, uint8_t* p_data,
    uint8_t len)
{
		uint8_t db_temp[RECORD_ID_MAX_NUM*RECORD_CHUNK_SIZE];
		db_spi_flash_init();
    spi_flash_read_data(db_temp, APP_DB_DATA_OFFSET, sizeof(db_temp));
    spi_release();

		bool retb = false;
		for(int i=0;i<RECORD_ID_MAX_NUM && !retb;i++)
		{
				if(s_db_info.record_id[i] == record_id && s_db_info.used[i] == true)
				{
						uint32_t use_bit = s_db_info.mem_used_bit[i];
						for(int j=0;j<RECORD_ID_MAX_NUM && !retb;j++)
						{
								if(((1<<j)&use_bit)>0)
								{
										uint16_t pos_addr = 0;
										pos_addr = (j*RECORD_CHUNK_SIZE);
										memcpy((uint8_t*)(db_temp)+pos_addr,p_data,len);
								
										retb = true;
										break;
								}
						}
				}
		}

		if(retb)
		{
				int8_t ret;
		    db_spi_flash_init();

		    ret = db_erase_flash_sectors(RECORD_ID_MAX_NUM*RECORD_CHUNK_SIZE);
		    if (ret == ERR_OK)
		    {
		        spi_flash_write_data(db_temp, APP_DB_DATA_OFFSET, sizeof(db_temp));
		    }

		    spi_release();
				return MI_SUCCESS;
		}
		return MI_ERR_INVALID_PARAM;
}


bool check_db_info(db_info_t *pInfo)
 {
 		bool retb = true;
		if(pInfo != NULL)
		{
				if(pInfo->available != 0x55)
					return false;
				uint32_t bit_cout = 0;
				for(int i=0;i<RECORD_ID_MAX_NUM;i++)
				{
						if(pInfo->mem_used_bit[i])
						{
								bit_cout |= pInfo->mem_used_bit[i];
						}
				}
				if((bit_cout != pInfo->mem_allo_bit) || (pInfo->mem_allo_bit == 0xffffffff))  
					retb = false;
		}
		else
				retb = false;
		
		return retb;
}

void easy_nvds_init(void)
{
		read_db_info(&s_db_info);
		if(!check_db_info(&s_db_info))
		{
				db_info_reset(&s_db_info);
		}
}





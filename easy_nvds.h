/*****************************************************************************
File Name:    easy_nvds.h
Discription:  消息池管理
History:
Date                Author                   Description
2017-11-10         Lucien                    Creat
****************************************************************************/
#ifndef __EASY_NVDS_H__
#define __EASY_NVDS_H__

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mible_type.h"


/*
MEM_LOC_FOR_FW_1_IMG 		= "0x3000"
MEM_LOC_FOR_FW_2_IMG 		= "0x1b000"
MEM_LOC_FOR_PRODUCT_HEADER 	= "0x38000" 
*/

#define APP_DB_DATA_OFFSET     (0x38000-1*SPI_SECTOR_SIZE)

//数据记录信息
#define APP_DB_INFO_OFFSET		 (0x38000-2*SPI_SECTOR_SIZE)


mible_status_t easy_record_create(uint16_t record_id, uint8_t len);


mible_status_t easy_record_read(uint16_t record_id, uint8_t* p_data,
    uint8_t len);

mible_status_t easy_record_write(uint16_t record_id, uint8_t* p_data,
    uint8_t len);


mible_status_t easy_record_delete(uint16_t record_id);

void db_clear_flash(void);

void easy_nvds_init(void);

void test_db_info(void);


#endif

#ifndef MIBLE_WICED_H__
#define MIBLE_WICED_H__

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
#include "wiced_bt_trace.h"
#include "mible_server.h"
#include "mible_std_authen.h"
#include "mible_beacon.h"

#include <stdint.h>
#include <string.h> 
#include <stdint.h>

#define MIBLE_DID_RECORD_ID						0x01
#define MIBLE_TOKEN_RECORD_ID					0x02
#define MIBLE_BEACONKEY_RECORD_ID				0x03

#define MIBLE_GATT_HANDLE_MAX_NUM                         10
#define MIBLE_TOKEN_HANDLE_TRANSPORT_MAX_NUM              6
#define MIBLE_PRODUCT_ID_HANDLE_TRANSPORT_MAX_NUM         2
#define MIBLE_VERSION_HANDLE_TRANSPORT_MAX_NUM            10
#define MIBLE_AUTHENTICATION_HANDLE_TRANSPORT_MAX_NUM     4
#define MIBLE_DEVICE_ID_HANDLE_TRANSPORT_MAX_NUM          20
#define MIBLE_BEACON_KEY_HANDLE_TRANSPORT_MAX_NUM         12

#define MIBLE_ADV_DATA_LENGTH   4
#define MIBLE_TIMER_MAX_NUM     4
#define TASK_POST_TIMEOUT       10

typedef struct Mijia_Timer_db_
{
	wiced_timer_t        timer; 
	wiced_timer_callback_t  *pFunc;
	mible_timer_mode     mode;
	uint8_t              created;
}Mijia_Timer_db;
static Mijia_Timer_db gMiJia_TimerPool[MIBLE_TIMER_MAX_NUM];


wiced_timer_t mible_task_post_timer;

/*****************************************************************************
**  BLE CUSTOM UUID
*****************************************************************************/
//Miia gatt size
#define    UUID_TOKEN_SIZE               12
#define    UUID_PRODUCT_ID_SIZE          2
#define    UUID_VERSION_SIZE             20
#define    UUID_WIFI_CONFIG_SIZE         20
#define    UUID_AUTHENCATION_SIZE        4
#define    UUID_DEVICE_ID_SIZE           20
#define    UUID_BEACON_KEY_SIZE          12
#define    UUID_DEVICE_LIST_SIZE         20
#define    UUID_SECURITY_AUTH_SIZE       20

/* UUID value of the customize*/
#define UUID_SERVICE_SQUIRREL               0xFE95

enum ble_custom_uuid
{
    UUID_CHARACTERISTIC_TOKEN                               = 0x01,
    UUID_CHARACTERISTIC_PRODUCT_ID                          = 0x02,
    UUID_CHARACTERISTIC_VERSION                             = 0x04,
    UUID_CHARACTERISTIC_WIFICFG                	            = 0x05,
    UUID_CHARACTERISTIC_AUTHENTICATION                      = 0x10,
    UUID_CHARACTERISTIC_DEVICE_ID                           = 0x13,
    UUID_CHARACTERISTIC_BEACON_KEY                          = 0x14,
    UUID_CHARACTERISTIC_DEVICE                     	        = 0x15,
    UUID_CHARACTERISTIC_SECURE                 	            = 0x16,
};

enum
{
    SQUIRREL_IDX_SVC = 0x70,    // Please the value of the last item in hello_sensor_db_tags

    MI_IDX_TOKEN_CHAR, 
    MI_IDX_TOKEN_VAL,     
    MI_IDX_TOKEN_CFG,

    MI_IDX_PRODUCT_ID_CHAR,
    MI_IDX_PRODUCT_ID_VAL,

    MI_IDX_VERSION_ID_CHAR,
    MI_IDX_VERSION_ID_VAL, 
    MI_IDX_BEACON_KEY_CHAR, 
    MI_IDX_BEACON_KEY_VAL,

    MI_IDX_DEVICE_ID_CHAR,
    MI_IDX_DEVICE_ID_VAL,

    MI_IDX_AUTHENTICATION_CHAR, 
    MI_IDX_AUTHENTICATION_VAL,
    
	MI_IDX_WIFICFG_CHAR, 
    MI_IDX_WIFICFG_VAL, 
};

typedef struct Mijia_Param_
{
    uint8_t  ucDataLen; 
    uint8_t  ucWriteOverFlag;
    uint8_t  ucRsv[2];
    uint8_t  *pUcData;
}Mijia_Param;

typedef struct Mijia_Gatt_
{
    uint16_t  service_handle;
    uint16_t handle_uuid;
    uint8_t  conn_handle;
    uint16_t  handle_val;
    uint8_t  handle_max_len;
    uint8_t  offset;
    uint8_t  ucDataLen;
    uint8_t  *pUcData; 
}Mijia_Gatt;


static Mijia_Gatt  gMibleGatt_db[MIBLE_GATT_HANDLE_MAX_NUM] = {0};

//STRONG_BONDING, 0x3a2/930
//WEAK_BONDING,   0x9c/156

device_info dev_info = {
        .bonding = STRONG_BONDING,
        .pid = 930,
        .version = "0000",
};

wiced_bt_device_address_t  host_addr;




void advertising_init(void);

void advertising_start(void);

void mijia_gatt_db_init(void);

int mijia_gap_setup(uint8_t evt,void *ucStatus);

int mijia_gatt_read(uint16_t handle,uint8_t *readData,uint16_t *len,uint8_t offset);

int mijia_gatt_write(uint16_t conn_id, void *ucReq );

#endif

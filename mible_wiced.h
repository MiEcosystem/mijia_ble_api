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
#include "queue.h"
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
#define MAX_TASK_NUM            4



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


wiced_bt_device_address_t  host_addr;


typedef struct {
    mible_handler_t handler;
    void *arg;
} mible_task_t;

typedef struct Mijia_Timer_db_
{
	wiced_timer_t        timer; 
	wiced_timer_callback_t  *pFunc;
	mible_timer_mode     mode;
	uint8_t              created;
}Mijia_Timer_db;
static Mijia_Timer_db gMiJia_TimerPool[MIBLE_TIMER_MAX_NUM];


wiced_timer_t mible_task_post_timer;

queue_t task_queue;

mible_task_t task_buf[MAX_TASK_NUM];


void mible_task_post_handler( uint32_t count );

void mijia_gatt_db_init(void);

int mijia_gatt_write(uint16_t conn_id, void *ucReq );

int mijia_gatt_read(uint16_t handle,uint8_t *readData,uint16_t *len,uint8_t offset);

int mijia_gap_setup(uint8_t evt,void *ucStatus);

void advertising_init(void);

void advertising_start(void);

void std_authen_event_cb(mible_std_auth_evt_t evt,
        mible_std_auth_evt_param_t* param);


void mible_task_post_handler( uint32_t count )
{
	mible_task_t task;

	if (dequeue(&task_queue, &task) == MI_SUCCESS)
	{
	    //MI_LOG_INFO("\r\n****mible_task_post_handler: exectuing the post task\r\n");
		task.handler(task.arg);
	}
	else
	{
	   wiced_stop_timer( &mible_task_post_timer );
	   MI_LOG_INFO("mible_task_post_handler:stopped\r\n");
	}
}

void mijia_gatt_db_init(void)
{
	memset(gMiJia_TimerPool, 0, sizeof(Mijia_Timer_db) * MIBLE_TIMER_MAX_NUM);
	
	if(WICED_BT_SUCCESS != wiced_init_timer( &mible_task_post_timer, &mible_task_post_handler, 0, WICED_MILLI_SECONDS_TIMER ))
	{
		CY_LOG_INFO("BT task_post_timer inti: Fiailed!\r\n");
	}
	queue_init(&task_queue, task_buf, sizeof(task_buf)/sizeof(task_buf[0]), sizeof(task_buf[0]));

    // token
    gMibleGatt_db[0].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[0].handle_uuid=UUID_CHARACTERISTIC_TOKEN;
    gMibleGatt_db[0].conn_handle=MI_IDX_TOKEN_CHAR;
    gMibleGatt_db[0].handle_val=MI_IDX_TOKEN_VAL;
    gMibleGatt_db[0].handle_max_len=UUID_TOKEN_SIZE;//MIBLE_TOKEN_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[0].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[0].handle_max_len);
    memset(gMibleGatt_db[0].pUcData,'\0',gMibleGatt_db[0].handle_max_len);
    //PRODUCT ID
    gMibleGatt_db[1].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[1].handle_uuid= UUID_CHARACTERISTIC_PRODUCT_ID;
    gMibleGatt_db[1].conn_handle=MI_IDX_PRODUCT_ID_CHAR;
    gMibleGatt_db[1].handle_val=MI_IDX_PRODUCT_ID_VAL;
    gMibleGatt_db[1].handle_max_len=UUID_PRODUCT_ID_SIZE;//MIBLE_PRODUCT_ID_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[1].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[1].handle_max_len);
    memset(gMibleGatt_db[1].pUcData,'\0',gMibleGatt_db[1].handle_max_len);

    //version
    gMibleGatt_db[2].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[2].handle_uuid= UUID_CHARACTERISTIC_VERSION;
    gMibleGatt_db[2].conn_handle=MI_IDX_VERSION_ID_CHAR;
    gMibleGatt_db[2].handle_val=MI_IDX_VERSION_ID_VAL;
    gMibleGatt_db[2].handle_max_len=UUID_VERSION_SIZE;//MIBLE_VERSION_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[2].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[2].handle_max_len);
    memset(gMibleGatt_db[2].pUcData,'\0',gMibleGatt_db[2].handle_max_len);

	//WiFi config
	gMibleGatt_db[3].service_handle=SQUIRREL_IDX_SVC;
	gMibleGatt_db[3].handle_uuid= UUID_CHARACTERISTIC_WIFICFG;
	gMibleGatt_db[3].conn_handle=MI_IDX_WIFICFG_CHAR;
	gMibleGatt_db[3].handle_val=MI_IDX_WIFICFG_VAL;	 
	gMibleGatt_db[3].handle_max_len=UUID_WIFI_CONFIG_SIZE;//MIBLE_VERSION_HANDLE_TRANSPORT_MAX_NUM;
	gMibleGatt_db[3].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[3].handle_max_len);
	memset(gMibleGatt_db[3].pUcData,'\0',gMibleGatt_db[3].handle_max_len);

    //AUTHENTICATION
    gMibleGatt_db[4].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[4].handle_uuid= UUID_CHARACTERISTIC_AUTHENTICATION;
    gMibleGatt_db[4].conn_handle=MI_IDX_AUTHENTICATION_CHAR;
    gMibleGatt_db[4].handle_val=MI_IDX_AUTHENTICATION_VAL;
    gMibleGatt_db[4].handle_max_len=UUID_AUTHENCATION_SIZE;//MIBLE_AUTHENTICATION_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[4].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[4].handle_max_len);
    memset(gMibleGatt_db[4].pUcData,'\0',gMibleGatt_db[4].handle_max_len);

    //device id
    gMibleGatt_db[5].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[5].handle_uuid=UUID_CHARACTERISTIC_DEVICE_ID;
    gMibleGatt_db[5].conn_handle=MI_IDX_DEVICE_ID_CHAR;
    gMibleGatt_db[5].handle_val=MI_IDX_DEVICE_ID_VAL;
    gMibleGatt_db[5].handle_max_len=UUID_DEVICE_ID_SIZE;//MIBLE_DEVICE_ID_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[5].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[5].handle_max_len);
    memset(gMibleGatt_db[5].pUcData,'\0',gMibleGatt_db[5].handle_max_len);

  
	//beacon key
    gMibleGatt_db[6].service_handle=SQUIRREL_IDX_SVC;
    gMibleGatt_db[6].handle_uuid=UUID_CHARACTERISTIC_BEACON_KEY;
    gMibleGatt_db[6].conn_handle=MI_IDX_BEACON_KEY_CHAR;
    gMibleGatt_db[6].handle_val=MI_IDX_BEACON_KEY_VAL;
    gMibleGatt_db[6].handle_max_len=UUID_BEACON_KEY_SIZE;//MIBLE_BEACON_KEY_HANDLE_TRANSPORT_MAX_NUM;
    gMibleGatt_db[6].pUcData=(uint8_t*)wiced_memory_permanent_allocate(gMibleGatt_db[6].handle_max_len);
    memset(gMibleGatt_db[6].pUcData,'\0',gMibleGatt_db[6].handle_max_len);

    //MI_LOG_INFO("Gatts_Handle_Value_Init");
}

int mijia_gatt_write(uint16_t conn_id, void *ucReq )
{
    uint8_t i,j=0;
    mible_gatts_evt_param_t gattsParam={0};
    wiced_bt_gatt_write_t *p_req=(wiced_bt_gatt_write_t *)ucReq;
   
    CY_LOG_INFO("mijia_gatt_write len=%d, data=", p_req->val_len);
	CY_LOG_HEXDUMP(p_req->p_val,  p_req->val_len);

    if (p_req->val_len == 0)
    {
        MI_LOG_ERROR("mijia_gatt_write MI_ERR_INVALID_LENGTH1\r\n");
        return MI_ERR_INVALID_LENGTH;
    }

    for(i=0;i<MIBLE_GATT_HANDLE_MAX_NUM;i++)
    {
        if(p_req->handle == gMibleGatt_db[i].handle_val)
        {
			//MI_LOG_INFO("mijia_gatt_write: char handle");
            break;
        }
    }
	
    gMibleGatt_db[i].offset=p_req->offset;
    gMibleGatt_db[i].ucDataLen=p_req->val_len;
	gMibleGatt_db[i].handle_max_len=p_req->val_len;
	memcpy(gMibleGatt_db[i].pUcData, p_req->p_val, p_req->val_len);

	CY_LOG_INFO("Written gatt(XiaoMi):char uuid handle=0x%2x; length=%d; data=", p_req->handle, p_req->val_len);	
	CY_LOG_HEXDUMP(gMibleGatt_db[i].pUcData, gMibleGatt_db[i].ucDataLen);

	gattsParam.conn_handle=conn_id;
	gattsParam.write.value_handle=p_req->handle;
	gattsParam.write.data=p_req->p_val;
	gattsParam.write.len=p_req->val_len;
	gattsParam.write.offset=p_req->offset;

	mible_gatts_event_callback(MIBLE_GATTS_EVT_WRITE,&gattsParam);
	//mible_std_gatts_event_handler(MIBLE_GATTS_EVT_WRITE,&gattsParam);
	
	return MI_SUCCESS;
}

int mijia_gatt_read(uint16_t handle,uint8_t *readData,uint16_t *len,uint8_t offset)
{
    uint8_t i=0;
	
    for(i=0;i<MIBLE_GATT_HANDLE_MAX_NUM;i++)
    {
        if(handle==gMibleGatt_db[i].handle_val)
        {
            //MI_LOG_INFO("mijia_gatt_read: reading mijia char handle");
            break;
        }
    }
	if(i>=MIBLE_GATT_HANDLE_MAX_NUM)
	{
		
		MI_LOG_ERROR("Read gatt(XiaoMi) Not Found:len=%d, handle_val=0x%4x\r\n", *len, handle);
		return 0;
	}
	
    if((offset > gMibleGatt_db[i].handle_max_len))
    {
        MI_LOG_ERROR("mijia_gatt_read: MI_ERR_INVALID_LENGTH2\r\n");
        return MI_ERR_INVALID_LENGTH;
    }
    if(*len > gMibleGatt_db[i].handle_max_len)
    {
        *len = gMibleGatt_db[i].handle_max_len;
    }
    if((offset>0) && (offset <gMibleGatt_db[i].handle_max_len))
    {
        *len = (gMibleGatt_db[i].handle_max_len-offset);
    }
    memcpy(readData,gMibleGatt_db[i].pUcData+offset,*len);

	CY_LOG_INFO("Read gatt(XiaoMi):char uuid hand=0x%2x, offset=%d, length=%d, data=",handle, offset, *len);
	CY_LOG_HEXDUMP(readData,*len);

}

int mijia_gap_setup(uint8_t evt,void *ucStatus)
{
    mible_gap_evt_param_t gapParam;
    wiced_bt_gatt_connection_status_t *p_status=(wiced_bt_gatt_connection_status_t *)ucStatus;
    if(evt==MIBLE_GAP_EVT_CONNECTED)
    {
        gapParam.conn_handle=p_status->conn_id;
        memcpy(gapParam.connect.peer_addr,p_status->bd_addr,6);
        gapParam.connect.role=p_status->link_role;
        gapParam.connect.type=p_status->addr_type;
		
        mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED,&gapParam);
		//mible_std_gap_event_handler(MIBLE_GAP_EVT_CONNECTED,&gapParam);
   
    }
    else if(evt==MIBLE_GAP_EVT_DISCONNET)
    {
        gapParam.conn_handle=p_status->conn_id;
        if((p_status->reason>1) && (p_status->reason<5))
        {
            gapParam.disconnect.reason=(p_status->reason-1);
        }
		
        mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNET,&gapParam);
		//mible_std_gap_event_handler(MIBLE_GAP_EVT_CONNECTED,&gapParam);
    }
    else
    {
        ;
    }
}

void advertising_init(void)
{
    MI_LOG_INFO("\r\nadvertising init...\r\n");
     mibeacon_frame_ctrl_t frame_ctrl = {
    .time_protocol = 0,
    .is_encrypt = 0,
    .mac_include = 1,
    .cap_include = 1,
    .obj_include = 0,
    .bond_confirm = 0,
    .version = 0x03,
    };
    mibeacon_capability_t cap = {.connectable = 1,
                                .encryptable = 1,
                                .bondAbility = 1};
    mible_addr_t dev_mac;
    mible_gap_address_get(dev_mac);
    
    mibeacon_config_t mibeacon_cfg = {
    .frame_ctrl = frame_ctrl,
    .pid = PRODUCT_ID,
    .p_mac = (mible_addr_t*)dev_mac, 
    .p_capability = &cap,
    .p_obj = NULL,
    };
    
    uint8_t service_data[31];
	uint8_t service_data_len=0;
	
	if(MI_SUCCESS != mible_service_data_set(&mibeacon_cfg, service_data, &service_data_len)){
		MI_LOG_ERROR("\r\n mible_service_data_set: failed! \r\n");
		return;
	}

	uint8_t adv_data[23]={0};
	uint8_t adv_len=0;
	
	//add flags
	adv_data[0] = 0x02;
	adv_data[1] = 0x01;
	adv_data[2] = 0x06;
	
	memcpy(adv_data+3, service_data, service_data_len);
	adv_len = service_data_len + 3;
	
	mible_gap_adv_data_set(adv_data,adv_len,NULL,0);
	
	//MI_LOG_INFO("\r\n adv mi service data:");
	//MI_LOG_HEXDUMP(adv_data, adv_len);
	//MI_LOG_PRINTF("\r\n");
	return;
}

void advertising_start(void){
     mible_gap_adv_param_t adv_param =(mible_gap_adv_param_t){
    .adv_type = MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
    .adv_interval_min = 0x00a0,//MSEC_TO_UNITS(100, UNIT_0_625_MS),
    .adv_interval_max = 0x00b0,//MSEC_TO_UNITS(200, UNIT_0_625_MS),
    .ch_mask = {0},
    };
    if(MI_SUCCESS != mible_gap_adv_start(&adv_param)){
        MI_LOG_ERROR("mible_gap_adv_start failed. \r\n");
        return;
    }
   
}

void mible_service_init_cmp(void)
{
    MI_LOG_INFO("mible_service_init_cmp\r\n");
}

void mible_connected(void)
{
    MI_LOG_INFO("mible_connected \r\n");
}

void mible_disconnected(void)
{
    MI_LOG_INFO("mible_disconnected \r\n");
    advertising_start();
}

void mible_bonding_evt_callback(mible_bonding_state state)
{
    if(state == BONDING_FAIL){
        MI_LOG_INFO("BONDING_FAIL\r\n");
        mible_gap_disconnect(mible_server_connection_handle);
    }else if(state == BONDING_SUCC){
        MI_LOG_INFO("BONDING_SUCC\r\n");
    }else if(state == LOGIN_FAIL){
        MI_LOG_INFO("LOGIN_FAIL\r\n");
        mible_gap_disconnect(mible_server_connection_handle);
    }else{
        MI_LOG_INFO("LOGIN_SUCC\r\n");
    }
}
void std_authen_event_cb(mible_std_auth_evt_t evt,
        mible_std_auth_evt_param_t* p_param)
{
    switch(evt){
    case MIBLE_STD_AUTH_EVT_SERVICE_INIT_CMP:
        mible_service_init_cmp();
        break;
    case MIBLE_STD_AUTH_EVT_CONNECT:
        mible_gap_adv_stop();
        mible_connected();
        break;
    case MIBLE_STD_AUTH_EVT_DISCONNECT:
        mible_disconnected();
        break;
    case MIBLE_STD_AUTH_EVT_RESULT:
        mible_bonding_evt_callback(p_param->result.state);
        break;
    default:
        MI_LOG_ERROR("Unkown std authen event\r\n");
        break;
    }
}

#endif

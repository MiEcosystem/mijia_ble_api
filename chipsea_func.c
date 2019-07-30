#include "mible_beacon.h"
#include "mible_server.h"
#include "mible_log.h"
#include "chipsea_func.h"
#include "chipsea_fifo.h"
#include "osal.h"
#include "flash.h"
#include "gap.h"
#include <string.h>
#include "peripheral.h"
#include "mijiaService.h"
#include "ll.h"

uint8 mible_TaskID = 0;

static timer_item_t mible_timer_tbl[MIBLE_TIMER_NUM] =
    {
        {
            MIBLE_TIMER1_EVT,
            0,
            MIBLE_TIMER_SINGLE_SHOT,
            NULL,
            NULL,
        },
        {
            MIBLE_TIMER2_EVT,
            0,
            MIBLE_TIMER_SINGLE_SHOT,
            NULL,
            NULL,
        },
        {
            MIBLE_TIMER3_EVT,
            0,
            MIBLE_TIMER_SINGLE_SHOT,
            NULL,
            NULL,
        },
        {
            MIBLE_TIMER4_EVT,
            0,
            MIBLE_TIMER_SINGLE_SHOT,
            NULL,
            NULL,
        },
        {
            MIBLE_TIMER5_EVT,
            0,
            MIBLE_TIMER_SINGLE_SHOT,
            NULL,
            NULL,
        }};

mible_status_t timer_create(void **p_timer_id, mible_timer_handler timeout_handler, mible_timer_mode mode)
{
    if (p_timer_id == NULL)
    {
        return MI_ERR_INVALID_PARAM;
    }

    for (uint8_t index = 0; index < MIBLE_TIMER_NUM; index++)
    {
        if ((uint32_t *)&(mible_timer_tbl[index]) == (uint32_t *)(*((uint32_t **)p_timer_id)))
        {
            if (mible_timer_tbl[index].timeout != 0)
            {
                osal_stop_timerEx(mible_TaskID, mible_timer_tbl[index].id);
            }
            mible_timer_tbl[index].mode = mode;
            mible_timer_tbl[index].cb = timeout_handler;
            mible_timer_tbl[index].timeout = 0;
            mible_timer_tbl[index].para = NULL;
            *p_timer_id = (uint32_t *)&(mible_timer_tbl[index]);
            return MI_SUCCESS;
        }
    }

    for (uint8_t index = 0; index < MIBLE_TIMER_NUM; index++)
    {
        if (mible_timer_tbl[index].cb == NULL)
        {
            mible_timer_tbl[index].mode = mode;
            mible_timer_tbl[index].cb = timeout_handler;
            mible_timer_tbl[index].timeout = 0;
            mible_timer_tbl[index].para = NULL;
            *p_timer_id = (uint32_t *)&(mible_timer_tbl[index]);
            return MI_SUCCESS;
        }
    }
    *p_timer_id = NULL;
    return MI_ERR_NO_MEM;
}

mible_status_t timer_delete(void *timer_id)
{
    if (timer_id == NULL)
    {
        return MI_ERR_INVALID_PARAM;
    }
    for (uint8_t index = 0; index < MIBLE_TIMER_NUM; index++)
    {
        if ((uint32_t *)&(mible_timer_tbl[index]) == (uint32_t *)timer_id)
        {
            if (mible_timer_tbl[index].timeout != 0)
            {
                osal_stop_timerEx(mible_TaskID, mible_timer_tbl[index].id);
            }
            mible_timer_tbl[index].mode = MIBLE_TIMER_SINGLE_SHOT;
            mible_timer_tbl[index].cb = NULL;
            mible_timer_tbl[index].timeout = 0;
            mible_timer_tbl[index].para = NULL;
            return MI_SUCCESS;
        }
    }
    return MI_ERR_INVALID_PARAM;
}

mible_status_t timer_start(void *timer_id, uint32_t timeout_value, void *p_context)
{
    if (timer_id == NULL)
    {
        return MI_ERR_INVALID_PARAM;
    }
    for (uint8_t index = 0; index < MIBLE_TIMER_NUM; index++)
    {
        if ((uint32_t *)&(mible_timer_tbl[index]) == (uint32_t *)timer_id)
        {
            if (mible_timer_tbl[index].cb == NULL)
            {
                return MI_ERR_INVALID_PARAM;
            }
            mible_timer_tbl[index].timeout = timeout_value;
            mible_timer_tbl[index].para = p_context;
            osal_start_timerEx(mible_TaskID, mible_timer_tbl[index].id, mible_timer_tbl[index].timeout);
            return MI_SUCCESS;
        }
    }
    return MI_ERR_INVALID_PARAM;
}

mible_status_t timer_stop(void *timer_id)
{
    if (timer_id == NULL)
    {
        return MI_ERR_INVALID_PARAM;
    }
    for (uint8_t index = 0; index < MIBLE_TIMER_NUM; index++)
    {
        if ((uint32_t *)&(mible_timer_tbl[index]) == (uint32_t *)timer_id)
        {
            osal_stop_timerEx(mible_TaskID, mible_timer_tbl[index].id);
            mible_timer_tbl[index].timeout = 0;
            mible_timer_tbl[index].para = NULL;
            return MI_SUCCESS;
        }
    }
    return MI_ERR_INVALID_PARAM;
}

mible_status_t timer_execute(uint16_t id)
{
    for (uint8_t index = 0; index < MIBLE_TIMER_NUM; index++)
    {
        if (mible_timer_tbl[index].id == id)
        {
            mible_timer_tbl[index].cb(mible_timer_tbl[index].para);
            if (mible_timer_tbl[index].mode == MIBLE_TIMER_REPEATED)
            {
                osal_start_timerEx(mible_TaskID, mible_timer_tbl[index].id, mible_timer_tbl[index].timeout);
                return MI_SUCCESS;
            }
        }
    }
    return MI_ERR_INVALID_PARAM;
}

#define RECORD_FLASH_ADD 0x60000
#define RECORD_INIT_FLAG 0x5050

#define RECORD_ITEM_NUM 16
#define RECORD_ITEM_SIZE 32

typedef struct record_item
{
    uint16_t id;
    uint8_t len;
    uint8_t flag;
    uint16_t dat[RECORD_ITEM_SIZE];
} record_item_t;

typedef struct record_db
{
    uint16_t flag;
    record_item_t db[RECORD_ITEM_NUM];
} record_db_t;

record_db_t record_db;

static void record_db_read(void)
{
    HalFlashRead(RECORD_FLASH_ADD, (uint8_t *)&record_db, sizeof(record_db_t));
    if (record_db.flag != RECORD_INIT_FLAG)
    {
        memset(&record_db, 0, sizeof(record_db_t));
        record_db.flag = RECORD_INIT_FLAG;
        HalFlashErase(RECORD_FLASH_ADD >> 12);
        HalFlashWrite(RECORD_FLASH_ADD, (uint8_t *)&record_db, sizeof(record_db_t));
    }
}

mible_status_t record_create(uint16_t record_id, uint8_t len)
{
    if (len > RECORD_ITEM_SIZE)
    {
        MI_LOG_ERROR("%s(%d):record creat error\n", __func__, __LINE__);
        return MI_ERR_INVALID_PARAM;
    }
    for (int ii = 0; ii < RECORD_ITEM_NUM; ii++)
    {
        if (record_id == record_db.db[ii].id)
        {
            record_db.db[ii].len = len;
            record_db.db[ii].flag = 1;
            memset(record_db.db[ii].dat,0,sizeof(record_db.db[ii].dat));
            HalFlashErase(RECORD_FLASH_ADD >> 12);
            HalFlashWrite(RECORD_FLASH_ADD, (uint8_t *)&record_db, sizeof(record_db_t));
            return MI_SUCCESS;
        }
    }
    for (int ii = 0; ii < RECORD_ITEM_NUM; ii++)
    {
        if (0 == record_db.db[ii].flag)
        {
            record_db.db[ii].id = record_id;
            record_db.db[ii].len = len;
            record_db.db[ii].flag = 1;
            HalFlashErase(RECORD_FLASH_ADD >> 12);
            HalFlashWrite(RECORD_FLASH_ADD, (uint8_t *)&record_db, sizeof(record_db_t));
            return MI_SUCCESS;
        }
    }
    return MI_ERR_NO_MEM;
}

mible_status_t record_delete(uint16_t record_id)
{
    for (int ii = 0; ii < RECORD_ITEM_NUM; ii++)
    {
        if (record_id == record_db.db[ii].id)
        {
            memset(&(record_db.db[ii]), 0, sizeof(record_item_t));
            HalFlashErase(RECORD_FLASH_ADD >> 12);
            HalFlashWrite(RECORD_FLASH_ADD, (uint8_t *)&record_db, sizeof(record_db_t));
            return MI_SUCCESS;
        }
    }
    return MI_ERR_INVALID_PARAM;
}

mible_status_t record_read(uint16_t record_id, uint8_t *p_data, uint8_t len)
{
    for (int ii = 0; ii < RECORD_ITEM_NUM; ii++)
    {
        if (record_id == record_db.db[ii].id)
        {
            if (len != record_db.db[ii].len)
            {
                return MI_ERR_INVALID_PARAM;
            }
            memcpy(p_data, record_db.db[ii].dat, record_db.db[ii].len);
            return MI_SUCCESS;
        }
    }
		
    return MI_ERR_INVALID_PARAM;
}

mible_status_t record_write(uint16_t record_id, const uint8_t *p_data, uint8_t len)
{
    for (int ii = 0; ii < RECORD_ITEM_NUM; ii++)
    {
        if (record_id == record_db.db[ii].id)
        {
            if (len != record_db.db[ii].len)
            {
                return MI_ERR_INVALID_PARAM;
            }
            memcpy(record_db.db[ii].dat, p_data, record_db.db[ii].len);
            HalFlashErase(RECORD_FLASH_ADD >> 12);
            HalFlashWrite(RECORD_FLASH_ADD, (uint8_t *)&record_db, sizeof(record_db_t));
            return MI_SUCCESS;
        }
    }
		// no record_id founded, create first
		if (len > RECORD_ITEM_SIZE)
    {
        MI_LOG_ERROR("%s(%d):record creat error\n", __func__, __LINE__);
        return MI_ERR_INVALID_PARAM;
    }
		// find empty db, write
    for (int ii = 0; ii < RECORD_ITEM_NUM; ii++)
    {
        if (0 == record_db.db[ii].flag)
        {
            record_db.db[ii].id = record_id;
            record_db.db[ii].len = len;
            record_db.db[ii].flag = 1;
						memcpy(record_db.db[ii].dat, p_data, record_db.db[ii].len);
            HalFlashErase(RECORD_FLASH_ADD >> 12);
            HalFlashWrite(RECORD_FLASH_ADD, (uint8_t *)&record_db, sizeof(record_db_t));
            return MI_SUCCESS;
        }
    }
    return MI_ERR_INVALID_PARAM;
}

#define TASK_MAX  10
UserFifo_t  task_fifo;
FifoItem_t  task_buf[TASK_MAX];

mible_status_t task_post(mible_handler_t handler, void *arg)
{
    if(handler == NULL){
        return MI_ERR_INVALID_PARAM;
    }
    FifoItem_t item;
    item.func = handler;
    item.arg  = arg;
    if(userFifoPutItem(&task_fifo,&item) == 0){
        osal_set_event(mible_TaskID,MIBLE_TASK_EVT);
        return MI_SUCCESS;
    }
    else{
        return MI_ERR_NO_MEM;
    }
}

void tasks_exec(void)
{
    while(1){
        if(userFifoItemLen(&task_fifo) != 0){
			FifoItem_t item;
            userFifoGetItem(&task_fifo,&item);
            item.func(item.arg);
        }
        else{
            break;
        }
    }
}

void gap_evt_dispatch(mible_gap_evt_t evt)
{
	mible_gap_evt_param_t gap_params = {0};
    GAPRole_GetParameter(GAPROLE_CONNHANDLE,&(gap_params.conn_handle));
	switch ( evt )
    {
        case MIBLE_GAP_EVT_CONNECTED:{
            uint8_t addType = ADDRTYPE_PUBLIC;
            GAPRole_GetParameter(GAPROLE_CONN_BD_ADDR_TYPE,&addType);
            gap_params.connect.type = (addType == ADDRTYPE_PUBLIC)?MIBLE_ADDRESS_TYPE_PUBLIC:MIBLE_ADDRESS_TYPE_RANDOM;
            GAPRole_GetParameter(GAPROLE_CONN_BD_ADDR,gap_params.connect.peer_addr);
            GAPRole_GetParameter(GAPROLE_CONN_INTERVAL,(void*)&gap_params.connect.conn_param.max_conn_interval);
            GAPRole_GetParameter(GAPROLE_CONN_INTERVAL,(void*)&gap_params.connect.conn_param.min_conn_interval);
            GAPRole_GetParameter(GAPROLE_CONN_LATENCY,(void*)&gap_params.connect.conn_param.slave_latency);
            GAPRole_GetParameter(GAPROLE_CONN_TIMEOUT,(void*)&gap_params.connect.conn_param.conn_sup_timeout);
            uint8_t gapRole_profileRole = GAP_PROFILE_PERIPHERAL;
            GAPRole_GetParameter(GAPROLE_PROFILEROLE,(void*)&gapRole_profileRole);
            gap_params.connect.role = (gapRole_profileRole == GAP_PROFILE_PERIPHERAL)? MIBLE_GAP_PERIPHERAL : MIBLE_GAP_CENTRAL;
		}break;

        case MIBLE_GAP_EVT_DISCONNECT:{
            uint8_t disconnectReason = 0;
            GAPRole_GetParameter(GAPROLE_DISCONN_REASON,&disconnectReason);
            MI_LOG_INFO("%s(%d):disconnectReason = %d\r\n",__func__,__LINE__,disconnectReason);
            if(disconnectReason == LL_HOST_REQUESTED_TERM){
                gap_params.disconnect.reason = LOCAL_HOST_TERMINATED;
            }
            else if(disconnectReason == LL_PEER_REQUESTED_TERM){
                gap_params.disconnect.reason = REMOTE_USER_TERMINATED;
            }
            else{
                gap_params.disconnect.reason = CONNECTION_TIMEOUT;
            }
		}break;

        case MIBLE_GAP_EVT_CONN_PARAM_UPDATED:{
            GAPRole_GetParameter(GAPROLE_CONN_INTERVAL,(void*)&gap_params.connect.conn_param.max_conn_interval);
            GAPRole_GetParameter(GAPROLE_CONN_INTERVAL,(void*)&gap_params.connect.conn_param.min_conn_interval);
            GAPRole_GetParameter(GAPROLE_CONN_LATENCY,(void*)&gap_params.connect.conn_param.slave_latency);
            GAPRole_GetParameter(GAPROLE_CONN_TIMEOUT,(void*)&gap_params.connect.conn_param.conn_sup_timeout);
		}break;

        case MIBLE_GAP_EVT_ADV_REPORT:

            break;
	}
    mible_gap_event_callback(evt, &gap_params) ;
}

void gatts_evt_dispatch(mible_gatts_evt_t evt,uint8 param_id,uint8 len)
{
    static uint8_t dat_buf[30] = {0};

	mible_gatts_evt_param_t gatts_params = {0};
    GAPRole_GetParameter(GAPROLE_CONNHANDLE,&(gatts_params.conn_handle));

	switch( evt )
    {
        case MIBLE_GATTS_EVT_WRITE:
        {
            memset(dat_buf,0,sizeof(dat_buf));
            MijiaProfile_GetParameter(param_id,(void*)dat_buf);

            MijiaProfile_GetInfByID(param_id,&(gatts_params.write.len),&(gatts_params.write.value_handle));
            gatts_params.write.len = len;
            gatts_params.write.offset = 0;
            gatts_params.write.data = dat_buf;
            break;
        }
	}
    mible_gatts_event_callback(evt, &gatts_params) ;
}

device_info dev_info = {
    .bonding = WEAK_BONDING, // can be modified according to product
    .pid = 0x9c,             // product id, can be modified according to product
    .version = "0000",       // can be modified according to product
};

mible_status_t mible_get_advertising_data(uint8 *datBuf, uint8 *datLen)
{
    mibeacon_frame_ctrl_t frame_ctrl = {
        .is_encrypt = 0,
        .mac_include = 1,
        .cap_include = 1,
        .obj_include = 0,
        .bond_confirm = 0,
        .version = 0x03,
    };
    mibeacon_capability_t cap = {
        .connectable = 1,
        .encryptable = 1,
        .bondAbility = 1,
    };

    mible_addr_t dev_mac;
    mible_gap_address_get(dev_mac);

    mibeacon_config_t mibeacon_cfg = {
        .frame_ctrl = frame_ctrl,
        .pid = dev_info.pid,
        .p_mac = (mible_addr_t *)dev_mac,
        .p_capability = &cap,
        .p_obj = NULL,
    };

    if (MI_SUCCESS != mible_service_data_set(&mibeacon_cfg, datBuf + 3, datLen))
    {
        MI_LOG_ERROR("mible_service_data_set failed. \r\n");
        return MI_ERR_INTERNAL;
    }

    // add flags
    datBuf[0] = 0x02;
    datBuf[1] = 0x01;
    datBuf[2] = 0x06;
    *datLen += 3;

    MI_LOG_INFO("adv mi service data:");
    MI_LOG_HEXDUMP(datBuf, *datLen);
    MI_LOG_PRINTF("\r\n");
    return MI_SUCCESS;
}

void mible_service_init_cmp(void)
{
	MI_LOG_INFO("%s(%d):mible_service_init_cmp\r\n",__func__,__LINE__);
}

void mible_connected(void)
{
	MI_LOG_INFO("%s(%d):mible_connected \r\n",__func__,__LINE__);
}

void mible_disconnected(void)
{
	MI_LOG_INFO("%s(%d):mible_disconnected \r\n",__func__,__LINE__);
}

void mible_bonding_evt_callback(mible_bonding_state state)
{
	if(state == BONDING_FAIL){
	    MI_LOG_INFO("%s(%d):BONDING_FAIL \r\n",__func__,__LINE__);
		mible_gap_disconnect(mible_server_connection_handle);
	}else if(state == BONDING_SUCC){
		MI_LOG_INFO("%s(%d):BONDING_SUCC \r\n",__func__,__LINE__);
	}else if(state == LOGIN_FAIL){
		MI_LOG_INFO("%s(%d):LOGIN_FAIL \r\n",__func__,__LINE__);
		mible_gap_disconnect(mible_server_connection_handle);
	}else{
		MI_LOG_INFO("%s(%d):LOGIN_SUCC \r\n",__func__,__LINE__);
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
            mible_connected();
            break;
        case MIBLE_STD_AUTH_EVT_DISCONNECT:
            mible_disconnected();
            break;
        case MIBLE_STD_AUTH_EVT_RESULT:
            mible_bonding_evt_callback(p_param->result.state);
            break;
        default:
	        MI_LOG_ERROR("%s(%d):Unkown std authen event \r\n",__func__,__LINE__);
            break;
	}
}

void mible_services_init(uint8 taskId)
{
    mible_TaskID = taskId;
    record_db_read();
    userFifoInit(&task_fifo,task_buf,TASK_MAX);

		mible_std_auth_evt_register(std_authen_event_cb);
		mible_server_info_init(&dev_info, MODE_STANDARD);
    mible_server_miservice_init();
}

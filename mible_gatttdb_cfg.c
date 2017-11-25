/*****************************************************************************
File Name:    mible_gatttdb_cfg.c
Discription:  GATT属性表  配置文件  测试使用
History:
Date                Author                   Description
2017-11-13         Lucien                    Creat
****************************************************************************/
#include "mible_gatttdb_cfg.h"
#include "mible_api.h"
#include "arch_console.h"

#include "mible_log.h"
#include "arch_console.h"


/*app variable*/
device_info dev_info = {
		.bonding = STRONG_BONDING,
		.pid = 156,
		.version = "4321",
	};


/////Lucien debug
mible_gatts_char_db_t s_char_db[MIJIA_IDX_NB];
mible_gatts_srv_db_t s_srv_db[1];
mible_gatts_db_t s_server_db;

uint8_t token_buf[BLE_MIJIA_TOKEN_DATA_LEN];
uint8_t pid_buf[BLE_MIJIA_PRODUCT_ID_LEN]={0x55,0x55};
uint8_t version_buf[BLE_MIJIA_VERSION_LEN];
uint8_t wifi_cfg_buf[BLE_MIJIA_WIFI_CFG_LEN];
mible_status_t mible_server_miservice_init(void)
{
		arch_printf("mible_server_miservice_init\r\n");
		mible_gatts_srv_db_t *p_mi_service = &s_srv_db[0];//(mible_gatts_srv_db_t*)malloc(sizeof(mible_gatts_srv_db_t)); 
		mible_gatts_char_db_t *p_mi_char = &s_char_db[0];//(mible_gatts_char_db_t *)malloc(MIBLE_STD_CHAR_NUM*sizeof(mible_gatts_char_db_t));

		s_server_db.p_srv_db = p_mi_service;
		s_server_db.srv_num =  1;


// service init 
	  s_server_db.p_srv_db[0] = (mible_gatts_srv_db_t){
		.srv_type = MIBLE_PRIMARY_SERVICE,
		.srv_uuid = (mible_uuid_t){.type = 0, .uuid16 = BLE_UUID_MIJIA_SERVICE, },
		.char_num = MIJIA_IDX_NB,
		.p_char_db = p_mi_char,
	};

// char token init 
	*(p_mi_char + CHAR_TOKEN_POSITION) = (mible_gatts_char_db_t){
		.char_uuid = (mible_uuid_t){.type = 0, .uuid16 = BLE_UUID_MIJIA_TOKEN_CHARACTERISTICS, },	
		.char_property = MIBLE_WRITE | MIBLE_NOTIFY,
		.p_value = token_buf,
		.char_value_len = BLE_MIJIA_TOKEN_DATA_LEN,
		.is_variable_len = true,
		.rd_author = false,
		.wr_author = true,
		.char_desc_db = (mible_gatts_char_desc_db_t) {.extend_prop = NULL, .char_format = NULL, .user_desc = NULL,},
	};




// char pid init 
	*(p_mi_char + CHAR_PID_POSITION) = (mible_gatts_char_db_t){
		.char_uuid = (mible_uuid_t){.type = 0, .uuid16 = BLE_UUID_MIJIA_PRODUCT_ID_CHARACTERISTICS, },	
		.char_property = MIBLE_READ,
		.p_value = pid_buf,
		.char_value_len = BLE_MIJIA_PRODUCT_ID_LEN,
		.is_variable_len = false,
		.rd_author = true,
		.wr_author = false,
		.char_desc_db = (mible_gatts_char_desc_db_t) {.extend_prop = NULL, .char_format = NULL, .user_desc = NULL,},
	};	



// char version init 
	*(p_mi_char + CHAR_VERSION_POSITION) = (mible_gatts_char_db_t){
		.char_uuid = (mible_uuid_t){.type = 0, .uuid16 = BLE_UUID_MIJIA_VERSION_CHARACTERISTICS, },	
		.char_property = MIBLE_READ,
		.p_value = version_buf,
		.char_value_len = BLE_MIJIA_VERSION_LEN,
		.is_variable_len = false,
		.rd_author = false,
		.wr_author = false,
		.char_desc_db = (mible_gatts_char_desc_db_t) {.extend_prop = NULL, .char_format = NULL, .user_desc = NULL,},
	};	


// char wifi_cfg init 
	*(p_mi_char + CHAR_WIFICFG_POSITION) = (mible_gatts_char_db_t){
		.char_uuid = (mible_uuid_t){.type = 0, .uuid16 = BLE_UUID_WIFI_CONFIG_CHARACTERISTICS,},	
		.char_property = MIBLE_WRITE | MIBLE_NOTIFY,
		.p_value = wifi_cfg_buf,
		.char_value_len = BLE_MIJIA_WIFI_CFG_LEN,
		.is_variable_len = false,
		.rd_author = false,
		.wr_author = false,
		.char_desc_db = (mible_gatts_char_desc_db_t) {.extend_prop = NULL, .char_format = NULL, .user_desc = NULL,},
	};	

#if 0



// char auth init 
	*(p_mi_char + CHAR_AUTHEN_POSITION) = (mible_gatts_char_db_t){
		.char_uuid = (mible_uuid_t){.type = 0, .uuid16 = MIBLE_CHAR_UUID_AUTHENTICATION,},	
		.char_property = MIBLE_WRITE,
		.p_value = data_buffer,
		.char_value_len = MIBLE_CHAR_LEN_AUTHENTICATION,
		.is_variable_len = false,
		.rd_author = false,
		.wr_author = false,
		.char_desc_db = (mible_gatts_char_desc_db_t) {.extend_prop = NULL, .char_format = NULL, .user_desc = NULL,},
	};	

// char did init 
	*(p_mi_char + CHAR_DID_POSITION) = (mible_gatts_char_db_t){
		.char_uuid = (mible_uuid_t){.type = 0, .uuid16 = MIBLE_CHAR_UUID_DID,},	
		.char_property = MIBLE_WRITE | MIBLE_READ,
		.p_value = data_buffer,
		.char_value_len = MIBLE_CHAR_LEN_DID,
		.is_variable_len = false,
		.rd_author = false,
		.wr_author = false,
		.char_desc_db = (mible_gatts_char_desc_db_t) {.extend_prop = NULL, .char_format = NULL, .user_desc = NULL,},
	};	

// char beaconKey init
	*(p_mi_char + CHAR_BEACONKEY_POSITION) = (mible_gatts_char_db_t){
		.char_uuid = (mible_uuid_t){.type = 0, .uuid16 = MIBLE_CHAR_UUID_BEACONKEY, },	
		.char_property = MIBLE_READ,
		.p_value = srv_info.beacon_key,
		.char_value_len = MIBLE_CHAR_LEN_BEACONKEY,
		.is_variable_len = false,
		.rd_author = false,
		.wr_author = false,
		.char_desc_db = (mible_gatts_char_desc_db_t) {.extend_prop = NULL, .char_format = NULL, .user_desc = NULL,},
	};	
	
	mible_gatts_service_init(&mi_std_gatts_db);
	#endif
	return MI_SUCCESS;
}



void simulation_miserver_test(void)
{
	//mible_server_info_init(&dev_info);
	mible_server_miservice_init();
	mible_gatts_service_init(&s_server_db);
}




#if 0
void advertising_start(void)
{
	MI_LOG_INFO("advertising_start\r\n");
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
	.pid =dev_info.pid,
	.p_mac = (mible_addr_t*)dev_mac, 
	.p_capability = &cap,
	.p_obj = NULL,
	};
	
	uint8_t service_data[25];
	uint8_t service_data_len=0;
	
	mible_gap_adv_param_t adv_param =(mible_gap_adv_param_t){
		.scan_rsp_len = 0,
		.adv_type = MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
		.adv_interval_min = 0x00a0,
		.adv_interval_max = 0x00b0,
		.ch_mask = {0},
	};
	
	if(MI_SUCCESS != mibeacon_service_data_set(&mibeacon_cfg, service_data, &service_data_len)){
		MI_LOG_ERROR("mibeacon_data_set failed. \r\n");
		return;
	}
	
	MI_LOG_INFO("service_data_len = %d\r\n", service_data_len);
	
	if(MI_SUCCESS != mible_adv_data_set(service_data, service_data_len, 0x06, adv_param.adv_data, &adv_param.adv_len)){
		MI_LOG_ERROR("mible_adv_data_set failed. \r\n");
		return;
	}
	
	MI_LOG_HEXDUMP(adv_param.adv_data, adv_param.adv_len);
	//MI_LOG_PRINTF("\r\n");
	
	//Lucien debug
	for(int i=0;i<adv_param.adv_len;i++)
	{
				adv_param.adv_data[i] = adv_param.adv_data[i+3];
	}

	
	if(MI_SUCCESS != mible_gap_adv_start(&adv_param)){
		MI_LOG_ERROR("mible_gap_adv_start failed. \r\n");
		return;
	}

	return;
	
}

#else


void advertising_start(void)
{
		static mible_gap_adv_param_t adv_param =
		{
				.adv_data = USER_ADVERTISE_DATA,
				.adv_len = USER_ADVERTISE_DATA_LEN,
				.scan_rsp_data = "",
				.scan_rsp_len = 0,
				.adv_type = MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED,
				.adv_interval_min = 1100,
				.adv_interval_max = 1100,
				.ch_mask = {0},
		};
		mible_gap_adv_start(&adv_param);
}

#endif








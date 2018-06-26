/*****************************************************************************
File Name:    sdk_supplement.c
Description:  SDK 接口函数补充
History:
Date                Author                   Description
2018-02-01         Lucien                    Creat
****************************************************************************/
#include "sdk_supplement.h"
#include "app_mid.h"
#include "app_easy_security.h"
#include "gapm_task.h"
#include "nvds.h"
#include "aes.h"
#include "aes_api.h"


#define APP_EASY_GAP_MAX_CONNECTION     APP_EASY_MAX_ACTIVE_CONNECTION

static struct gapm_start_connection_cmd *start_connection_cmd                      __attribute__((section("retention_mem_area0"),zero_init)); // @RETENTION MEMORY
static struct gapm_start_advertise_cmd *adv_cmd                                    __attribute__((section("retention_mem_area0"),zero_init)); // @RETENTION MEMORY
static struct gapc_param_update_cmd *param_update_cmd[APP_EASY_GAP_MAX_CONNECTION] __attribute__((section("retention_mem_area0"),zero_init)); // @RETENTION MEMORY
static struct aes_env_tag aes_env __attribute__((section("exchange_mem_case1"))); //@RETENTION MEMORY 

//备份广播内容
static struct gapm_update_advertise_data_cmd s_update;


static struct gapm_start_advertise_cmd* app_manual_gap_advertise_start_create_msg(mible_gap_adv_param_t *param)
{
    // Allocate a message for GAP
    if (adv_cmd == NULL && param != NULL)
    {
        struct gapm_start_advertise_cmd *cmd;
        cmd = app_advertise_start_msg_create();
        adv_cmd = cmd;

				switch(param->adv_type)
				{
							case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:
								cmd->op.code = GAPM_ADV_UNDIRECT;
								break;
							case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:
								break;
							case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:
								cmd->op.code = GAPM_ADV_NON_CONN;
								break;

							default:
									return NULL;
				}
        
				
        cmd->op.addr_src = user_adv_conf.addr_src;
        cmd->intv_min = param->adv_interval_min;//user_adv_conf.intv_min;
        cmd->intv_max = param->adv_interval_max;//user_adv_conf.intv_max;
        cmd->channel_map = ((!param->ch_mask.ch_37_off)<<0) + ((!param->ch_mask.ch_38_off)<<1) + ((!param->ch_mask.ch_39_off)<<2);//user_adv_conf.channel_map;
        cmd->info.host.mode = user_adv_conf.mode;
        cmd->info.host.adv_filt_policy = user_adv_conf.adv_filt_policy;
				adv_cmd->info.host.adv_data_len = 0;
				adv_cmd->info.host.scan_rsp_data_len = 0;

				if(s_update.adv_data_len != 0){
						adv_cmd->info.host.adv_data_len = s_update.adv_data_len;
						memcpy(&(cmd->info.host.adv_data[0]), s_update.adv_data, s_update.adv_data_len);
				}

				if(s_update.scan_rsp_data_len != 0)
				{
						adv_cmd->info.host.scan_rsp_data_len = s_update.scan_rsp_data_len;
						memcpy(&(cmd->info.host.scan_rsp_data[0]), s_update.scan_rsp_data, s_update.scan_rsp_data_len);
				}
    }
    return adv_cmd;
}



mible_status_t app_manual_gap_advertise_start(mible_gap_adv_param_t * param)
{
		#define MAX_ADV_INTERVAL (0x20)
		#define MIN_ADV_INTERVAL (0x4000)
    if(param == NULL)
				return MI_ERR_INVALID_PARAM;
		if((param->adv_interval_max < param->adv_interval_min) ||\
				(param->adv_interval_min < MAX_ADV_INTERVAL) || (param->adv_interval_min > MIN_ADV_INTERVAL) ||\
				(param->adv_interval_max < MAX_ADV_INTERVAL) || (param->adv_interval_max < MAX_ADV_INTERVAL))
				return MI_ERR_INVALID_PARAM;

		if(param == NULL)
				return MI_ERR_INVALID_PARAM;
		
		struct gapm_start_advertise_cmd* cmd;
		cmd = app_manual_gap_advertise_start_create_msg(param);

		// Send the message
    app_advertise_start_msg_send(cmd);
    adv_cmd = NULL ;

    // We are now connectable
    ke_state_set(TASK_APP, APP_CONNECTABLE);


		return MI_SUCCESS;
}

/**
 ****************************************************************************************
 * @brief Create parameter update request message.
 * @return gapc_param_update_cmd Pointer to the parameter update request message
 ****************************************************************************************
 */
static struct gapc_param_update_cmd* app_manul_gap_param_update_msg_create(uint8_t conidx,mible_gap_conn_param_t *conn_params)
{
    // Allocate a message for GAP
    if (param_update_cmd[conidx] == NULL)
    {
        struct gapc_param_update_cmd* cmd;
        cmd = app_param_update_msg_create(conidx);
        ASSERT_WARNING(conidx < APP_EASY_GAP_MAX_CONNECTION);
        param_update_cmd[conidx] = cmd;

        cmd->intv_max = conn_params->max_conn_interval;//user_connection_param_conf.intv_max;
        cmd->intv_min = conn_params->min_conn_interval;//user_connection_param_conf.intv_min;
        cmd->latency = conn_params->slave_latency;     //user_connection_param_conf.latency;
        cmd->time_out = conn_params->conn_sup_timeout;// user_connection_param_conf.time_out;
        cmd->ce_len_min = user_connection_param_conf.ce_len_min;
        cmd->ce_len_max = user_connection_param_conf.ce_len_max;
    }
    return param_update_cmd[conidx];
}

void app_manual_gap_param_update_start(uint8_t conidx,mible_gap_conn_param_t *conn_params)
{
    struct gapc_param_update_cmd* cmd;
    cmd = app_manul_gap_param_update_msg_create(conidx,conn_params);

    // Send the message
    app_param_update_msg_send(cmd);
    param_update_cmd[conidx] = NULL;
}



void get_mac_addr(uint8_t *paddr)
{
		extern uint8_t nvds_get_func(uint8_t tag, nvds_tag_len_t * lengthPtr, uint8_t *buf);
		struct bd_addr dev_bdaddr;
		memset(&dev_bdaddr,0,sizeof(dev_bdaddr));
		nvds_tag_len_t len = 0;
		uint8_t state = nvds_get_func(NVDS_TAG_BD_ADDRESS,&len, dev_bdaddr.addr);
		if(paddr != NULL)
		{
				memcpy(paddr,&dev_bdaddr,sizeof(dev_bdaddr));
		}
}


mible_status_t app_gap_adv_data_set(uint8_t const * p_data,
        uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
		if( ((dlen + 3) > ADV_DATA_LEN) || (srdlen > SCAN_RSP_DATA_LEN))
				return MI_ERR_INVALID_PARAM;
		
	  

		//广播内容有变
		if(p_data != NULL)
		{
				memset(s_update.adv_data,0,sizeof(s_update.adv_data));
				memcpy(s_update.adv_data,p_data,dlen);
				s_update.adv_data_len = dlen;
		}
		//响应广播内容有变
		if(p_sr_data != NULL)
		{
				memset(s_update.scan_rsp_data,0,srdlen);
				memcpy(s_update.scan_rsp_data,p_sr_data,srdlen);
				s_update.scan_rsp_data_len = srdlen;
		}
		
		struct gapm_update_advertise_data_cmd *cmd =
        KE_MSG_ALLOC(GAPM_UPDATE_ADVERTISE_DATA_CMD, TASK_GAPM, TASK_APP, gapm_update_advertise_data_cmd);

		cmd->operation = GAPM_UPDATE_ADVERTISE_DATA;
    memcpy(cmd->adv_data,s_update.adv_data,s_update.adv_data_len);
		cmd->adv_data_len = s_update.adv_data_len;
		memcpy(cmd->scan_rsp_data,s_update.scan_rsp_data,s_update.scan_rsp_data_len);
		cmd->scan_rsp_data_len = s_update.scan_rsp_data_len;
		ke_msg_send(cmd);

		return MI_SUCCESS;
}


mible_status_t app_aes_encrypt(const uint8_t* key,const uint8_t *in,uint8_t plen,uint8_t *out)
{
		if(plen > KEY_LEN)
			return MI_ERR_INVALID_LENGTH;
		if(NULL == key || NULL == in || NULL == out)
			return MI_ERR_INVALID_PARAM;
		AES_KEY aes_key;
		unsigned char IV[KEY_LEN]           =   {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
		memcpy(aes_env.aes_key.iv, IV, KEY_LEN);

		uint8_t plaintext_temp[32];
		memset(plaintext_temp,0,sizeof(plaintext_temp));
		memcpy(plaintext_temp,in,plen);
		aes_set_key(key, 128, &aes_key, AES_ENCRYPT);
		aes_enc_dec(plaintext_temp, out, &aes_key, AES_ENCRYPT, 0);
		
		return MI_SUCCESS;
}


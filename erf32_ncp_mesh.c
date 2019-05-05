#include <stdio.h>
#include "mible_api.h"
#include "mible_type.h"
#include "mible_port.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "erf32_api.h"
#include "mible_log.h"
#include "rtl_platform.h"
#include <string.h>

#define MESH_GENERIC_CLIENT_state_on_off 				0
#define MESH_GENERIC_CLIENT_state_lightness_actual  	128
#define MESH_GENERIC_CLIENT_state_ctl_temperature 		134

#define MESH_GENERIC_CLIENT_request_on_off				0
#define MESH_GENERIC_CLIENT_request_lightness_actual	128
#define MESH_GENERIC_CLIENT_request_ctl_temperature		133

#define PROV_ADDRESS_KEY	"prov_address"
#define NETKEY_KEY			"netkey" 	// index[1] + netkey[16]
#define APPKEY_KEY			"appkey"  	// (index[1] + appkey[16])*n 

#define DEFAULT_TTL         5
#define RELAY_EN            1
#define RELAY_STEP          50
#define RELAY_RETRANS_CNT   2
#define NETTX_STEP          50
#define NETTX_CNT           2
#define PEND_ACK_BASE       (150+NETTX_STEP*NETTX_CNT)
#define PEND_ACK_STEP       (NETTX_STEP)
#define WAIT_ACK_BASE       (200+NETTX_STEP*NETTX_CNT)
#define WAIT_ACK_STEP       (NETTX_STEP)
#define SEGMENT_DELAY       (NETTX_STEP*NETTX_CNT + 20)
#define MAX_SAR_RETRY       3


static mible_mesh_event_cb_t mible_mesh_event_callback_handler;
static bool recv_unprop = false; 
static bool prov_configured = false;  // if configured 

// kv_db
// key: "prov_address"


typedef struct{
	uint8_t iv;  
	uint16_t address;	// key: "prov_address"
	uint8_t netkey_number; 
	uint8_t appkey_number;
	uint8_t netkey_id; 
	uint8_t netkey[16]; 
}prov_info_t;

static prov_info_t prov_info;  

// config sub status 
typedef struct{
	uint32_t handle; 
	mible_mesh_config_status_t config_sub_status; 
}stack_sub_status_t; 
static stack_sub_status_t sub_status_record={0};

// config node reset 
typedef struct{
	uint32_t handle;
	mible_mesh_config_status_t config_reset_status; 
}stack_reset_status_t; 
static stack_reset_status_t reset_status_record={0}; 


static mible_mesh_access_message_rx_t generic_status; 

int mible_mesh_event_callback(mible_mesh_event_type_t type, void * data)
{
    if(mible_mesh_event_callback_handler != NULL){
        mible_mesh_event_callback_handler(type, (mible_mesh_event_params_t *)data);
    }
    return 0;
}

void mible_mesh_stack_event_handler(struct gecko_cmd_packet *evt)
{
	if (NULL == evt) {
    	return;
  	}

	switch (BGLIB_MSG_ID(evt->header)) {

		case gecko_evt_mesh_prov_initialized_id:

			MI_LOG_WARNING("gecko_evt_mesh_prov_initialized_id. \n"); 
			if(evt->data.evt_mesh_prov_initialized.networks == 0){
				MI_LOG_DEBUG("Have not been configured yet.\n"); 
				prov_configured = false; 
				memset(&prov_info, 0, sizeof(prov_info_t));
				//clear kv db 
				platform_kv_delete(NETKEY_KEY);
				
			}else{
				MI_LOG_DEBUG("Have already been configured. \n");
				prov_configured = true; 
				prov_info.address = evt->data.evt_mesh_prov_initialized.address; 
				prov_info.iv = evt->data.evt_mesh_prov_initialized.ivi; 
				prov_info.netkey_number = 
					evt->data.evt_mesh_prov_initialized.networks; 

				// read kv db 
				uint8_t netkey_store[17];
				platform_kv_read(NETKEY_KEY, netkey_store, 17);
				prov_info.netkey_id = netkey_store[0]; 
				memcpy(prov_info.netkey, netkey_store+1, 16);
			}
			
			mible_mesh_event_callback(MIBLE_MESH_EVENT_STACK_INIT_DONE,NULL);
			break; 									

		case gecko_evt_mesh_prov_unprov_beacon_id:

			MI_LOG_WARNING("gecko_evt_mesh_prov_unprov_beacon_id. \n"); 
			if(!recv_unprop)
				return;				
			// MIBLE_MESH_EVENT_UNPROV_DEVICE
			mible_mesh_unprov_beacon_t beacon;
			memcpy(beacon.device_uuid,evt->data.evt_mesh_prov_unprov_beacon.uuid.data,16);
			memcpy(beacon.mac, evt->data.evt_mesh_prov_unprov_beacon.address.addr,6);
			beacon.oob_info = evt->data.evt_mesh_prov_unprov_beacon.oob_capabilities;
			memcpy(beacon.uri_hash, (uint8_t*)&(evt->data.evt_mesh_prov_unprov_beacon.uri_hash),4);
			mible_mesh_event_callback(MIBLE_MESH_EVENT_UNPROV_DEVICE, &beacon);
			break;

		case gecko_evt_mesh_generic_client_server_status_id:{
			MI_LOG_DEBUG("receive evt mesh status\n");
			memset(&generic_status, 0, sizeof(mible_mesh_access_message_rx_t)); 

			switch(evt->data.evt_mesh_generic_client_server_status.type){
				case MESH_GENERIC_CLIENT_state_on_off:

					MI_LOG_INFO("receive onoff message from 0x%x \n", 
							evt->data.evt_mesh_generic_client_server_status.server_address);
					MI_HEXDUMP(evt->data.evt_mesh_generic_client_server_status.parameters.data,
							evt->data.evt_mesh_generic_client_server_status.parameters.len); 
					generic_status.opcode.opcode = MIBLE_MESH_MSG_GENERIC_ONOFF_STATUS;
					break; 
			  	case MESH_GENERIC_CLIENT_state_lightness_actual:

					MI_LOG_INFO("receive lightness message from 0x%x \n", 
							evt->data.evt_mesh_generic_client_server_status.server_address);
					generic_status.opcode.opcode = MIBLE_MESH_MSG_LIGHT_LIGHTNESS_STATUS;
					break; 
			  	case MESH_GENERIC_CLIENT_state_ctl_temperature:

					MI_LOG_INFO("receive lightctl message from 0x%x \n", 
							evt->data.evt_mesh_generic_client_server_status.server_address);
					MI_HEXDUMP(evt->data.evt_mesh_generic_client_server_status.parameters.data,
							evt->data.evt_mesh_generic_client_server_status.parameters.len); 
					generic_status.opcode.opcode = MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_STATUS;
					break; 
			  	default:
					return; 
			}
			generic_status.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
			generic_status.meta_data.dst_addr = prov_info.address;
			generic_status.meta_data.src_addr = 
					evt->data.evt_mesh_generic_client_server_status.server_address;
			generic_status.buf = 
					evt->data.evt_mesh_generic_client_server_status.parameters.data; 
			generic_status.buf_len = 
						evt->data.evt_mesh_generic_client_server_status.parameters.len; 
			
			mible_mesh_event_params_t evt_generic_param = {
				.generic_msg = generic_status,
			};
			mible_mesh_event_callback_handler(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, 
					&evt_generic_param); 
		}
			break; 

		case gecko_evt_mesh_vendor_model_receive_id:{

			MI_LOG_INFO("vendor model received. \n"); 
			if(evt->data.evt_mesh_vendor_model_receive.opcode != 
					(MIBLE_MESH_MIOT_SPEC_STATUS & 0x3F)){
				MI_LOG_WARNING("unknown vendor model message.\n"); 
			}
			memset(&generic_status, 0, sizeof(mible_mesh_access_message_rx_t)); 
			generic_status.opcode.opcode = evt->data.evt_mesh_vendor_model_receive.opcode;
			generic_status.opcode.company_id = MIBLE_MESH_COMPANY_ID_XIAOMI;
			generic_status.meta_data.dst_addr = evt->data.evt_mesh_vendor_model_receive.destination_address;
			generic_status.meta_data.src_addr = evt->data.evt_mesh_vendor_model_receive.source_address; 
			generic_status.buf = evt->data.evt_mesh_vendor_model_receive.payload.data; 
			generic_status.buf_len = evt->data.evt_mesh_vendor_model_receive.payload.len; 
			mible_mesh_event_params_t evt_vendor_param = {
				.generic_msg = generic_status,
			};
			MI_HEXDUMP(generic_status.buf, generic_status.buf_len); 
			mible_mesh_event_callback_handler(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, 
					&evt_vendor_param); 
		}
			break; 
		
		case gecko_evt_mesh_config_client_reset_status_id:

			MI_LOG_WARNING("receive reset status event.\n"); 
			if(evt->data.evt_mesh_config_client_reset_status.handle == 
					reset_status_record.handle){

				mible_mesh_event_params_t evt_reset_param = {
					.config_msg = reset_status_record.config_reset_status,
				};
				mible_mesh_event_callback_handler(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB,
						&evt_reset_param); 
			}
			break;
		case gecko_evt_mesh_config_client_model_sub_status_id:

			MI_LOG_WARNING("receive model sub status event. handle 0x%x\n", 
					evt->data.evt_mesh_config_client_model_sub_status.handle); 
			if(evt->data.evt_mesh_config_client_model_sub_status.handle == 
					sub_status_record.handle){
				
				mible_mesh_event_params_t evt_sub_model_param = {
					.config_msg = sub_status_record.config_sub_status,
				};
				MI_LOG_DEBUG("OPCODE=0x%x, cid=0x%x, model_id=0x%x, dst_addr=0x%x, src_addr=0x%x, status = 0x%x, elem_addr = 0x%x, sub_addr=0x%x\n", sub_status_record.config_sub_status.opcode, sub_status_record.config_sub_status.opcode.company_id,
						sub_status_record.config_sub_status.model_sub_status.model_id, 
						sub_status_record.config_sub_status.meta_data.dst_addr,
						sub_status_record.config_sub_status.meta_data.src_addr,
						sub_status_record.config_sub_status.model_sub_status.status,
						sub_status_record.config_sub_status.model_sub_status.elem_addr,
						sub_status_record.config_sub_status.model_sub_status.address);
				
				mible_mesh_event_callback_handler(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB,
						&evt_sub_model_param); 
	
			}
			break; 
		case gecko_evt_mesh_friend_friendship_established_id:

			MI_LOG_DEBUG("FFFFFFriendship established. lpn address: 0x%x\n",
				evt->data.evt_mesh_friend_friendship_established.lpn_address); 
			break; 

		case gecko_evt_mesh_friend_friendship_terminated_id:

			MI_LOG_ERROR("FFFFFFriendship terminated. reason: 0x%x\n",
				evt->data.evt_mesh_friend_friendship_terminated.reason); 
			break;
		default:
			break; 
	}
}

/**
 *@brief    appkey information for node, mesh profile 4.3.2.37-39, Report 4.3.2.40 Config AppKey Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : mesh spec opcode, add/update/delete ...
 *@param    [in] param : appkey parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_appkey(uint16_t opcode, mible_mesh_appkey_params_t *param)
{
	MI_LOG_ERROR("[mible_mesh_node_set_appkey] Not Support yet. \n"); 
	return -1; 
}

/**
 *@brief    bind appkey information for node, mesh profile 4.3.2.46-47, Report 4.3.2.48 Config Model App Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : bind/unbind
 *@param    [in] param : bind parameters corresponding to node
 *@return   0: success, negetive value: failure
 */

int mible_mesh_node_bind_appkey(uint16_t opcode, mible_mesh_model_app_params_t *param)
{
	MI_LOG_ERROR("[mible_mesh_node_bind_appkey] Not Support yet. \n"); 
	return 0; 
#if 0
	if(param == NULL)
		return -1;
	mible_mesh_model_id_t id = (mible_mesh_model_id_t)(param->model_id);
	switch(opcode){
		case MIBLE_MESH_MSG_CONFIG_MODEL_APP_BIND:{
			struct gecko_msg_mesh_config_client_bind_model_rsp_t *bind_ret; 
			bind_ret = gecko_cmd_mesh_config_client_bind_model(param->global_netkey_index, 
					param->dst_addr, param->element_addr - param->dst_addr, 
					param->appkey_index, id.company_id, id.model_id);
			//bind_ret->handle;
			return bind_ret->result;
		}
		break;
		case MIBLE_MESH_MSG_CONFIG_MODEL_APP_UNBIND:{
			struct gecko_msg_mesh_config_client_unbind_model_rsp_t *unbind_ret; 
			unbind_ret = gecko_cmd_mesh_config_client_bind_model(param->global_netkey_index, 
					param->dst_addr, param->element_addr - param->dst_addr, 
					param->appkey_index, id.company_id, id.model_id);
			//unbind_ret->handle;
			return unbind_ret->result;
		}	
		break;
		default:
			return -1; 
	}
#endif
}
 
/**
 *@brief    set subscription information for node, mesh profile 4.3.2.19-25.
 *          Report 4.3.2.26 Config Model Subscription Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : delete/overwrite ...
 *@param    [in] param : subscription parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_subscription(uint16_t opcode, mible_mesh_subscription_params_t * param)
{
	MI_LOG_INFO("[mible_mesh_node_set_subscription] \n"); 
	if(param == NULL)
		return -1;
	mible_mesh_model_id_t id = (mible_mesh_model_id_t)(param->model_id);
	int ret = 0; 

	cmd_mutex_get();
	sub_status_record.handle = 0; 
	if(opcode == MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_ADD || 
			opcode == MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_OVERWRITE){

		MI_LOG_WARNING("[Add subscription]: node 0x%x, model_id 0x%x  sub_addr 0x%x\n", 
				param->dst_addr, id.model_id, param->sub_addr.value);

		struct gecko_msg_mesh_config_client_set_model_sub_rsp_t *add_ret;
		add_ret = gecko_cmd_mesh_config_client_set_model_sub(param->global_netkey_index, 
				param->dst_addr, param->element_addr - param->dst_addr, 
				id.company_id, id.model_id, param->sub_addr.value);
		
		sub_status_record.handle = add_ret->handle; 
		MI_LOG_DEBUG("set sub handle = 0x%x\n", add_ret->handle); 
		ret = add_ret->result;

	}else if(opcode == MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE){

		MI_LOG_WARNING("[Dele subscription]: node 0x%x, model_id 0x%x  sub_addr 0x%x\n", 
				param->dst_addr, id.model_id, param->sub_addr.value);

		struct gecko_msg_mesh_config_client_remove_model_sub_rsp_t *remove_ret;
		remove_ret = gecko_cmd_mesh_config_client_remove_model_sub(param->global_netkey_index, 
				param->dst_addr, param->element_addr - param->dst_addr, 
				id.company_id, id.model_id, param->sub_addr.value);
		sub_status_record.handle = remove_ret->handle; 
		ret = remove_ret->result;								

	}else{
		MI_LOG_ERROR("[mible_mesh_node_set_subscription]%d Method not supported\n", opcode); 
		cmd_mutex_put();
		return -1;
	}
	sub_status_record.config_sub_status.opcode.opcode =
		MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_STATUS ;
	sub_status_record.config_sub_status.opcode.company_id =
		MIBLE_MESH_COMPANY_ID_SIG;
	sub_status_record.config_sub_status.meta_data.src_addr = param->dst_addr;
	sub_status_record.config_sub_status.meta_data.dst_addr = prov_info.address;
	sub_status_record.config_sub_status.model_sub_status.status = 0;
	sub_status_record.config_sub_status.model_sub_status.elem_addr = 
		param->element_addr;
	sub_status_record.config_sub_status.model_sub_status.address = 
		param->sub_addr.value;  
	sub_status_record.config_sub_status.model_sub_status.model_id = 
		param->model_id; 
	cmd_mutex_put();
	return ret; 
}

/**
 *@brief    reset node, 4.3.2.53 Config Node Reset, Report 4.3.2.54 Config Node Reset Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : reset
 *@param    [in] param : reset parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_reset(uint16_t opcode, mible_mesh_reset_params_t *param)
{
	if(param == NULL)
		return -1; 
	MI_LOG_WARNING("mesh node reset: 0x%x\n", param->dst_addr);
	cmd_mutex_get();
	struct gecko_msg_mesh_config_client_reset_node_rsp_t *reset_ret; 
	reset_ret = gecko_cmd_mesh_config_client_reset_node(param->global_netkey_index, 
			param->dst_addr); 

	if(reset_ret->result != 0){
		MI_LOG_ERROR("mible_mesh_node_set_error. 0x%x\n", reset_ret->result); 
	}else{
		reset_status_record.handle = reset_ret->handle; 
		reset_status_record.config_reset_status.opcode.company_id = 
			MIBLE_MESH_COMPANY_ID_SIG;
		reset_status_record.config_reset_status.opcode.opcode = 
			MIBLE_MESH_MSG_CONFIG_NODE_RESET_STATUS; 
		reset_status_record.config_reset_status.meta_data.src_addr =
			param->dst_addr;
		reset_status_record.config_reset_status.meta_data.dst_addr =
			prov_info.address;
	}
	cmd_mutex_put();
	return reset_ret->result;
}

/**
 *@brief    generic message, Mesh model 3.2, 4.2, 5.2, 6.3, or 7 Summary.
 *          report event: MIBLE_MESH_EVENT_GENERIC_OPTION_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] param : control parameters corresponding to node
 *          according to opcode, generate a mesh message; extral params: ack_opcode, tid, get_or_set.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_generic_control(mible_mesh_generic_params_t * param)
{
	uint8_t flags = 0; 
	int ret = 0;
	cmd_mutex_get();
	switch(param->opcode.opcode){
		// GENERIC ONOFF 
		case MIBLE_MESH_MSG_GENERIC_ONOFF_GET:

			ret = gecko_cmd_mesh_generic_client_get(
					MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_CLIENT, 0, param->dst_addr.value, 
					param->global_appkey_index, MESH_GENERIC_CLIENT_state_on_off)->result;
			cmd_mutex_put();
			return ret; 

		case MIBLE_MESH_MSG_GENERIC_ONOFF_SET:

			flags = 1; 

		case MIBLE_MESH_MSG_GENERIC_ONOFF_SET_UNACKNOWLEDGED:

			flags = 0; 
			ret = gecko_cmd_mesh_generic_client_set(
					MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_CLIENT, 0, 
					param->dst_addr.value, param->global_appkey_index, param->data[1], 
					0, 0, flags, MESH_GENERIC_CLIENT_request_on_off, 1, param->data)->result;
			cmd_mutex_put();
			return ret; 
				
		// LIGHTNESS 
		case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_GET:
			
			ret = gecko_cmd_mesh_generic_client_get(
					MIBLE_MESH_MODEL_ID_LIGHTNESS_CLIENT, 0, param->dst_addr.value, 
					param->global_appkey_index, MESH_GENERIC_CLIENT_request_lightness_actual)->result;
			cmd_mutex_put();
			return ret; 

		case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET:

			flags = 1; 
	
		case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET_UNACKNOWLEDGED:

			flags = 0; 
			ret = gecko_cmd_mesh_generic_client_set(
					MIBLE_MESH_MODEL_ID_LIGHTNESS_CLIENT, 0, 
					param->dst_addr.value, param->global_appkey_index, param->data[2], 
					0, 0, flags, MESH_GENERIC_CLIENT_request_lightness_actual, 
					2, param->data)->result;
			cmd_mutex_put();
			return ret; 

		// LIGHTCTL
		case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_GET:
		
			ret = gecko_cmd_mesh_generic_client_get(
					MIBLE_MESH_MODEL_ID_CTL_CLIENT, 0, param->dst_addr.value, 
					param->global_appkey_index, MESH_GENERIC_CLIENT_state_ctl_temperature)->result;
			cmd_mutex_put();
			return ret; 
			
		case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET: 
			
			flags = 1; 
			   
		case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET_UNACKNOWLEDGED:

			flags = 0; 
			ret = gecko_cmd_mesh_generic_client_set(
					MIBLE_MESH_MODEL_ID_CTL_CLIENT, 0, param->dst_addr.value, 
					param->global_appkey_index, param->data[4], 0, 0, flags, 
					MESH_GENERIC_CLIENT_request_ctl_temperature, 4, param->data)->result;
			cmd_mutex_put();
			return ret; 
		// Vendor model
		case MIBLE_MESH_MIOT_SPEC_GET:
			
		case MIBLE_MESH_MIOT_SPEC_SET:

			MI_LOG_DEBUG("VVVVVVVVVVVendor model message send.\n"); 
			MI_HEXDUMP(param->data, param->data_len); 

			MI_LOG_DEBUG("vendor_id=0x%x, model_id=0x%x, dest_addr=0x%x, appkey_index=%d, opcode=%d\n",
					MIBLE_MESH_COMPANY_ID_XIAOMI, MIBLE_MESH_MIOT_SPEC_CLIENT_MODEL, 
					param->dst_addr.value, param->global_appkey_index, param->opcode.opcode&0x3F); 

			ret = gecko_cmd_mesh_vendor_model_send(0, MIBLE_MESH_COMPANY_ID_XIAOMI, 
					MIBLE_MESH_MIOT_SPEC_CLIENT_MODEL, param->dst_addr.value, 
					0, param->global_appkey_index, 0, param->opcode.opcode&0x3F,
					1, param->data_len, param->data)->result;
			if(ret != 0){
				MI_LOG_ERROR("Send vendor model message fail. 0x%x\n", ret); 
			}
			cmd_mutex_put();
			return ret; 
		
		default:
			break; 
	} 
	cmd_mutex_put();
	return -1;
}

/**********************************************************************//**
 * Provisioner Local Operation Definitions
 * netkey_index: local key index, global_netkey_index, is used to encrypt network data;
 * appkey_index: local key index, global_appkey_index, is used to encrypt app data;
 **********************************************************************/

/**
 *@brief    sync method, register event callback
 *@param    [in] mible_mesh_event_cb : event callback
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_register_event_callback(mible_mesh_event_cb_t mible_mesh_event_cb)
{
	mible_mesh_event_callback_handler = mible_mesh_event_cb;
	return 0;
}

/**
 *@brief    sync method, unregister event callback
 *@param    [in] mible_mesh_event_cb : event callback
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_unregister_event_callback(mible_mesh_event_cb_t mible_mesh_event_cb)
{
	if(mible_mesh_event_callback_handler == mible_mesh_event_cb){
        mible_mesh_event_callback_handler = NULL;
        return 0;
    }
    return -1;
}

/**
 *@brief    async method, init mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_INIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_init_stack(void)
{
	cmd_mutex_get();
	int result = gecko_cmd_mesh_test_set_sar_config(10000,
                                                    PEND_ACK_BASE,
                                                    PEND_ACK_STEP,
                                                    WAIT_ACK_BASE,
                                                    WAIT_ACK_STEP,
                                                    MAX_SAR_RETRY)->result;
	if(result != 0){
		MI_LOG_ERROR("set sar config error. 0x%x \n",result);
	}
    result = gecko_cmd_mesh_test_set_segment_send_delay(SEGMENT_DELAY)->result;
	if(result != 0){
		MI_LOG_ERROR("set segment send delay error. 0x%x \n", result); 
	}

	MI_LOG_WARNING("[mible_mesh_gateway_init_stack] \n"); 
	struct gecko_msg_mesh_prov_init_rsp_t * init_ret; 
	init_ret = gecko_cmd_mesh_prov_init();
	if(init_ret->result != 0){
		MI_LOG_ERROR("gecko cmd mesh prov init failed. error: 0x%x\n", init_ret->result); 
	}

	gecko_cmd_mesh_generic_client_init();
	// init vendor client 
	uint8_t opcode[6] = {0x01,0x03,0x04,0x05,0x0e,0x0f}; 
	struct gecko_msg_mesh_vendor_model_init_rsp_t *vendor_init_ret = 
			gecko_cmd_mesh_vendor_model_init(0, 0x038f, 0x0001, 0, 6, opcode); 
	if(vendor_init_ret->result != 0){
		MI_LOG_ERROR("vendor model init error. 0x%x\n", vendor_init_ret->result);
	}

	cmd_mutex_put();
	return 0;
}
/**
 *@brief    deinit mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_DEINIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_deinit_stack(void)
{
	return 0;
}
/**
 *@brief    async method, init mesh provisioner
 *          load self info, include unicast address, iv, seq_num, init model;
 *          clear local db, related appkey_list, netkey_list, device_key_list,
 *          we will load latest data for cloud;
 *          report event: MIBLE_MESH_EVENT_PROVISIONER_INIT_DONE, data: NULL.
 *@param    [in] info : init parameters corresponding to gateway
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_init_provisioner(mible_mesh_gateway_info_t *info)
{
	cmd_mutex_get();
	MI_LOG_WARNING("[mible_mesh_gateway_init_provisioner]\n");
	if(prov_configured == false){
		MI_LOG_DEBUG("initialize prov network\n");
		struct gecko_msg_mesh_prov_initialize_network_rsp_t *ret; 
		ret = gecko_cmd_mesh_prov_initialize_network(info->unicast_address, 
				info->iv_index);
		if(ret->result != 0){
			MI_LOG_ERROR("prov initialize failed 0x%x \n",ret->result); 
			cmd_mutex_put();
			return ret->result;
		}
	}
	// TODO 
    int result = gecko_cmd_mesh_test_set_nettx(NETTX_CNT, NETTX_STEP/10-1)->result;
	if(result !=0){
		MI_LOG_ERROR("set nettx error. 0x%x \n", result); 
	}	
	uint8_t var = DEFAULT_TTL;
    result = gecko_cmd_mesh_test_set_local_config(mesh_node_default_ttl, 
			0, sizeof(var), &var)->result;
	//scan param
	gecko_cmd_le_gap_set_discovery_type(1,1);

	// init friend feature
	MI_LOG_DEBUG("Init friend feature \n"); 
	if(gecko_cmd_mesh_friend_init()->result != 0){
		MI_LOG_ERROR("Init friend feature error. \n"); 
	}

	cmd_mutex_put();
	mible_mesh_event_callback(MIBLE_MESH_EVENT_PROVISIONER_INIT_DONE, NULL);
	return 0; 
}

/**
 *@brief    sync method, acquire reload_flag
 *          if you are a transciver device, always return flag = 1,
 *          we will load latest data from application;
 *          if you automatically reload data from flash or others,
 *          we will skip create_network, set_appkey, set_model_app, default group sub;
 *@return    0: do not need to reload, 1: need to reload, negetive value: failure
 */
int mible_mesh_gateway_get_reload_flag(void)
{
	if(prov_configured == false){
		return 1; 
	}else{
		return 0;
	}

}

/**
 *@brief    sync method, create mesh network for provisioner.
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] netkey : netkey value
 *@param    [in|out] stack_netkey_index : [in] default value: 0xFFFF, [out] stack generates netkey_index
 *          if your stack don't manage netkey_index and stack_netkey_index relationships, update stack_netkey_index;
 *          otherwise, do nothing.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_create_network(uint16_t netkey_index, uint8_t *netkey, uint16_t *stack_netkey_index)
{
	cmd_mutex_get();
	MI_LOG_WARNING("[mible_mesh_gateway_create_network] \n"); 
	// can only be invoked once.
	if(prov_configured == false){
		
		MI_LOG_DEBUG("Create new network\n"); 
		struct gecko_msg_mesh_prov_create_network_rsp_t *network_ret; 
		network_ret = gecko_cmd_mesh_prov_create_network(MIBLE_MESH_KEY_LEN, netkey);
		if(network_ret->result != 0){
			MI_LOG_ERROR("prov create network failed \n");
			cmd_mutex_put();
			return network_ret->result;
		}
		prov_info.netkey_id = network_ret->network_id; 
		*stack_netkey_index = network_ret->network_id; 
		prov_configured = true; 
		
		MI_LOG_INFO("network_id: %d \n", network_ret->network_id); 
		// save netkey 
		uint8_t netkey_store[17];
		netkey_store[0] = network_ret->network_id; 
		memcpy(netkey_store+1, netkey,16); 
		if(0 != platform_kv_write(NETKEY_KEY, netkey_store, 17)){
			MI_LOG_ERROR("netkey store error. \n"); 
		} 
		
	}else{
		//TODO compare the netkey 
		MI_LOG_DEBUG("Network was already created.\n"); 
		*stack_netkey_index = prov_info.netkey_id; 
	}
	// del all appkey TODO
	gecko_cmd_mesh_test_del_local_key(1,0);
	gecko_cmd_mesh_test_del_local_key(1,1);
	gecko_cmd_mesh_test_del_local_key(1,2);
	gecko_cmd_mesh_test_del_local_key(1,3);
	
	cmd_mutex_put();
	return 0;
}

/**
 *@brief    set local provisioner network transmit params.
 *@param    [in] count : advertise counter for every adv packet, adv transmit times
 *@param    [in] interval_steps : adv interval = interval_steps*0.625ms
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_network_transmit_param(uint8_t count, uint8_t interval_steps)
{
	cmd_mutex_get();
	MI_LOG_WARNING("[mible_mesh_gateway_set_network_transmit_param] \n"); 
	struct gecko_msg_mesh_test_set_nettx_rsp_t *ret; 
	ret = gecko_cmd_mesh_test_set_nettx(count, interval_steps);
	cmd_mutex_put();
	return 0; 
}

/**
 *@brief    start recv unprovision beacon, report result by MIBLE_EVENT.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_start_recv_unprovbeacon(void)
{
	cmd_mutex_get();
	MI_LOG_WARNING("[mible_mesh_start_recv_unprovbeacon] \n");
	recv_unprop = true; 
	int ret =  gecko_cmd_mesh_prov_scan_unprov_beacons()->result;
	cmd_mutex_put();
	return ret;
}

/**
 *@brief    stop recv unprovision beacon, terminate report result.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_stop_recv_unprovbeacon(void)
{
	MI_LOG_WARNING("[mible_mesh_stop_recv_unprovbeacon] \n");
	recv_unprop = false; 
	return 0;
}

/**
 *@brief    update iv index, .
 *@param    [in] iv_index : current IV Index
 *@param    [in] flags : contains the Key Refresh Flag and IV Update Flag
 *@return   0: success, negetive value: failure
 */

int mible_mesh_gateway_update_iv_info(uint32_t iv_index, uint8_t flags)
{
	MI_LOG_ERROR("[mible_mesh_gateway_update_iv_info] Not Supported Yet. \n");
	return 0;
}

/**
 *@brief    add/delete local netkey.
 *@param    [in] op : add or delete
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] netkey : netkey value
 *@param    [in|out] stack_netkey_index : [in] default value: 0xFFFF, [out] stack generates netkey_index
 *          if your stack don't manage netkey_index and stack_netkey_index relationships, update stack_netkey_index.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_netkey(mible_mesh_op_t op, uint16_t netkey_index, uint8_t *netkey, uint16_t *stack_netkey_index)
{
	MI_LOG_ERROR("[mible_mesh_gateway_set_netkey]Not Support Yet. \n");
#if 0
	if(netkey == NULL)
		return -1; 
	
	if(prov_configured == true){
		MI_LOG_ERROR("Invalid method: mible_mesh_gateway_set_netkey \n");
		return -2; 
	}
	aes_key_128 key;
	memcpy(key.data,netkey,16); 
	if(op == MIBLE_MESH_OP_ADD){
		struct gecko_msg_mesh_test_add_local_key_rsp_t *add_ret;
		add_ret = gecko_cmd_mesh_test_add_local_key(0, key, netkey_index, 0);
		*stack_netkey_index = netkey_index; 
		return add_ret->result; 
	}else{
		struct gecko_msg_mesh_test_del_local_key_rsp_t *dele_ret;
		dele_ret = gecko_cmd_mesh_test_del_local_key(0, netkey_index);
		*stack_netkey_index = netkey_index; 
		return dele_ret->result; 
	}
#endif 
	return -1;
}

/**
 *@brief    add/delete local appkey.
 *@param    [in] op : add or delete
 *@param    [in] netkey_index : key index for netkey
 *@param    [in] appkey_index : key index for appkey
 *@param    [in] appkey : appkey value
 *@param    [in|out] stack_appkey_index : [in] default value: 0xFFFF, [out] stack generates appkey_index
 *          if your stack don't manage appkey_index and stack_appkey_index relationships, update stack_appkey_index.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_appkey(mible_mesh_op_t op, uint16_t netkey_index, uint16_t appkey_index,uint8_t * appkey, uint16_t *stack_appkey_index)
{
	MI_LOG_WARNING("[mible_mesh_gateway_set_appkey]\n");

	if(appkey == NULL)
		return -1; 
	aes_key_128 key;
	memcpy(key.data, appkey, 16); 
	
	cmd_mutex_get();
	if(op == MIBLE_MESH_OP_ADD){

		struct gecko_msg_mesh_prov_create_appkey_rsp_t *add_ret; 
		add_ret = gecko_cmd_mesh_prov_create_appkey(netkey_index, 16, appkey);
		if(add_ret->result != 0){
			MI_LOG_ERROR("create appkey error. 0x%x\n", add_ret->result); 
		}
		*stack_appkey_index = add_ret->appkey_index; 
		MI_LOG_DEBUG("[add appkey]index:%d, netkey_index:%d, result:%d \n", 
				add_ret->appkey_index, netkey_index, add_ret->result);
		
		MI_LOG_DEBUG("[netkey count] %d\n",gecko_cmd_mesh_test_get_key_count(0)->count);
		MI_LOG_DEBUG("[appkey count] %d\n",gecko_cmd_mesh_test_get_key_count(1)->count);
		cmd_mutex_put();
		return add_ret->result; 
		/*
		struct gecko_msg_mesh_test_add_local_key_rsp_t *add_ret;
		add_ret = gecko_cmd_mesh_test_add_local_key(1, key, appkey_index, netkey_index);
		*stack_appkey_index = appkey_index; 
		MI_LOG_DEBUG("[add appkey]index:%d, netkey_index:%d, result:%d \n", 
				appkey_index, add_ret->result);
		struct gecko_msg_mesh_test_get_key_rsp_t *get_key_ret;
		get_key_ret = gecko_cmd_mesh_test_get_key(1, appkey_index, 1);
		MI_LOG_DEBUG("[get app key]result:%x, id:%d, data[0] = %x \n", 
				get_key_ret->result, get_key_ret->id, get_key_ret->key.data[0]);
		return add_ret->result; */
	}else{
		struct gecko_msg_mesh_test_del_local_key_rsp_t *dele_ret;
		dele_ret = gecko_cmd_mesh_test_del_local_key(1, appkey_index);
		*stack_appkey_index = appkey_index;
		MI_LOG_DEBUG("[dele appkey]index:%d, result:%d \n", 
				appkey_index,dele_ret->result);
		cmd_mutex_put();
		return dele_ret->result; 
	}
}

/**
 *@brief    bind/unbind model app.
 *@param    [in] op : bind is MIBLE_MESH_OP_ADD, unbind is MIBLE_MESH_OP_DELETE
 *@param    [in] company_id: company id
 *@param    [in] model_id : model_id
 *@param    [in] appkey_index : key index for appkey
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_model_app(mible_mesh_op_t op, uint16_t company_id, uint16_t model_id, uint16_t appkey_index)
{
	cmd_mutex_get();
	MI_LOG_WARNING("[mible_mesh_gateway_set_model_app] \n");
	if(op == MIBLE_MESH_OP_ADD){

		struct gecko_msg_mesh_test_bind_local_model_app_rsp_t *bind_ret;
		bind_ret = gecko_cmd_mesh_test_bind_local_model_app(0,
				appkey_index, company_id, model_id);
		MI_LOG_DEBUG("[model bind] company_id = %x, model_id = %x, appkey_index = %x, result = %x \n", company_id, model_id, appkey_index, bind_ret->result); 
		cmd_mutex_put();
		return bind_ret->result; 
	}else{
	
		struct gecko_msg_mesh_test_unbind_local_model_app_rsp_t *unbind_ret;
		unbind_ret = gecko_cmd_mesh_test_unbind_local_model_app(0,
				appkey_index, company_id, model_id);
		MI_LOG_DEBUG("[model unbind] company_id = %x, model_id = %x, appkey_index = %x, result = %x\n ", company_id, model_id, appkey_index, unbind_ret->result); 
		cmd_mutex_put();
		return unbind_ret->result; 
	}

}

/**
 *@brief    add/delete device key.
 *@param    [in] op : add or delete
 *@param    [in] unicast_address: remote device unicast address
 *@param    [in] device_key : device key value
 *@return   0: success, negetive value: failure
 */
int mible_mesh_gateway_set_device_key(mible_mesh_op_t op, mible_mesh_node_info_t *device)
{
	cmd_mutex_get();
	uuid_128 uuid; 
	memcpy(uuid.data, device->uuid, 16);
	aes_key_128 device_key; 
	memcpy(device_key.data, device->device_key, 16);

	if(op == MIBLE_MESH_OP_ADD){
		MI_LOG_WARNING("add device: address 0x%x \n", device->unicast_address); 
		MI_HEXDUMP(device->uuid, 16); 
		gecko_cmd_mesh_prov_ddb_delete(uuid);
		struct gecko_msg_mesh_prov_ddb_add_rsp_t * add_ret;
		add_ret = gecko_cmd_mesh_prov_ddb_add(uuid, device_key,device->netkey_index, 
				device->unicast_address, device->elements_num);
		if(add_ret->result != 0){
			MI_LOG_ERROR("add device error. error: 0x%x \n", add_ret->result); 
		}
	}else{
		MI_LOG_WARNING("delete device: address 0x%x \n", device->unicast_address); 
		struct gecko_msg_mesh_prov_ddb_delete_rsp_t *del_ret;
		del_ret = gecko_cmd_mesh_prov_ddb_delete(uuid);
		MI_HEXDUMP(device->uuid, 16); 
	}
	// get list 
	struct gecko_msg_mesh_prov_ddb_list_devices_rsp_t *dev_list_ret;
	dev_list_ret = gecko_cmd_mesh_prov_ddb_list_devices();
	MI_LOG_WARNING("device list count = %d\n", dev_list_ret->count); 
	cmd_mutex_put();
	return 0;

}

/**
 *@brief    add/delete subscription params.
 *@param    [in] op : add or delete
 *@param    [in] param: subscription params
 *@return   0: success, negetive value: failure
 */

int mible_mesh_gateway_set_sub_address(mible_mesh_op_t op, uint16_t company_id, uint16_t model_id, mible_mesh_address_t *sub_addr)
{
	cmd_mutex_get();
	if(op == MIBLE_MESH_OP_ADD){
		struct gecko_msg_mesh_test_add_local_model_sub_rsp_t *add_ret; 
		add_ret = gecko_cmd_mesh_test_add_local_model_sub(0, company_id,
				model_id, sub_addr->value);
		if(add_ret->result != 0){
			MI_LOG_ERROR("model sub address error. 0x%x\n", add_ret->result); 
			cmd_mutex_put();
	   		return add_ret->result; 	
		}else{
			MI_LOG_WARNING("model sub address: company_id = 0x%x, modle_id = 0x%x, sub_addr = 0x%x\n", company_id, model_id, sub_addr->value); 
			
			cmd_mutex_put();
			return 0;
		}
	}else{
		struct gecko_msg_mesh_test_del_local_model_sub_rsp_t *del_ret;
	   	del_ret = gecko_cmd_mesh_test_del_local_model_sub(0, company_id, 
				model_id, sub_addr->value);
		cmd_mutex_put();
		return del_ret->result;
	}
}

/**
 *@brief    suspend adv send for mesh stack.
 *@param    [in] NULL
 *@return   0: success, negetive value: failure
 */
int mible_mesh_suspend_transmission(void)
{
	return 0; 
}

/**
 *@brief    resume adv send for mesh stack.
 *@param    [in] NULL
 *@return   0: success, negetive value: failure
 */
int mible_mesh_resume_transmission(void)
{
	return 0; 
}

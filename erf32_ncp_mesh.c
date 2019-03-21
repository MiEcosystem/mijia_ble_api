#include "mible_mesh_api.h"
#include "mible_type.h"
#include "mible_port.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "erf32_api.h"

static mible_mesh_event_cb_t mible_mesh_event_callback_handler;
static bool recv_unprop = false; 
static int mible_mesh_event_callback(mible_mesh_event_type_t type, void * data)
{
    if(mible_mesh_event_callback_handler != NULL){
        mible_mesh_event_callback_handler(type, (mible_mesh_event_params_t *)data);
    }
    return 0;
}


void mible_stack_event_handler(struct gecko_cmd_packet *evt)
{
	if (NULL == evt) {
    	return;
  	}

	switch (BGLIB_MSG_ID(evt->header)) {
		case gecko_evt_mesh_prov_unprov_beacon_id:{
			if(!recv_unprop)
				return;				
			// MIBLE_MESH_EVENT_UNPROV_DEVICE
			mible_mesh_unprov_beacon_t beacon;
			memcpy(beacon.device_uuid,evt->data.evt_mesh_prov_unprov_beacon.uuid.len,16);
			memcpy(beacon.mac, evt->data.evt_mesh_prov_unprov_beacon.address.addr,6);
			beacon.oob_info = evt->data.evt_mesh_prov_unprov_beacon.oob_capabilities;
			memcpy(beacon.uri_hash, (uint8_t*)&(evt->data.evt_mesh_prov_unprov_beacon.uri_hash),4);
		}
		break;
		case gecko_evt_mesh_config_client_binding_status_id:
		break; 
		case gecko_evt_mesh_config_client_model_sub_status_id:
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
int mible_mesh_node_set_appkey(uint16_t opcode, mible_mesh_appkey_params_t *param);

/**
 *@brief    bind appkey information for node, mesh profile 4.3.2.46-47, Report 4.3.2.48 Config Model App Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : bind/unbind
 *@param    [in] param : bind parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_bind_appkey(uint32_t opcode, mible_mesh_model_app_params_t *param)
{
	if(param == NULL)
		return -1;
	mible_mesh_model_id_t id = (mible_mesh_model_id_t)(param->model_id);
	switch(opcode){
		case MIBLE_MESH_MSG_CONFIG_MODEL_APP_BIND:{
			struct gecko_msg_mesh_config_client_bind_model_rsp_t *bind_ret; 
			bind_ret = gecko_cmd_mesh_config_client_bind_model(param->global_netkey_index, 
					param->dst_addr-param->element_addr, param->element_addr, 
					param->appkey_index, id.company_id, id.model_id);
			//bind_ret->handle;
			return bind_ret->result;
		}
		break;
		case MIBLE_MESH_MSG_CONFIG_MODEL_APP_UNBIND:{
			struct gecko_msg_mesh_config_client_unbind_model_rsp_t *unbind_ret; 
			unbind_ret = gecko_cmd_mesh_config_client_bind_model(param->global_netkey_index, 
					param->dst_addr-param->element_addr, param->element_addr, 
					param->appkey_index, id.company_id, id.model_id);
			//unbind_ret->handle;
			return unbind_ret->result;
		}	
		break;
		default:
			return -1; 
	}
}

/**
 *@brief    set subscription information for node, mesh profile 4.3.2.19-25.
 *          Report 4.3.2.26 Config Model Subscription Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : delete/overwrite ...
 *@param    [in] param : subscription parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_set_subscription(uint32_t opcode, mible_mesh_subscription_params_t * param)
{
	if(param == NULL)
		return -1;
	mible_mesh_model_id_t id = (mible_mesh_model_id_t)(param->model_id);
	switch(opcode){
		case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_ADD:
		case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_OVERWRITE:{
			struct gecko_msg_mesh_config_client_set_model_sub_rsp_t *add_ret;
			add_ret = gecko_cmd_mesh_config_client_set_model_sub(param->global_netkey_index, 
					param->dst_addr-param->element_addr, param->element_addr, 
					id.company_id, id.model_id, param->sub_addr);
			//add_ret->handle 
			return add_ret->result;
		}
		break;
		case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE:{
			struct gecko_msg_mesh_config_client_remove_model_sub_rsp_t *remove_ret;
			remove_ret = gecko_cmd_mesh_config_client_remove_model_sub(param->global_netkey_index, 
					param->dst_addr-param->element_addr, param->element_addr, 
					id.company_id, id.model_id, param->sub_addr);
			//remove_ret->handle 
			return remove_ret->result;												 
															 
		}
		break;
		default:
			return -1; 
	}
}

/**
 *@brief    reset node, 4.3.2.53 Config Node Reset, Report 4.3.2.54 Config Node Reset Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_access_message_rx_t.
 *@param    [in] opcode : reset
 *@param    [in] param : reset parameters corresponding to node
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_reset(uint32_t opcode, mible_mesh_reset_params_t *param)
{
	if(param == NULL)
		return -1; 
	return gecko_cmd_mesh_config_client_reset_node(param->global_netkey_index, 
			param->dst_addr)->result; 
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
	struct gecko_msg_mesh_generic_client_get_rsp_t *get_ret;
	struct gecko_msg_mesh_generic_client_set_rsp_t *set_ret;

	switch(param->opcode){
		// GENERIC ONOFF 
		case MIBLE_MESH_MSG_GENERIC_ONOFF_GET:

			get_ret = gecko_cmd_mesh_generic_client_get(
					MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_CLIENT, param->element_index, 
					param->dst_addr, param->global_appkey_index, 
					MESH_GENERIC_CLIENT_state_on_off);
			return get_ret->result; 

		case MIBLE_MESH_MSG_GENERIC_ONOFF_SET:

			flags = 1; 

		case MIBLE_MESH_MSG_GENERIC_ONOFF_SET_UNACKNOWLEDGED:

			flags = 0; 
			set_ret = gecko_cmd_mesh_generic_client_set(
					MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_CLIENT, param->element_index, 
					param->dst_addr, param->global_appkey_index, param->data[1], 0, 0, flags, 
					MESH_GENERIC_CLIENT_state_on_off, 1, param->data);
			return set_ret->result; 
				
		// LIGHTNESS 
		case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_GET:
			
			get_ret = gecko_cmd_mesh_generic_client_get(
					MIBLE_MESH_MODEL_ID_LIGHTNESS_CLIENT, param->element_index, 
					param->dst_addr, param->global_appkey_index, 
					MESH_GENERIC_CLIENT_state_lightness_actual);
			return get_ret->result; 

		case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET:

			flags = 1; 
	
		case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET_UNACKNOWLEDGED:

			flags = 0; 
			set_ret = gecko_cmd_mesh_generic_client_set(
					MIBLE_MESH_MODEL_ID_LIGHTNESS_CLIENT, param->element_index, 
					param->dst_addr, param->global_appkey_index, param->data[2], 0, 0, flags, 
					MESH_GENERIC_CLIENT_state_lightness_actual, 2, param->data);
			return set_ret->result; 

		// LIGHTCTL
		case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_GET:
		
			get_ret = gecko_cmd_mesh_generic_client_get(
					MIBLE_MESH_MODEL_ID_CTL_CLIENT, param->element_index, 
					param->dst_addr, param->global_appkey_index, 
					MESH_GENERIC_CLIENT_state_ctl_temperature);
			return get_ret->result; 
			
		case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET: 
			
			flags = 1; 
			   
		case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET_UNACKNOWLEDGED:

			flags = 0; 
			set_ret = gecko_cmd_mesh_generic_client_set(
					MIBLE_MESH_MODEL_ID_CTL_CLIENT, param->element_index, 
					param->dst_addr, param->global_appkey_index, param->data[4], 0, 0, flags, 
					MESH_GENERIC_CLIENT_state_ctl_temperature, 4, param->data);
			return set_ret->result; 

		default:
			break; 
	} 
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
	// TODO 
	mible_mesh_event_callback(MIBLE_MESH_EVENT_STACK_INIT_DONE, NULL); 
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
	struct gecko_msg_mesh_prov_initialize_network_rsp_t *ret; 
	ret = gecko_cmd_mesh_prov_initialize_network(info->unicast_address, info->iv_index);
	return ret->result; 
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
	return 0; 
}

/**
 *@brief    start recv unprovision beacon, report result by MIBLE_EVENT.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_start_recv_unprovbeacon(void)
{
	recv_unprop = true; 
	return gecko_cmd_mesh_prov_scan_unprov_beacons()->result;

}

/**
 *@brief    stop recv unprovision beacon, terminate report result.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_stop_recv_unprovbeacon(void)
{
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
	if(netkey == NULL)
		return -1; 
	aes_key_128 key;
	memcpy(key.data,netkey,16); 
	if(op == MIBLE_MESH_OP_ADD){
		struct gecko_msg_mesh_test_add_local_key_rsp_t *add_ret;
		add_ret = gecko_cmd_mesh_test_add_local_key(0, key, netkey_index, 0);
		return add_ret->result; 
	}else{
		struct gecko_msg_mesh_test_del_local_key_rsp_t *dele_ret;
		dele_ret = gecko_cmd_mesh_test_del_local_key(0, netkey_index);
		return dele_ret->result; 
	}
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
	if(appkey == NULL)
		return -1; 
	aes_key_128 key;
	memcpy(key.data, appkey, 16); 
	if(op == MIBLE_MESH_OP_ADD){
		struct gecko_msg_mesh_test_add_local_key_rsp_t *add_ret;
		add_ret = gecko_cmd_mesh_test_add_local_key(1, key, appkey_index,netkey_index);
		return add_ret->result; 
	}else{
		struct gecko_msg_mesh_test_del_local_key_rsp_t *dele_ret;
		dele_ret = gecko_cmd_mesh_test_del_local_key(1, appkey_index);
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
	if(op == MIBLE_MESH_OP_ADD){
		struct gecko_msg_mesh_test_bind_local_model_app_rsp_t *bind_ret;
		bind_ret = gecko_cmd_mesh_test_bind_local_model_app(0,
				appkey_index, company_id, model_id);
		return bind_ret->result; 
	}else{
	
		struct gecko_msg_mesh_test_unbind_local_model_app_rsp_t *unbind_ret;
		unbind_ret = gecko_cmd_mesh_test_unbind_local_model_app(0,
				appkey_index, company_id, model_id);
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
	uuid_128 uuid; 
	memcpy(uuid.data, device->uuid, 16);
	aes_key_128 device_key; 
	memcpy(device_key.data, device->device_key, 16);

	if(op == MIBLE_MESH_OP_ADD){
		struct gecko_msg_mesh_prov_ddb_add_rsp_t * add_ret;
		add_ret = gecko_cmd_mesh_prov_ddb_add(uuid, device_key,device->netkey_index, 
				device->unicast_address, device->elements_num);
		return add_ret->result;
	}else{
		return gecko_cmd_mesh_prov_ddb_delete(uuid)->result;
	}
}

/**
 *@brief    add/delete subscription params.
 *@param    [in] op : add or delete
 *@param    [in] param: subscription params
 *@return   0: success, negetive value: failure
 */

int mible_mesh_gateway_set_sub_address(mible_mesh_op_t op, uint16_t company_id, uint16_t model_id, mible_mesh_address_t *sub_addr)
{
	struct gecko_msg_mesh_test_add_local_model_sub_rsp_t *ret; 
	ret = gecko_cmd_mesh_test_add_local_model_sub(0, company_id,
			model_id, sub_addr->type); 
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

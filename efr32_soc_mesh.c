/*
 * erf32_soc_mesh.c
 *
 *  Created on: 2020Äê11ÔÂ26ÈÕ
 *      Author: mi
 */
#include <stdio.h>
#include <string.h>
#include "mible_mesh_api.h"
#include "mi_config.h"
#include "mible_log.h"
#include "mible_api.h"
#include "efr32_api.h"
#include "mesh_auth/mible_mesh_auth.h"
#include "mesh_auth/mible_mesh_device.h"
#include "mijia_profiles/mi_service_server.h"

#include <mesh_sizes.h>
#include <gecko_configuration.h>
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"
#include "mesh_app_memory_config.h"
#include "em_rtcc.h"

/* Bluetooth stack headers */
#include "mesh_generic_model_capi_types.h"
#include "mesh_lighting_model_capi_types.h"
#include "mesh_lib.h"

#define PRIMARY_ELEM        0
#define SECONDARY_ELEM      1
#define TERTIARY_ELEM       3

#define DEFAULT_TTL         5
#define RELAY_EN            0
#define RELAY_STEP          50
#define RELAY_RETRANS_CNT   2
#define NETTX_STEP          10
#define NETTX_CNT           7
#define PEND_ACK_BASE       (150+NETTX_STEP*NETTX_CNT)
#define PEND_ACK_STEP       (NETTX_STEP)
#define WAIT_ACK_BASE       (200+NETTX_STEP*NETTX_CNT)
#define WAIT_ACK_STEP       (NETTX_STEP)
#define SEGMENT_DELAY       (NETTX_STEP*NETTX_CNT + 20)
#define MAX_SAR_RETRY       3

#define TIMER_ID_RESTART                        128
#define TIMER_ID_POLL_SECOND                    129

static const uint8_t miot_spec_opcode_set[] = {
        [0] = MIBLE_MESH_MIOT_SPEC_GET&0x3F,
        [1] = MIBLE_MESH_MIOT_SPEC_SET&0x3F,
        [2] = MIBLE_MESH_MIOT_SPEC_SET_NOACK&0x3F,
        [3] = MIBLE_MESH_MIOT_SPEC_STATUS&0x3F,
        [4] = MIBLE_MESH_SYNC_PROPS_REQ&0x3F,
        [5] = MIBLE_MESH_SYNC_PROPS_RSP&0x3F,
        [6] = MIBLE_MESH_MIOT_SPEC_EVENT&0x3F,
        [7] = MIBLE_MESH_MIOT_SPEC_EVENT_TLV&0x3F,
        [8] = MIBLE_MESH_MIOT_SPEC_ACTION&0x3F,
        [9] = MIBLE_MESH_MIOT_SPEC_ACTION_ACK&0x3F,
        [10]= MIBLE_MESH_MIOT_SPEC_INDICATION&0x3F,
        [11]= MIBLE_MESH_MIOT_SPEC_INDICATION_ACK&0x3F,
        [12] = MIBLE_MESH_MIOT_SPEC_VENDOR_CONFIG_REQ&0x3F,
        [13] = MIBLE_MESH_MIOT_SPEC_VENDOR_CONFIG_RSP&0x3F
};

static uint64_t systime = 0;
static uint32_t rtc_cnt = 0;
static uint8_t is_provisioned = 0;
static uint8_t conn_handle = 0xFF;        /* handle of the last opened LE connection */

extern int mesh_event_cnt;
extern int non_mesh_event_cnt;

/**
 *@brief    async method, init mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_INIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_init_stack(void)
{
    gecko_bgapi_class_mesh_node_init();
    gecko_bgapi_class_mesh_vendor_model_init();
    gecko_bgapi_class_mesh_test_init();
#ifndef MI_MESH_TEMPLATE_CLOUD
    gecko_bgapi_class_mesh_generic_server_init();
#endif

    uint16_t result = gecko_cmd_mesh_test_set_sar_config(10000,
                                                PEND_ACK_BASE,
                                                PEND_ACK_STEP,
                                                WAIT_ACK_BASE,
                                                WAIT_ACK_STEP,
                                                MAX_SAR_RETRY)->result;
    MI_ERR_CHECK(result);

    result = gecko_cmd_mesh_test_set_segment_send_delay(SEGMENT_DELAY)->result;
    MI_ERR_CHECK(result);

    return 0;
}
/**
 *@brief    deinit mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_DEINIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_deinit_stack(void)
{
    return 0;
}

/**
 *@brief    async method, init mesh device
 *          load self info, include unicast address, iv, seq_num, init model;
 *          clear local db, related appkey_list, netkey_list, device_key_list,
 *          we will load latest data for cloud;
 *          report event: MIBLE_MESH_EVENT_PROVISIONER_INIT_DONE, data: NULL.
 *@param    [in] info : init parameters corresponding to gateway
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_init_node(void)
{
    uint16_t result = 0;

    result = gecko_cmd_mesh_node_init()->result;
    if (result) {
        MI_LOG_ERROR("mesh stack init failed 0x%04X\n", result);
    }
    MI_ERR_CHECK(result);

    return 0;
}

/***************************************************************************//**
 * This function process the requests for the generic model.
 *
 * @param[in] model_id       Server model ID.
 * @param[in] element_index  Server model element index.
 * @param[in] client_addr    Address of the client model which sent the message.
 * @param[in] server_addr    Address the message was sent to.
 * @param[in] appkey_index   The application key index used in encrypting the request.
 * @param[in] request        Pointer to the request structure.
 * @param[in] transition_ms  Requested transition time (in milliseconds).
 * @param[in] delay_ms       Delay time (in milliseconds).
 * @param[in] request_flags  Message flags. Bitmask of the following:
 *                           - Bit 0: Nonrelayed. If nonzero indicates
 *                                    a response to a nonrelayed request.
 *                           - Bit 1: Response required. If nonzero client
 *                                    expects a response from the server.
 ******************************************************************************/
static void generic_request(uint16_t model_id,
                          uint16_t element_index,
                          uint16_t client_addr,
                          uint16_t server_addr,
                          uint16_t appkey_index,
                          const struct mesh_generic_request *request,
                          uint32_t transition_ms,
                          uint16_t delay_ms,
                          uint8_t request_flags)
{
    mible_mesh_event_params_t evt_generic_param;

    MI_LOG_INFO("ON/OFF request: requested state=<%s>, transition=%lu, delay=%u\r\n",
                request->on_off ? "ON" : "OFF", transition_ms, delay_ms);

    memset(&evt_generic_param.generic_msg, 0, sizeof(mible_mesh_access_message_t));
    switch(request->kind){
    case mesh_generic_request_on_off:
        evt_generic_param.generic_msg.opcode.opcode = MIBLE_MESH_MSG_GENERIC_ONOFF_SET;
        evt_generic_param.generic_msg.buf = (uint8_t*)&(request->on_off);
        evt_generic_param.generic_msg.buf_len = sizeof(uint8_t);
        break;
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
    case mesh_lighting_request_lightness_actual:
        evt_generic_param.generic_msg.opcode.opcode = MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET;
        evt_generic_param.generic_msg.buf = (uint8_t*)&(request->lightness);
        evt_generic_param.generic_msg.buf_len = sizeof(uint16_t);
        break;
#if defined(MI_MESH_TEMPLATE_LIGHTCTL)
    case mesh_lighting_request_ctl_temperature:
        evt_generic_param.generic_msg.opcode.opcode = MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET;
        evt_generic_param.generic_msg.buf = (uint8_t*)&(request->ctl_temperature);
        evt_generic_param.generic_msg.buf_len = sizeof(uint16_t) + sizeof(int16_t);
        break;
#endif
#endif
    default:
        MI_LOG_ERROR("can't find request type %d\r\n",request->kind);
        return;
    }
    evt_generic_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_generic_param.generic_msg.meta_data.dst_addr = server_addr;
    evt_generic_param.generic_msg.meta_data.src_addr = client_addr;
    evt_generic_param.generic_msg.meta_data.elem_index = element_index;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_generic_param);
}

/***************************************************************************//**
 * Initialization of the models supported by this node.
 * This function registers callbacks for each of the supported models.
 ******************************************************************************/
static int init_sig_models(void)
{
    uint16_t result;
    /* Initialize mesh lib */
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_FAN)
    mesh_lib_init(malloc, free, 1);
#elif defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
    mesh_lib_init(malloc, free, 2);
#elif defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
    mesh_lib_init(malloc, free, 3);
#endif

#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL) || defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) \
    || defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_FAN)
    result = gecko_cmd_mesh_generic_server_init_on_off()->result;
    MI_ERR_CHECK(result);
    result = mesh_lib_generic_server_register_handler(MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
                                            PRIMARY_ELEM,
                                            generic_request,
                                            NULL,
                                            NULL);
    MI_ERR_CHECK(result);
#endif
#if defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
    result = mesh_lib_generic_server_register_handler(MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
                                            SECONDARY_ELEM,
                                            generic_request,
                                            NULL,
                                            NULL);
    MI_ERR_CHECK(result);
#endif
#if defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
    result = mesh_lib_generic_server_register_handler(MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
                                            TERTIARY_ELEM,
                                            generic_request,
                                            NULL,
                                            NULL);
    MI_ERR_CHECK(result);
#endif

#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
    result = gecko_cmd_mesh_generic_server_init_lightness()->result;
    MI_ERR_CHECK(result);
    result = mesh_lib_generic_server_register_handler(MESH_LIGHTING_LIGHTNESS_SERVER_MODEL_ID,
                                            PRIMARY_ELEM,
                                            generic_request,
                                            NULL,
                                            NULL);
    MI_ERR_CHECK(result);
#endif
#if defined(MI_MESH_TEMPLATE_LIGHTCTL)
    result = gecko_cmd_mesh_generic_server_init_ctl()->result;
    MI_ERR_CHECK(result);
    result = mesh_lib_generic_server_register_handler(MESH_LIGHTING_CTL_TEMPERATURE_SERVER_MODEL_ID,
                                            SECONDARY_ELEM,
                                            generic_request,
                                            NULL,
                                            NULL);
    MI_ERR_CHECK(result);
#endif

#ifndef MI_MESH_TEMPLATE_CLOUD
    result = gecko_cmd_mesh_generic_server_init_common()->result;
    MI_ERR_CHECK(result);
#endif
    return result;
}

static void local_config_init(void)
{
    uint16_t result;

    result = init_sig_models();
    MI_ERR_CHECK(result);
    result = gecko_cmd_mesh_vendor_model_init(  0,
                                                MIBLE_MESH_COMPANY_ID_XIAOMI,
                                                MIBLE_MESH_MIOT_SPEC_SERVER_MODEL,
                                                0,
                                                sizeof(miot_spec_opcode_set),
                                                miot_spec_opcode_set)->result;
    MI_ERR_CHECK(result);
    result = gecko_cmd_mesh_vendor_model_init(  0,
                                                MIBLE_MESH_COMPANY_ID_XIAOMI,
                                                MIBLE_MESH_MIJIA_SERVER_MODEL,
                                                0,
                                                0,
                                                NULL)->result;
    MI_ERR_CHECK(result);

    result = gecko_cmd_mesh_test_set_nettx(NETTX_CNT, NETTX_STEP/10-1)->result;
    MI_ERR_CHECK(result);
    result = gecko_cmd_mesh_test_set_relay(RELAY_EN, RELAY_RETRANS_CNT, RELAY_STEP/10-1)->result;
    MI_ERR_CHECK(result);
    result = gecko_cmd_mesh_node_set_iv_update_age(691200)->result;
    MI_LOG_INFO("gecko_cmd_mesh_node_set_iv_update_age %d\r\n", result);
    MI_ERR_CHECK(result);

    MI_LOG_INFO("local configuration server config.\n");
    uint8_t var = DEFAULT_TTL;
    result = gecko_cmd_mesh_test_set_local_config(mesh_node_default_ttl, 0, sizeof(var), &var)->result;
    MI_ERR_CHECK(result);
    var = 1;
    result = gecko_cmd_mesh_test_set_local_config(mesh_node_beacon, 0, sizeof(var), &var)->result;
    MI_ERR_CHECK(result);
    var = 0;

#ifdef MI_LOG_ENABLED
    struct gecko_msg_mesh_test_get_nettx_rsp_t *p_nettx = gecko_cmd_mesh_test_get_nettx();
    MI_LOG_WARNING("nettx\t cnt %d\t interval %d\n", p_nettx->count, (p_nettx->interval+1)*10);

    struct gecko_msg_mesh_test_get_relay_rsp_t *p_relay = gecko_cmd_mesh_test_get_relay();
    MI_LOG_WARNING("relay\t stat %d\t cnt %d\t interval %d\n", p_relay->enabled, p_relay->count, (p_relay->interval+1)*10);

    uint8_t friend = gecko_cmd_mesh_test_get_local_config(mesh_node_friendship, 0)->data.data[0];
    uint8_t beacon = gecko_cmd_mesh_test_get_local_config(mesh_node_beacon, 0)->data.data[0];
    uint8_t ttl = gecko_cmd_mesh_test_get_local_config(mesh_node_default_ttl, 0)->data.data[0];
    uint8_t proxy = gecko_cmd_mesh_test_get_local_config(mesh_node_gatt_proxy, 0)->data.data[0];
    uint8_t identity = gecko_cmd_mesh_test_get_local_config(mesh_node_identity, 0)->data.data[0];
    MI_LOG_INFO("friend: %d  beacon: %d  ttl: %d  proxy: %d  identity: %d\n", friend, beacon, ttl, proxy, identity);
#endif
}

/**
 *@brief    set node provsion data.
 *@param    [in] param : prov data include devkey, netkey, netkey idx,
 *          uni addr, iv idx, key flag
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_provsion_data(mible_mesh_provisioning_data_t *param)
{
    uint16_t result = 0;
    MI_LOG_WARNING("[mible_mesh_gateway_set_model_app] \n");
    aes_key_128 devkey, netkey;
    memcpy(devkey.data, param->devkey, 16);
    memcpy(netkey.data, param->netkey, 16);
    result = gecko_cmd_mesh_node_set_provisioning_data(
            devkey,
            netkey,
            param->net_idx,
            param->iv,
            param->address,
            param->flags & 0x01)->result;
    MI_ERR_CHECK(result);
    return result;
}

/**
 *@brief    mesh provsion done. need update node info and
 *          callback MIBLE_MESH_EVENT_DEVICE_INIT_DONE event
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_provsion_done(void)
{
    return mible_mesh_device_reboot();
}

/**
 *@brief    reset node, 4.3.2.53 Config Node Reset, Report 4.3.2.54 Config Node Reset Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_reset(void)
{
    // erase mesh data
    return gecko_cmd_mesh_node_reset()->result;
}

/**
 *@brief    mesh unprovsion done. need update node info and
 *          callback MIBLE_MESH_EVENT_DEVICE_INIT_DONE event
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_unprovsion_done(void)
{
    return mible_mesh_device_reboot();
}

/**
 *@brief    set local provisioner network transmit params.
 *@param    [in] count : advertise counter for every adv packet, adv transmit times
 *@param    [in] interval_steps : adv interval = interval_steps*0.625ms
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_network_transmit_param(uint8_t count, uint8_t interval_steps)
{
    uint16_t result;
    MI_LOG_WARNING("[mible_mesh_gateway_set_network_transmit_param] \n");
    result = gecko_cmd_mesh_test_set_nettx(count, interval_steps)->result;
    MI_ERR_CHECK(result);
    return 0;
}

/**
 *@brief    set node relay onoff.
 *@param    [in] enabled : 0: relay off, 1: relay on
 *@param    [in] count: Number of relay transmissions beyond the initial one. Range: 0-7
 *@param    [in] interval: Relay retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_relay(uint8_t enabled,uint8_t count, uint8_t interval)
{
    uint16_t result;
    result = gecko_cmd_mesh_test_set_relay(enabled, count, interval)->result;
    MI_ERR_CHECK(result);
    return result;
}

/**
 *@brief    update iv index, .
 *@param    [in] iv_index : current IV Index
 *@param    [in] flags : contains the Key Refresh Flag and IV Update Flag
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_update_iv_info(uint32_t iv_index, uint8_t flags)
{
    MI_LOG_WARNING("[mible_mesh_gateway_update_iv_info]  \n");
    if(gecko_cmd_mesh_test_set_iv_index(iv_index)->result != 0){
        MI_LOG_ERROR("set iv index error. \n");
        return -1;
    }
    if(gecko_cmd_mesh_test_set_ivupdate_state(flags)->result != 0){
        MI_LOG_ERROR("set ivupdate state error. \n");
        return -2;
    }
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
int mible_mesh_device_set_netkey(mible_mesh_op_t op, uint16_t netkey_index, uint8_t *netkey)
{
    MI_LOG_WARNING("[mible_mesh_gateway_set_netkey] \n");
    aes_key_128 key;
    memcpy(key.data, netkey, 16);
    if(op == MIBLE_MESH_OP_ADD){
        if(gecko_cmd_mesh_test_add_local_key(0, key, netkey_index, 0xFFFF)->result != 0){
            MI_LOG_ERROR("set netkey error. \n");
            return -1;
        }
#if !MINIMIZE_FLASH_SIZE
    }else{
        if(gecko_cmd_mesh_test_del_local_key(0, netkey_index)->result != 0){
            MI_LOG_ERROR("delete netkey error. \n");
            return -2;
        }
#endif
    }
    return 0;
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
int mible_mesh_device_set_appkey(mible_mesh_op_t op, uint16_t netkey_index, uint16_t appkey_index, uint8_t * appkey)
{
    MI_LOG_WARNING("[mible_mesh_gateway_set_appkey] \n");
    aes_key_128 key;
    memcpy(key.data, appkey, 16);
    if(op == MIBLE_MESH_OP_ADD){
        if(gecko_cmd_mesh_test_add_local_key(1, key, appkey_index, netkey_index)->result != 0){
            MI_LOG_ERROR("set appkey error. \n");
            return -1;
        }
        //*stack_appkey_index = appkey_index;
#if !MINIMIZE_FLASH_SIZE
    }else{
        if(gecko_cmd_mesh_test_del_local_key(1, appkey_index)->result != 0){
            MI_LOG_ERROR("delete appkey error. \n");
            return -2;
        }
        //*stack_appkey_index = appkey_index;
#endif
    }
    return 0;
}

/**
 *@brief    bind/unbind model app.
 *@param    [in] op : bind is MIBLE_MESH_OP_ADD, unbind is MIBLE_MESH_OP_DELETE
 *@param    [in] company_id: company id
 *@param    [in] model_id : model_id
 *@param    [in] appkey_index : key index for appkey
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_model_app(mible_mesh_op_t op, uint16_t elem_index, uint16_t company_id, uint16_t model_id, uint16_t appkey_index)
{
    uint16_t result = 0;
    if(op == MIBLE_MESH_OP_ADD){
        result = gecko_cmd_mesh_test_bind_local_model_app(elem_index, appkey_index,
                company_id == 0? 0xFFFF: company_id, model_id)->result;
        MI_LOG_DEBUG("[model bind] company_id = %x, model_id = %x, appkey_index = %x, result = %x \n", company_id, model_id, appkey_index, result);
#if !MINIMIZE_FLASH_SIZE
    }else{
        result = gecko_cmd_mesh_test_unbind_local_model_app(elem_index, appkey_index,
                company_id == 0? 0xFFFF: company_id, model_id)->result;
        MI_LOG_DEBUG("[model unbind] company_id = %x, model_id = %x, appkey_index = %x, result = %x \n", company_id, model_id, appkey_index, result);
#endif
    }
    return result;
}

/**
 *@brief    add/delete subscription params.
 *@param    [in] op : add or delete
 *@param    [in] element : model element
 *@param    [in] company_id: company id
 *@param    [in] model_id : model_id
 *@param    [in] sub_addr: subscription address params
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_sub_address(mible_mesh_op_t op, uint16_t element, uint16_t company_id, uint16_t model_id, uint16_t sub_addr)
{
    uint16_t result = 0;
    struct gecko_msg_mesh_test_get_local_model_sub_rsp_t* p = gecko_cmd_mesh_test_get_local_model_sub(
            element, company_id == 0? 0xFFFF : company_id, model_id);
    MI_LOG_INFO("[set_sub_address]op: %s, elem %d, model %04x, subaddr %04x, sub_list:\n",
                op? "del":"add", element, model_id, sub_addr, p->result);
    MI_LOG_HEXDUMP(p->addresses.data, p->addresses.len);

    if (p->result != bg_err_success){
        MI_LOG_ERROR("model %04X cid %04X has no sub ?\n", model_id, company_id);
        return -1;
    }

    if(op == MIBLE_MESH_OP_ADD){
        if (p->addresses.len && memcmp(&sub_addr, p->addresses.data, p->addresses.len) == 0) {
            return 0;
        }
        /* delete all subaddr */
        for (uint8_t i = 0; i < p->addresses.len; i+=2) {
            uint16_t addr = p->addresses.data[i] + (p->addresses.data[i+1]<<8);
            result = gecko_cmd_mesh_test_del_local_model_sub(element,
                    company_id == 0? 0xFFFF : company_id, model_id, addr)->result;
            MI_ERR_CHECK(result);
        }
        result = gecko_cmd_mesh_test_add_local_model_sub(element,
                company_id == 0? 0xFFFF : company_id, model_id, sub_addr)->result;
        MI_ERR_CHECK(result);
    }else{
        if(p->addresses.len){
            result = gecko_cmd_mesh_test_del_local_model_sub(element,
                        company_id == 0? 0xFFFF : company_id, model_id, sub_addr)->result;
            MI_ERR_CHECK(result);
        }
    }

    return 0;
}

/**
 *@brief    set node tx power.
 *@param    [in] power : TX power in 0.1 dBm steps.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_tx_power(int16_t power)
{
    uint16_t result;
    result = gecko_cmd_system_set_tx_power(power)->set_power;
    MI_LOG_DEBUG("set tx power %d.\n", result);
    return result;
}

/**
 *@brief    reboot device.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_reboot(void)
{
    return gecko_cmd_hardware_set_soft_timer(32768 / 2, TIMER_ID_RESTART, 1)->result;
}

/**
 *@brief    generic message, Mesh model 3.2, 4.2, 5.2, 6.3, or 7 Summary.
 *          report event: MIBLE_MESH_EVENT_GENERIC_OPTION_CB, data: mible_mesh_access_message_t.
 *@param    [in] param : control parameters corresponding to node
 *          according to opcode, generate a mesh message; extral params: ack_opcode, tid, get_or_set.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_generic_control(mible_mesh_access_message_t *param)
{
    int result = 0;
    struct mesh_generic_state current;
    switch(param->opcode.opcode){
    case MIBLE_MESH_MSG_GENERIC_ONOFF_STATUS:
        MI_LOG_DEBUG("Generic onoff model message send. src %04x, dst %04x, opcode %04x, data:\n",
                param->meta_data.src_addr, param->meta_data.dst_addr, param->opcode.opcode);
        MI_HEXDUMP(param->buf, param->buf_len);
        current.kind = mesh_generic_state_on_off;
        memcpy(&(current.on_off), param->buf, sizeof(uint8_t));
        result = mesh_lib_generic_server_update(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,
                param->meta_data.elem_index, &current, &current, 0);
        MI_ERR_CHECK(result);
        result = mesh_lib_generic_server_response(MESH_GENERIC_ON_OFF_SERVER_MODEL_ID,
                param->meta_data.elem_index, param->meta_data.dst_addr,
                param->meta_data.appkey_index, &current, &current, 0, 0);
        MI_ERR_CHECK(result);
        break;
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
    case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_STATUS:
        MI_LOG_DEBUG("Light lightness model message send. src %04x, dst %04x, opcode %04x, data:\n",
                param->meta_data.src_addr, param->meta_data.dst_addr, param->opcode.opcode);
        MI_HEXDUMP(param->buf, param->buf_len);
        current.kind = mesh_lighting_state_lightness_actual;
        memcpy(&(current.lightness), param->buf, sizeof(uint16_t));
        result = mesh_lib_generic_server_update(MESH_LIGHTING_LIGHTNESS_SERVER_MODEL_ID,
                param->meta_data.elem_index, &current, &current, 0);
        MI_ERR_CHECK(result);
        result = mesh_lib_generic_server_response(MESH_LIGHTING_LIGHTNESS_SERVER_MODEL_ID,
                param->meta_data.elem_index, param->meta_data.dst_addr,
                param->meta_data.appkey_index, &current, &current, 0, 0);
        MI_ERR_CHECK(result);
        break;
#endif
#if defined(MI_MESH_TEMPLATE_LIGHTCTL)
    case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_STATUS:
        MI_LOG_DEBUG("Light lightness model message send. src %04x, dst %04x, opcode %04x, data:\n",
                param->meta_data.src_addr, param->meta_data.dst_addr, param->opcode.opcode);
        MI_HEXDUMP(param->buf, param->buf_len);
        current.kind = mesh_lighting_state_ctl_temperature;
        memcpy(&(current.ctl_temperature), param->buf, sizeof(uint32_t));
        result = mesh_lib_generic_server_update(MESH_LIGHTING_CTL_TEMPERATURE_SERVER_MODEL_ID,
                param->meta_data.elem_index, &current, &current, 0);
        MI_ERR_CHECK(result);
        result = mesh_lib_generic_server_response(MESH_LIGHTING_CTL_TEMPERATURE_SERVER_MODEL_ID,
                param->meta_data.elem_index, param->meta_data.dst_addr,
                param->meta_data.appkey_index, &current, &current, 0, 0);
        MI_ERR_CHECK(result);
        break;
#endif
    case MIBLE_MESH_MIOT_SPEC_STATUS:
    case MIBLE_MESH_SYNC_PROPS_REQ:
    case MIBLE_MESH_MIOT_SPEC_EVENT:
    case MIBLE_MESH_MIOT_SPEC_EVENT_TLV:
    case MIBLE_MESH_MIOT_SPEC_ACTION_ACK:
    case MIBLE_MESH_MIOT_SPEC_INDICATION:
    case MIBLE_MESH_MIOT_SPEC_VENDOR_CONFIG_RSP:
        MI_LOG_DEBUG("VVVVVVVVVVVendor model message send. src %04x, dst %04x, opcode %04x, data:\n",
                param->meta_data.src_addr, param->meta_data.dst_addr, param->opcode.opcode);
        MI_HEXDUMP(param->buf, param->buf_len);
        result = gecko_cmd_mesh_vendor_model_send(0, MIBLE_MESH_COMPANY_ID_XIAOMI,
                MIBLE_MESH_MIOT_SPEC_SERVER_MODEL, param->meta_data.dst_addr, 0,
                param->meta_data.appkey_index, 0, param->opcode.opcode&0x3F, 1,
                param->buf_len, param->buf)->result;
        MI_ERR_CHECK(result);
        break;
    }
    return result;
}

uint64_t mible_mesh_get_exact_systicks(void)
{
    uint32_t ticks = RTCC_CounterGet() - rtc_cnt;
    //MI_LOG_DEBUG("mible_mesh_get_exact_systicks %lld ms\n", systime*1000 + ticks*1000/32768);
    return systime*1000 + ticks*1000/32768;
}

static void process_mesh_node_init_event(struct gecko_cmd_packet *evt)
{
    mible_mesh_node_init_t node_info = {
        .map = {
            [0] = {
                .siid = 0,
                .piid = 0,
                .model_id = MIBLE_MESH_MIOT_SPEC_SERVER_MODEL,
                .company_id = MIBLE_MESH_COMPANY_ID_XIAOMI,
                .element = 0,
                .appkey_idx = 0,
            },
            [1] = {
                .siid = 0,
                .piid = 0,
                .model_id = MIBLE_MESH_MIJIA_SERVER_MODEL,
                .company_id = MIBLE_MESH_COMPANY_ID_XIAOMI,
                .element = 0,
                .appkey_idx = 0,
            },
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL) || defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) \
    || defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_FAN)
            [2] = {
                .siid = 2,
                .piid = 1,
                .model_id = MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
                .company_id = MIBLE_MESH_COMPANY_ID_SIG,
                .element = 0,
                .appkey_idx = 0,
            },
    #if defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
            [3] = {
                .siid = 3,
                .piid = 1,
                .model_id = MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
                .company_id = MIBLE_MESH_COMPANY_ID_SIG,
                .element = 1,
                .appkey_idx = 0,
            },
        #if defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
            [4] = {
                .siid = 4,
                .piid = 1,
                .model_id = MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
                .company_id = MIBLE_MESH_COMPANY_ID_SIG,
                .element = 2,
                .appkey_idx = 0,
            },
        #else
            [4] = {0},
        #endif
    #elif defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
            [3] = {
                .siid = 2,
                .piid = 2,
                .model_id = MIBLE_MESH_MODEL_ID_LIGHTNESS_SERVER,
                .company_id = MIBLE_MESH_COMPANY_ID_SIG,
                .element = 0,
                .appkey_idx = 0,
            },
        #if defined(MI_MESH_TEMPLATE_LIGHTCTL)
            [4] = {
                .siid = 2,
                .piid = 3,
                .model_id = MIBLE_MESH_MODEL_ID_CTL_TEMPEATURE_SERVER,
                .company_id = MIBLE_MESH_COMPANY_ID_SIG,
                .element = 1,
                .appkey_idx = 0,
            },
        #else
            [4] = {0},
        #endif
    #else
            [3] = {0},
            [4] = {0},
    #endif
#else
            [2] = {0},
            [3] = {0},
            [4] = {0},
#endif
        },
    };
    node_info.provisioned = evt->data.evt_mesh_node_initialized.provisioned;
    node_info.address = evt->data.evt_mesh_node_initialized.address;
    node_info.ivi = evt->data.evt_mesh_node_initialized.ivi;
    is_provisioned = node_info.provisioned;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_DEVICE_INIT_DONE, &node_info);
}

static void process_mesh_vendor_model_recv_event(struct gecko_cmd_packet *evt)
{
    mible_mesh_event_params_t evt_vendor_param;

    memset(&evt_vendor_param.generic_msg, 0, sizeof(mible_mesh_access_message_t));
    evt_vendor_param.generic_msg.opcode.opcode = evt->data.evt_mesh_vendor_model_receive.opcode | 0xC0;
    evt_vendor_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_XIAOMI;
    evt_vendor_param.generic_msg.meta_data.dst_addr = evt->data.evt_mesh_vendor_model_receive.destination_address;
    evt_vendor_param.generic_msg.meta_data.src_addr = evt->data.evt_mesh_vendor_model_receive.source_address;
    evt_vendor_param.generic_msg.buf = evt->data.evt_mesh_vendor_model_receive.payload.data;
    evt_vendor_param.generic_msg.buf_len = evt->data.evt_mesh_vendor_model_receive.payload.len;

    MI_LOG_INFO("vvvvvvendor model received, opcode %04x, data:\n", evt_vendor_param.generic_msg.opcode.opcode);
    MI_LOG_HEXDUMP(evt->data.evt_mesh_vendor_model_receive.payload.data, evt->data.evt_mesh_vendor_model_receive.payload.len);
    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_vendor_param);
}

static void process_mesh_node_model_config(struct gecko_cmd_packet *evt)
{
    static uint16_t m_pri_group = 0;
    mible_mesh_event_params_t evt_vendor_param;
    memset(&evt_vendor_param.config_msg, 0, sizeof(mible_mesh_config_status_t));

    struct gecko_msg_mesh_node_model_config_changed_evt_t changed = evt->data.evt_mesh_node_model_config_changed;
    if(changed.mesh_node_config_state == 0x02){     //Model subscription list
        uint16_t m_pri_elem_addr = gecko_cmd_mesh_node_get_element_address(0)->address;
        struct gecko_msg_mesh_test_get_local_model_sub_rsp_t* p = gecko_cmd_mesh_test_get_local_model_sub(
                    changed.element_address - m_pri_elem_addr, changed.vendor_id, changed.model_id);
        if (p->result == bg_err_success) {
            evt_vendor_param.config_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
            evt_vendor_param.config_msg.meta_data.dst_addr = changed.element_address;
            evt_vendor_param.config_msg.meta_data.elem_index = changed.element_address - m_pri_elem_addr;
            evt_vendor_param.config_msg.model_sub_set.elem_addr = changed.element_address;
            evt_vendor_param.config_msg.model_sub_set.model_id.model_id = changed.model_id;
            evt_vendor_param.config_msg.model_sub_set.model_id.company_id = changed.vendor_id;
            if(p->addresses.len){
                evt_vendor_param.config_msg.opcode.opcode = MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_OVERWRITE;
                memcpy(&evt_vendor_param.config_msg.model_sub_set.address, p->addresses.data, 2);
                m_pri_group = evt_vendor_param.config_msg.model_sub_set.address - evt_vendor_param.config_msg.meta_data.elem_index;
                MI_LOG_INFO("[SUB OVERWRITE]model %04x, elem %d, group address %d\n", changed.model_id,
                            evt_vendor_param.config_msg.meta_data.elem_index, evt_vendor_param.config_msg.model_sub_set.address);
            }else{
                evt_vendor_param.config_msg.opcode.opcode = MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE;
                evt_vendor_param.config_msg.model_sub_set.address = m_pri_group + evt_vendor_param.config_msg.meta_data.elem_index;
                MI_LOG_INFO("[SUB DELETE]model %04x, elem %d, group address %d\n", changed.model_id,
                            evt_vendor_param.config_msg.meta_data.elem_index, evt_vendor_param.config_msg.model_sub_set.address);
            }

            mible_mesh_event_callback(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, &evt_vendor_param);
        }
    }else if(changed.mesh_node_config_state == 0x00){
        MI_LOG_INFO("[Model application key bindings]addr %04x, vendor %04x, model %04x\n",
                    changed.element_address, changed.vendor_id, changed.model_id);
    }else if(changed.mesh_node_config_state == 0x01){
        MI_LOG_INFO("[Model publication parameters]addr %04x, vendor %04x, model %04x\n",
                    changed.element_address, changed.vendor_id, changed.model_id);
    }
}

static void process_mesh_node_config_set(struct gecko_cmd_packet *evt)
{
    mible_mesh_config_status_t config_msg;
    memset(&config_msg, 0, sizeof(mible_mesh_config_status_t));

    struct gecko_msg_mesh_node_config_set_evt_t config_set = evt->data.evt_mesh_node_config_set;
    if(config_set.id == mesh_node_relay){
        config_msg.opcode.opcode = MIBLE_MESH_MSG_CONFIG_RELAY_SET;
        config_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
        config_msg.meta_data.netkey_index = config_set.netkey_index;
        config_msg.relay_set.relay = config_set.value.data[0];
        config_msg.relay_set.relay_retrans_cnt = config_set.value.data[1]&0x07;
        config_msg.relay_set.relay_retrans_intvlsteps = (config_set.value.data[1]&0xF8) >> 3;
        MI_LOG_INFO("[RELAY SET]relay %d, cnt %d, intval %d\n", config_msg.relay_set.relay,
                config_msg.relay_set.relay_retrans_cnt, config_msg.relay_set.relay_retrans_intvlsteps);
        MI_LOG_HEXDUMP(config_set.value.data, config_set.value.len);

        mible_mesh_event_params_t evt_vendor_param = {
            .config_msg = config_msg,
        };
        mible_mesh_event_callback(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, &evt_vendor_param);
    }else{
        MI_LOG_INFO("[Node config]known id %d\n", config_set.id);
    }
}

static void process_mesh_node_reset(struct gecko_cmd_packet *evt)
{
    mible_mesh_event_params_t evt_vendor_param;

    /* if connection is open then close it before rebooting */
    if (conn_handle != 0xFF) {
        gecko_cmd_le_connection_close(conn_handle);
    }

    evt_vendor_param.config_msg.opcode.opcode = MIBLE_MESH_MSG_CONFIG_NODE_RESET;
    evt_vendor_param.config_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, &evt_vendor_param);
}

static void process_mesh_iv_update_event(uint32_t iv_index, uint8_t flags)
{
    mible_mesh_iv_t mesh_iv;
    mesh_iv.iv_index = iv_index;
    mesh_iv.flags = flags;
    mible_mesh_event_callback(MIBLE_MESH_EVENT_IV_UPDATE, &mesh_iv);
}

static void process_soft_timer_event(struct gecko_cmd_packet *evt)
{
    switch (evt->data.evt_hardware_soft_timer.handle) {
    case TIMER_ID_RESTART:
        MI_LOG_INFO("system reboot.\n");
        gecko_cmd_system_reset(0);
        break;
    case TIMER_ID_POLL_SECOND:
        systime ++;
        rtc_cnt = RTCC_CounterGet();

        MI_LOG_DEBUG("events: mesh %3d, others %3d. systime: %d\n",
                    mesh_event_cnt, non_mesh_event_cnt, systime);
        mesh_event_cnt = 0;
        non_mesh_event_cnt = 0;
        // procedures run periodic
        if (is_provisioned && systime % 1800 == 0) {
            uint32_t seq_remain = gecko_cmd_mesh_node_get_seq_remaining(0)->count;
            if (seq_remain < 0x100000 && gecko_cmd_mesh_node_get_ivupdate_state()->state == 0)
                gecko_cmd_mesh_node_request_ivupdate();
        }
        break;
    default:
        break;
    }
}

/**
 * Handling of stack events. Both BLuetooth LE and Bluetooth mesh events are handled here.
 */
static uint8_t mesh_scan_level;
void mible_mesh_stack_event_handler(struct gecko_cmd_packet *evt)
{
    uint16_t result;
    if (NULL == evt) {
        return;
    }

    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_hardware_soft_timer_id:
        process_soft_timer_event(evt);
        break;
    case gecko_evt_system_boot_id:
        MI_LOG_WARNING("[Stack event] gecko_evt_system_boot_id\n");
        mi_service_init();
        //mi_scheduler_init(10, mible_mesh_schd_event_handler, NULL);
        mible_mesh_device_set_tx_power(85);
        result = gecko_cmd_hardware_set_soft_timer(MS_2_TIMERTICK(1000), TIMER_ID_POLL_SECOND, 0)->result;
        MI_ERR_CHECK(result);
        if (GPIO_PinInGet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN) == 0){
            result = gecko_cmd_mesh_node_reset()->result;
            MI_ERR_CHECK(result);
            result = gecko_cmd_hardware_set_soft_timer(32768 / 2, TIMER_ID_RESTART, 1)->result;
            MI_ERR_CHECK(result);
        }
        mible_mesh_event_callback(MIBLE_MESH_EVENT_STACK_INIT_DONE, NULL);
        break;
    case gecko_evt_mesh_node_initialized_id:
        MI_LOG_WARNING("[Stack event] gecko_evt_mesh_node_initialized_id\n");
        local_config_init();
        process_mesh_node_init_event(evt);
        break;
    case gecko_evt_mesh_node_changed_ivupdate_state_id:
        MI_LOG_WARNING("[Stack event] evt gecko_evt_mesh_node_changed_ivupdate_state_id, ivindex = %x, state %d\r\n",
               evt->data.evt_mesh_node_changed_ivupdate_state.ivindex, evt->data.evt_mesh_node_changed_ivupdate_state.state);
        process_mesh_iv_update_event(evt->data.evt_mesh_node_changed_ivupdate_state.ivindex,
                                    evt->data.evt_mesh_node_changed_ivupdate_state.state);
        break;
    case gecko_evt_mesh_node_ivrecovery_needed_id:
        MI_LOG_WARNING("[Stack event] evt gecko_evt_mesh_node_ivrecovery_needed_id, network_ivindex = %x, node_ivindex = %x\r\n",
               evt->data.evt_mesh_node_ivrecovery_needed.network_ivindex, evt->data.evt_mesh_node_ivrecovery_needed.node_ivindex);
        gecko_cmd_mesh_node_set_ivrecovery_mode(1);
        process_mesh_iv_update_event(evt->data.evt_mesh_node_ivrecovery_needed.network_ivindex, 0);
        break;
    case gecko_evt_mesh_generic_server_client_request_id:
        MI_LOG_WARNING("[Stack event] server_client_request\r\n");
        mesh_lib_generic_server_event_handler(evt);
        break;
    case gecko_evt_mesh_generic_server_state_changed_id:
        MI_LOG_WARNING("[Stack event] server_state_changed\r\n");
        // pass the server state changed event to mesh lib handler that will invoke
        // the callback functions registered by application
        // mesh_lib_generic_server_event_handler(evt);
        break;
    case gecko_evt_mesh_vendor_model_receive_id:
        MI_LOG_WARNING("[Stack event] vendor_model_receive\r\n");
        process_mesh_vendor_model_recv_event(evt);
        break;
    case gecko_evt_mesh_node_model_config_changed_id:
        MI_LOG_WARNING("[Stack event] model_config_changed\r\n");
        process_mesh_node_model_config(evt);
        break;
    case gecko_evt_mesh_node_config_set_id:
        MI_LOG_WARNING("[Stack event] config_set\r\n");
        //process_mesh_node_config_set(evt);
        break;
    case gecko_evt_mesh_node_reset_id:
        MI_LOG_WARNING("[Stack event] evt gecko_evt_mesh_node_reset_id\r\n");
        process_mesh_node_reset(evt);
        break;

    case gecko_evt_le_connection_opened_id:
        conn_handle = evt->data.evt_le_connection_opened.connection;
        MI_LOG_WARNING("[Stack event] gecko_evt_le_connection_opened_id: %d\n", evt->data.evt_le_connection_opened.advertiser);
        mesh_scan_level = mible_mesh_device_scan_get();
        result = gecko_cmd_le_gap_end_procedure()->result;
        MI_ERR_CHECK(result);
        result = gecko_cmd_le_gap_set_discovery_timing(1, 200, 40)->result;
        MI_ERR_CHECK(result);
        result = gecko_cmd_le_gap_start_discovery(1, 2)->result;
        MI_ERR_CHECK(result);
        break;
    case gecko_evt_le_connection_closed_id:
        conn_handle = 0xFF;
        MI_LOG_WARNING("[Stack event] conn closed, reason 0x%x\r\n", evt->data.evt_le_connection_closed.reason);
        result = mible_mesh_device_scan_set(mesh_scan_level);
        MI_ERR_CHECK(result);
        break;
    case gecko_evt_le_connection_parameters_id:
        MI_LOG_WARNING("[Stack event] gecko_evt_le_connection_parameters_id: conn %d, int %d, latency %d, timeout %d\n",
                evt->data.evt_le_connection_parameters.connection, evt->data.evt_le_connection_parameters.interval,
                evt->data.evt_le_connection_parameters.latency, evt->data.evt_le_connection_parameters.timeout);
        break;
    default:
        //      MI_LOG_INFO("unhandled evt: %8.8x class %2.2x method %2.2x\r\n", evt_id, (evt_id >> 16) & 0xFF, (evt_id >> 24) & 0xFF);
        break;
    }
}


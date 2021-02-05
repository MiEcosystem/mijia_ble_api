#include <stdio.h>
#include <string.h>
#include "mible_mesh_api.h"
#include "mi_config.h"
#include "mible_log.h"
#include "mible_api.h"
#include "mesh_auth/mible_mesh_auth.h"
#include "mesh_auth/mible_mesh_device.h"
#include "mijia_profiles/mi_service_server.h"
#include "cryptography/mi_mesh_otp.h"
#include "cryptography/mi_crypto.h"

//#include <trace.h>
#include <platform_utils.h>
#include <platform_macros.h>
#include "rtl876x_nvic.h"
#include "rtl876x_rtc.h"
#include "otp_config.h"
#include "gap_scheduler.h"
#include "gap_vendor.h"
#include "mem_config.h"
#include "mesh_api.h"
#include "mesh_sdk.h"
#include "mesh_node.h"
#include "health.h"

#include "rtk_mesh.h"
#include "miot_model.h"
#include "mijia_model.h"


/* default ttl number */
#define MI_DEFAULT_TTL                           5
/* network default parameters */
#define MI_NET_RETRANS_COUNT                     7 //8*10ms
#define MI_NET_RETRANS_INTERVAL_STEPS            0 //(n + 1) * 10ms
/* relay default parameters */
#define MI_RELAY_RETRANS_COUNT                   2
#define MI_RELAY_RETRANS_INTERVAL_STEPS          4 //(n + 1) * 10ms
/* network message cache */
#define MI_NMC_SIZE                              96
/* reply protection list size */
#define MI_RPL_SIZE                              32
/* iv update trigger sequence */
#define MI_IV_UPDATE_TRIGGER_SEQUENCE_NUM        0xf00000
/* time of expecting to receive segment acknowledge */
#define MI_TIME_RECV_SEG_ACK                     350
/* time after sending segment acknowledgement */
#define MI_TIME_SEND_SEG_ACK                     300
/* scheduler task number */
#define MI_GAP_SCHED_TASK_NUM                    15
/* relay parallel max number */
#define MI_GAP_SCHED_RELAY_PARALLEL_MAX_NUM      5

#define MI_GATT_TIMEOUT                         20000
#define MI_REGSUCC_TIMEOUT                      5000

/* mi inner message type */
typedef enum
{
    MI_SCHD_PROCESS,
} mi_inner_msg_type_t;

/* mi inner message data */
typedef struct
{
    mi_inner_msg_type_t type;
    uint16_t sub_type;
    union
    {
        uint32_t parm;
        void *pbuf;
    };
} mi_inner_msg_t;

/* app message parameters */
static uint8_t mi_event;
static plt_os_queue_handle_t mi_event_queue_handle;
static plt_os_queue_handle_t mi_queue_handle;
static plt_timer_t poll_second_timer = NULL;

static uint32_t systime = 0;
static uint32_t rtc_cnt = 0;
static uint8_t is_provisioned = 0;
//static uint8_t conn_handle = 0xFF;        /* handle of the last opened LE connection */
/* mible state, avoid multiple initialize */
static bool mible_start = FALSE;
static bool is_initialized = false;
/* connect paramenters */
static uint16_t mible_conn_handle = 0xFFFF;
/* connect timer */
static void *mible_conn_timer = NULL;

extern mible_status_t mible_record_init(void);
extern T_GAP_CAUSE le_vendor_set_priority(T_GAP_VENDOR_PRIORITY_PARAM *p_priority_param);

static void process_mesh_node_init_event(void)
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
    node_info.provisioned = is_provisioned;//(cb_type == PROV_CB_TYPE_PROV) ? 1:0;
    node_info.lpn_node = MI_MESH_LOW_POWER_NODE;
    node_info.address = mesh_node.unicast_addr;
    node_info.ivi = iv_index_get();
    //is_provisioned = node_info.provisioned;
    
    mible_mesh_event_callback(MIBLE_MESH_EVENT_DEVICE_INIT_DONE, &node_info);
}

static void process_mesh_node_reset(prov_cb_type_t cb_type, prov_cb_data_t cb_data)
{
    mible_mesh_event_params_t evt_vendor_param;

    evt_vendor_param.config_msg.opcode.opcode = MIBLE_MESH_MSG_CONFIG_NODE_RESET;
    evt_vendor_param.config_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    is_provisioned = 0;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, &evt_vendor_param);
}

/******************************************************************
 * @fn      prov_cb
 * @brief   Provisioning callbacks are handled in this function.
 *
 * @param   cb_data  -  @ref TProvisioningCbData
 * @return  the operation result
 */
bool prov_cb(prov_cb_type_t cb_type, prov_cb_data_t cb_data)
{
    MI_LOG_INFO("prov_cb: type = %d", cb_type);

    switch (cb_type)
    {
    case PROV_CB_TYPE_PB_ADV_LINK_STATE:
        break;
    case PROV_CB_TYPE_UNPROV:
        /* stack ready or node reset*/
        if(is_provisioned){
            process_mesh_node_reset(cb_type, cb_data);
        }else{
            is_provisioned = false;
            is_initialized = true;
        }
        //mi_unprov();
        break;
    case PROV_CB_TYPE_START:
        break;
    case PROV_CB_TYPE_COMPLETE:
        is_provisioned = true;
        process_mesh_node_init_event();
        break;
    case PROV_CB_TYPE_FAIL:
        break;
    case PROV_CB_TYPE_PROV:
        /* stack ready */
        is_provisioned = true;
        is_initialized = true;
        break;
    default:
        break;
    }
    return true;
}

void mi_default_config(void)
{
    mesh_node.ttl = MI_DEFAULT_TTL;
    mesh_node.net_trans_count = MI_NET_RETRANS_COUNT;
    mesh_node.net_trans_steps = MI_NET_RETRANS_INTERVAL_STEPS;
    mesh_node.relay_retrans_count = MI_RELAY_RETRANS_COUNT;
    mesh_node.relay_retrans_steps = MI_RELAY_RETRANS_INTERVAL_STEPS;
    mesh_node.nmc_size = MI_NMC_SIZE;
    mesh_node.seq_siv = MI_IV_UPDATE_TRIGGER_SEQUENCE_NUM;
    mesh_node.trans_retrans_base = MI_TIME_RECV_SEG_ACK;
    mesh_node.trans_retrans_seg_factor = 0;
    mesh_node.trans_ack_base = MI_TIME_SEND_SEG_ACK;
    mesh_node.trans_ack_seg_factor = 0;
    mesh_node.relay_parallel_max = MI_GAP_SCHED_RELAY_PARALLEL_MAX_NUM;
/*
    uint16_t scan_window = MI_GAP_SCHED_SCAN_WINDOW;
    uint16_t scan_interval = MI_GAP_SCHED_SCAN_INTERVAL;
    gap_sched_params_set(GAP_SCHED_PARAMS_SCAN_WINDOW, &scan_window, sizeof(uint16_t));
    gap_sched_params_set(GAP_SCHED_PARAMS_SCAN_INTERVAL, &scan_interval, sizeof(uint16_t));
*/
    uint8_t task_num = MI_GAP_SCHED_TASK_NUM;
    gap_sched_params_set(GAP_SCHED_PARAMS_TASK_NUM, &task_num, sizeof(task_num));
}

/* sig models */
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL) || \
    defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || \
    defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_FAN)
#include "generic_on_off.h"
static mesh_model_info_t generic_on_off_server;
/**
 * @brief generic on/off server data callback
 * @param[in] pmodel_info: generic on/off server model handler
 * @param[in] type: data type
 * @param[in, out] pargs: data need to process
 */
static int32_t generic_on_off_server_data(const mesh_model_info_p pmodel_info, uint32_t type,
                                          void *pargs)
{
    mesh_msg_t *pmesh_msg = pargs;
    mible_mesh_event_params_t evt_generic_param;
    
    memset(&evt_generic_param.generic_msg, 0, sizeof(mible_mesh_access_message_t));
    
    evt_generic_param.generic_msg.opcode.opcode = pmesh_msg->access_opcode;
    if(pmesh_msg->access_opcode == MIBLE_MESH_MSG_GENERIC_ONOFF_SET || 
       pmesh_msg->access_opcode == MIBLE_MESH_MSG_GENERIC_ONOFF_SET_UNACKNOWLEDGED){
        uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset + ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
        evt_generic_param.generic_msg.buf = pbuffer;
        evt_generic_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
    }
    evt_generic_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_generic_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_generic_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_generic_param.generic_msg.meta_data.elem_index = pmodel_info->element_index;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_generic_param);
    return 0;
}
#if defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
static mesh_model_info_t generic_on_off_server_2st;
#if defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
static mesh_model_info_t generic_on_off_server_3rd;
#endif //defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
#endif //defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
#endif

#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
#include "light_lightness.h"
static mesh_model_info_t light_lightness_server;
/**
 * @brief light lightness server data callback
 * @param[in] pmodel_info: light lightness server model handler
 * @param[in] type: data type
 * @param[in, out] pargs: data need to process
 */
static int32_t light_lightness_server_data(const mesh_model_info_p pmodel_info, uint32_t type,
                                           void *pargs)
{
    mesh_msg_t *pmesh_msg = pargs;
    mible_mesh_event_params_t evt_generic_param;
    
    memset(&evt_generic_param.generic_msg, 0, sizeof(mible_mesh_access_message_t));
    evt_generic_param.generic_msg.opcode.opcode = pmesh_msg->access_opcode;
    if(pmesh_msg->access_opcode == MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET || 
       pmesh_msg->access_opcode == MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET_UNACKNOWLEDGED){
        uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset + ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
        evt_generic_param.generic_msg.buf = pbuffer;
        evt_generic_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
    }
    evt_generic_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_generic_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_generic_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_generic_param.generic_msg.meta_data.elem_index = pmodel_info->element_index;
    
    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_generic_param);
    return 0;
}
#endif
#if defined(MI_MESH_TEMPLATE_LIGHTCTL)
#include "light_ctl.h"
static mesh_model_info_t light_ctl_temperature_server;
/**
 * @brief light ctl server data callback
 * @param[in] pmodel_info: light ctl server model handler
 * @param[in] type: data type
 * @param[in, out] pargs: data need to process
 */
static int32_t light_ctl_server_data(const mesh_model_info_p pmodel_info, uint32_t type,
                                     void *pargs)
{
    mesh_msg_t *pmesh_msg = pargs;
    mible_mesh_event_params_t evt_generic_param;
    
    memset(&evt_generic_param.generic_msg, 0, sizeof(mible_mesh_access_message_t));
    evt_generic_param.generic_msg.opcode.opcode = pmesh_msg->access_opcode;
    if(pmesh_msg->access_opcode == MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET || 
       pmesh_msg->access_opcode == MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET_UNACKNOWLEDGED){
        uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset + ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
        evt_generic_param.generic_msg.buf = pbuffer;
        evt_generic_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
    }
    evt_generic_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_generic_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_generic_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_generic_param.generic_msg.meta_data.elem_index = pmodel_info->element_index;
    
    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_generic_param);
    return 0;
}
#endif
/* miot server model */
static mesh_model_info_t miot_server;
static mesh_model_info_t mijia_server;
/**
 * @brief miot server (038F0000) data callback
 * @param[in] pmodel_info: light lightness server model handler
 * @param[in] type: data type
 * @param[in, out] pargs: data need to process
 */
static int32_t miot_server_data(const mesh_model_info_p pmodel_info, uint32_t type,
                         void *pargs)
{
    mesh_msg_t *pmesh_msg = pargs;
    mible_mesh_event_params_t evt_vendor_param;
    
    memset(&evt_vendor_param.generic_msg, 0, sizeof(mible_mesh_access_message_t));
    evt_vendor_param.generic_msg.opcode.opcode = pmesh_msg->access_opcode >> 16;
    
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset + ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
    evt_vendor_param.generic_msg.buf = pbuffer;
    evt_vendor_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
    evt_vendor_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_XIAOMI;
    evt_vendor_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_vendor_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_vendor_param.generic_msg.meta_data.elem_index = 0;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_vendor_param);
    return 0;
}
/**
 * @brief mijia server (038F0002) data callback
 * @param[in] pmodel_info: light lightness server model handler
 * @param[in] type: data type
 * @param[in, out] pargs: data need to process
 */
static int32_t mijia_server_data(const mesh_model_info_p pmodel_info, uint32_t type,
                                 void *pargs)
{
    return 0;
}

/* health server model */
static mesh_model_info_t health_server_model;

static bool cfg_server_receive_peek(mesh_msg_p pmesh_msg)
{
    bool ret = true;
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset;
    
    mible_mesh_event_params_t evt_vendor_param;
    memset(&evt_vendor_param.config_msg, 0, sizeof(mible_mesh_config_status_t));

    evt_vendor_param.config_msg.opcode.opcode = pmesh_msg->access_opcode;
    evt_vendor_param.config_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_vendor_param.config_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_vendor_param.config_msg.meta_data.src_addr = pmesh_msg->src;
    evt_vendor_param.config_msg.meta_data.elem_index = pmesh_msg->dst - mesh_node.unicast_addr;
    
    switch (pmesh_msg->access_opcode)
    {
    case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_OVERWRITE:
    case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE:
    {
        cfg_model_sub_add_t *pmsg = (cfg_model_sub_add_t *)pbuffer;
        evt_vendor_param.config_msg.model_sub_set.elem_addr = pmsg->element_addr;
        evt_vendor_param.config_msg.model_sub_set.address = pmsg->addr;
        if(pmesh_msg->msg_len == sizeof(cfg_model_sub_add_t)){
            evt_vendor_param.config_msg.model_sub_set.model_id.model_id = pmsg->model_id >> 16;
            evt_vendor_param.config_msg.model_sub_set.model_id.company_id = pmsg->model_id & 0xffff;
        }else{
            evt_vendor_param.config_msg.model_sub_set.model_id.model_id = pmsg->model_id & 0xffff;
            evt_vendor_param.config_msg.model_sub_set.model_id.company_id = MIBLE_MESH_COMPANY_ID_SIG;
        }

        mible_mesh_event_callback(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, &evt_vendor_param);
    }
        break;
#if !MINIMIZE_FLASH_SIZE
    case MIBLE_MESH_MSG_CONFIG_RELAY_SET:
    {
        cfg_relay_set_t *pmsg = (cfg_relay_set_t *)pbuffer;
        evt_vendor_param.config_msg.relay_set.relay = pmsg->state;
        evt_vendor_param.config_msg.relay_set.relay_retrans_cnt = pmsg->count;
        evt_vendor_param.config_msg.relay_set.relay_retrans_intvlsteps = pmsg->steps;
        mible_mesh_event_callback(MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, &evt_vendor_param);
        ret = cfg_server_receive(pmesh_msg);
    }
        break;
#endif
    default:
        MI_LOG_DEBUG("access_opcode : 0x%04x!", pmesh_msg->access_opcode);
        ret = cfg_server_receive(pmesh_msg);
        break;
    }
    return ret;
}

static int init_models(void)
{
    uint16_t result = 0;
    /* create elements */
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) || \
    defined(MI_MESH_TEMPLATE_FAN) || defined(MI_MESH_TEMPLATE_CLOUD)
    DBG_DIRECT("[init_model] creat element 0!");
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
#elif defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
    DBG_DIRECT("[init_model] creat element 0&1!");
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
#elif defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
    DBG_DIRECT("[init_model] creat element 0&1&2!");
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
    mesh_element_create(GATT_NS_DESC_UNKNOWN);
#endif
    
    /* register miot model */
    miot_server.model_data_cb = miot_server_data;
    result = miot_server_reg(0, &miot_server);
    DBG_DIRECT("[init_model] reg miot_server model, result: %d!", result);
    mijia_server.model_data_cb = mijia_server_data;
    result = mijia_server_reg(0, &mijia_server);
    DBG_DIRECT("[init_model] reg mijia_server model, result: %d!", result);
    mijia_server.pmodel_bound = &miot_server;
    
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL) || \
    defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || \
    defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_FAN)
    generic_on_off_server.model_data_cb = generic_on_off_server_data;
    result = generic_on_off_server_reg(0, &generic_on_off_server);
    DBG_DIRECT("[init_model] reg generic_on_off model 1, result: %d!", result);
    miot_server.pmodel_bound = &generic_on_off_server;
#endif
#if defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
    generic_on_off_server_2st.model_data_cb = generic_on_off_server_data;
    result = generic_on_off_server_reg(1, &generic_on_off_server_2st);
    DBG_DIRECT("[init_model] reg generic_on_off model 2, result: %d!", result);
#endif
#if defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
    generic_on_off_server_3rd.model_data_cb = generic_on_off_server_data;
    result = generic_on_off_server_reg(2, &generic_on_off_server_3rd);
    DBG_DIRECT("[init_model] reg generic_on_off model 3, result: %d!", result);
#endif

#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
    light_lightness_server.model_data_cb = light_lightness_server_data;
    result = light_lightness_server_reg(0, &light_lightness_server);
    DBG_DIRECT("[init_model] reg light_lightness model, result: %d!", result);
    light_lightness_server.pmodel_bound = &generic_on_off_server;
    miot_server.pmodel_bound = &light_lightness_server;
#endif
#if defined(MI_MESH_TEMPLATE_LIGHTCTL)
    light_ctl_temperature_server.model_data_cb = light_ctl_server_data;
    result = light_ctl_temperature_server_reg(1, &light_ctl_temperature_server);
    DBG_DIRECT("[init_model] reg light_ctl_temperature model, result: %d!", result);
#endif

    /* register health model */
    result = health_server_reg(0, &health_server_model);
    DBG_DIRECT("[init_model] reg health_server model, result: %d!", result);
    health_server_set_company_id(&health_server_model, COMPANY_ID);
    /* register config model cb*/
    cfg_server.model_receive = cfg_server_receive_peek;

    return result;
}

/**
 *@brief    async method, init mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_INIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_init_stack(void)
{
    /** set ble stack log level, disable nonsignificant log */
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, LEVEL_TRACE, 0);
    log_module_bitmap_trace_set(0xFFFFFFFFFFFFFFFF, LEVEL_INFO, 0);
    log_module_trace_set(MODULE_LOWERSTACK, LEVEL_ERROR, 0);
    log_module_trace_set(MODULE_SNOOP, LEVEL_ERROR, 0);

    /** set mesh stack log level, default all on, disable the log of level LEVEL_TRACE */
    uint32_t module_bitmap[MESH_LOG_LEVEL_SIZE] = {0};
    diag_level_set(LEVEL_TRACE, module_bitmap);

    /* mask some log */
    diag_level_set(LEVEL_WARN, module_bitmap);
    module_bitmap[0] = 0x01;
    diag_level_set(LEVEL_INFO, module_bitmap);

#if 1
    /** print the mesh sdk & lib version */
    mesh_sdk_version();

    /** mesh stack needs rand seed */
    plt_srand(platform_random(0xffffffff));
    
    /** set device name and appearance */
    char *dev_name = MODEL_NAME;
    uint16_t appearance = GAP_GATT_APPEARANCE_UNKNOWN;
    gap_sched_params_set(GAP_SCHED_PARAMS_DEVICE_NAME, dev_name, GAP_DEVICE_NAME_LEN);
    gap_sched_params_set(GAP_SCHED_PARAMS_APPEARANCE, &appearance, sizeof(appearance));

    /** set device uuid according to bt address */
    uint8_t bt_addr[6];
    uint8_t dev_uuid[16] = MESH_DEVICE_UUID;
    gap_get_param(GAP_PARAM_BD_ADDR, bt_addr);
    memcpy(dev_uuid, bt_addr, sizeof(bt_addr));
    device_uuid_set(dev_uuid);
    
    /** configure provisioning parameters */
    prov_capabilities_t prov_capabilities =
    {
        .algorithm = PROV_CAP_ALGO_FIPS_P256_ELLIPTIC_CURVE,
        .public_key = 0,
        .static_oob = 0,
        .output_oob_size = 0,
        .output_oob_action = 0,
        .input_oob_size = 0,
        .input_oob_action = 0
    };
    prov_params_set(PROV_PARAMS_CAPABILITIES, &prov_capabilities, sizeof(prov_capabilities_t));
    prov_params_set(PROV_PARAMS_CALLBACK_FUN, prov_cb, sizeof(prov_cb_pf));
    
    /** config node parameters */
    mesh_node_features_t features =
    {
        .role = MESH_ROLE_DEVICE,
#if MI_MESH_LOW_POWER_NODE
        .relay = 0,
#else
        .relay = 1,
#endif
        .proxy = 2,
        .fn = 0,
        .lpn = 0,
        .prov = 2,
        .udb = 0,
        .snb = 1,
        .bg_scan = 1,
        .flash = 1,
        .flash_rpl = 1
    };
    mesh_node_cfg_t node_cfg =
    {
        .dev_key_num = 1,
        .net_key_num = 3,
        .app_key_num = 3,
        .vir_addr_num = 3,
        .rpl_num = MI_RPL_SIZE,
        .sub_addr_num = 10,
        .proxy_num = 1,
        .udb_interval = 5,
        .snb_interval = 200,
        .prov_interval = 10,
        .proxy_interval = 10,
        .identity_interval = 20
    };
    mesh_node_cfg(features, &node_cfg);
    mi_default_config();

    /* register element and models */
    init_models();

    /* generate composition data */
    compo_data_page0_header_t compo_data_page0_header = {COMPANY_ID, PRODUCT_ID, VERSION_ID};
    compo_data_page0_gen(&compo_data_page0_header);
    
    /** restore light ahead since it may restore the fatory setting */
    //light_flash_restore();
    mible_record_init();

    /* use segment message to response configuration */
    //cfg_server_resp_with_seg_msg(TRUE);

    /* use different network transmit for configuration */
    //cfg_server_set_net_trans(6, 0);

    /** init mesh stack */
    mesh_init();

    /* clear iv index timer after power on */
    //mesh_node.iv_timer_count = 0;
#endif
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
 *          report event: MIBLE_MESH_EVENT_DEVICE_INIT_DONE, data: NULL.
 *@param    [in] info : init parameters corresponding to gateway
 *@return   0: success, negetive value: failure
 */
extern void rtk_mesh_stack_start(void);
int mible_mesh_device_init_node(void)
{
    /* send event in process_mesh_node_init_event */
    /** start mesh stack */
    //rtk_mesh_stack_start();
    //prov_params_set(PROV_PARAMS_CALLBACK_FUN, prov_cb, sizeof(prov_cb_pf));
    return 0;
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
    MI_LOG_WARNING("[mible_mesh_device_set_provsion_data] \n");
    /* clear the flash first */
    mesh_node_clear();
    
     /* store device key */
    mesh_node.dev_key_list[0].used = 1;
    mesh_node.dev_key_list[0].element_num = mesh_node.element_queue.count;
    mesh_node.dev_key_list[0].unicast_addr = param->address;
    memcpy(mesh_node.dev_key_list[0].dev_key, param->devkey, MESH_COMMON_KEY_SIZE);
    uint16_t dev_key_index = 0;
    mesh_flash_store(MESH_FLASH_PARAMS_DEV_KEY, &dev_key_index);
    
    /* store netkey */
    uint16_t net_key_index = 0;
    if (param->flags & 0x01)
    {
        uint8_t key[MESH_COMMON_KEY_SIZE] = {0};
        net_key_update(net_key_index, param->net_idx, key);
        net_key_update(net_key_index, param->net_idx, param->netkey);
        net_key_refresh(net_key_index);
    }
    else
    {
        net_key_update(net_key_index, param->net_idx, param->netkey);
    }
    mesh_flash_store(MESH_FLASH_PARAMS_NET_KEY, &net_key_index);
    /* store iv index */
    iv_index_set(param->iv);
    mesh_flash_store(MESH_FLASH_PARAMS_IV_INDEX, NULL);
    iv_index_timer_start();
    mesh_node.iv_update_flag = (param->flags & 0x02) >> 1;
    mesh_node.iv_timer_count = MESH_IV_INDEX_48W; //!< should stay (MESH_IV_INDEX_48W)
    mesh_seq_clear();

    /* store the node state finally */
    mesh_node.unicast_addr = param->address;
    mesh_node.node_state = PROV_NODE;
    mesh_flash_store(MESH_FLASH_PARAMS_NODE_INFO, NULL);

    return result;
}

/**
 *@brief    mesh provsion done. need update node info and
 *          callback MIBLE_MESH_EVENT_DEVICE_INIT_DONE event
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_provsion_done(void)
{
    prov_cb_data_t droppable_data;
    droppable_data.pb_generic_cb_type = PB_GENERIC_CB_MSG;
    prov_cb(PROV_CB_TYPE_COMPLETE, droppable_data);
    mible_timer_start(mible_conn_timer, MI_REGSUCC_TIMEOUT, NULL);
    return 0;
}

/**
 *@brief    reset node, 4.3.2.53 Config Node Reset, Report 4.3.2.54 Config Node Reset Status.
 *          report event: MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB, data: mible_mesh_config_status_t.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_node_reset(void)
{
    // erase mesh data
    mesh_node_reset();
    return 0;
}

/**
 *@brief    mesh unprovsion done. need update node info and
 *          callback MIBLE_MESH_EVENT_DEVICE_INIT_DONE event
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_unprovsion_done(void)
{
    return mible_reboot();
}

/**
 *@brief    mesh login done.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_login_done(uint8_t status)
{
    MI_LOG_INFO("LOGIN SUCCESS, stop TIMER_ID_CONN_TIMEOUT\n");
    return mible_timer_stop(mible_conn_timer);
}

/**
 *@brief    set local provisioner network transmit params.
 *@param    [in] count : advertise counter for every adv packet, adv transmit times
 *@param    [in] interval_steps : adv interval = interval_steps*0.625ms
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_network_transmit_param(uint8_t count, uint8_t interval_steps)
{
    MI_LOG_WARNING("[mible_mesh_gateway_set_network_transmit_param] \n");
    // TODO: Store in rom
    mesh_node.net_trans_count = count;
    mesh_node.net_trans_steps = interval_steps;
    return 0;
}

/**
 *@brief    set node relay onoff.
 *@param    [in] enabled : 0: relay off, 1: relay on
 *@param    [in] count: Number of relay transmissions beyond the initial one. Range: 0-7
 *@param    [in] interval: Relay retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_relay(uint8_t enabled, uint8_t count, uint8_t interval)
{
    mesh_node.features.relay = enabled;
    mesh_node.relay_retrans_count = count;
    mesh_node.relay_retrans_steps = interval;
    return 0;
}

/**
 *@brief    get node relay state.
 *@param    [out] enabled : 0: relay off, 1: relay on
 *@param    [out] count: Number of relay transmissions beyond the initial one. Range: 0-7
 *@param    [out] interval: Relay retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_relay(uint8_t *enabled, uint8_t *count, uint8_t *step)
{
    *enabled = mesh_node.features.relay;
    *count = mesh_node.relay_retrans_count;
    *step = mesh_node.relay_retrans_steps;

    MI_LOG_WARNING("relay\t stat %d\t cnt %d\t interval %d\n", *enabled, *count, (*step+1)*10);
    return 0;
}

/**
 *@brief    get seq number.
 *@param    [in] element : model element
 *@param    [out] seq : current sequence numer
 *@param    [out] iv_index : current IV Index
 *@param    [out] flags : IV Update Flag
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_seq(uint16_t element, uint32_t* seq, uint32_t* iv, uint8_t* flags)
{
    if (PROV_NODE == mesh_node.node_state){
        if(NULL != seq)
            *seq = mesh_node.seq;
        if(NULL != iv)
            *iv = mesh_node.iv_index;
        if(NULL != flags)
            *flags = mesh_node.iv_update_flag;
        MI_LOG_DEBUG("Seq %u, Iv %u, flags %u\n", mesh_node.seq,
                        mesh_node.iv_index, mesh_node.iv_update_flag);
        return 0;
    }
    return -1;
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
    /* store iv index */
    iv_index_set(iv_index);
    mesh_flash_store(MESH_FLASH_PARAMS_IV_INDEX, NULL);
    iv_index_timer_start();
    mesh_node.iv_update_flag = (flags & 0x02) >> 1;
    mesh_node.iv_timer_count = MESH_IV_INDEX_48W; //!< should stay (MESH_IV_INDEX_48W)
    mesh_seq_clear();
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
    uint8_t key[16];
    memcpy(key, netkey, sizeof(key));
    if(op == MIBLE_MESH_OP_ADD){
        /* store netkey */
        uint16_t net_key_index = 0;
        net_key_update(net_key_index, netkey_index, key);
        mesh_flash_store(MESH_FLASH_PARAMS_NET_KEY, &net_key_index);
#if !MINIMIZE_FLASH_SIZE
    }else{
        uint16_t net_key_index = net_key_index_from_global(netkey_index);
        net_key_delete(net_key_index);
        mesh_flash_store(MESH_FLASH_PARAMS_NET_KEY, &net_key_index);
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
    uint8_t key[16];
    memcpy(key, appkey, sizeof(key));
    if(op == MIBLE_MESH_OP_ADD){
        uint16_t net_key_index = net_key_index_from_global(netkey_index);
        uint16_t app_key_index = app_key_add(net_key_index, appkey_index, key);
        mesh_flash_store(MESH_FLASH_PARAMS_APP_KEY, &app_key_index);
#if !MINIMIZE_FLASH_SIZE
    }else{
        uint16_t app_key_index = app_key_index_from_global(appkey_index);
        app_key_delete(app_key_index);
        mesh_flash_store(MESH_FLASH_PARAMS_APP_KEY, &app_key_index);
#endif
    }
    return 0;
}

static int local_model_find(uint16_t elem_index, uint16_t company_id, uint16_t model_id, mesh_model_p *ppmodel)
{
    mesh_element_p pelement = mesh_element_get(elem_index);
    if (NULL == pelement){
        MI_LOG_ERROR("local_model_find: invalid element index(%d)", elem_index);
        return -1;
    }
    uint32_t model = model_id;
    if(company_id == 0 || company_id == MIBLE_MESH_COMPANY_ID_SIG){
        /* sig model */
        model = MESH_MODEL_TRANSFORM(model);
    }else if(company_id == MIBLE_MESH_COMPANY_ID_XIAOMI){
        /* vendor model */
        model <<= 16;
        model |= company_id;
    }else{
        MI_LOG_ERROR("local_model_find: invalid model id(0x%04x-0x%04x-%d)", model_id, company_id, elem_index);
        return -2;
    }
    
    mesh_model_p pmodel = mesh_model_get_by_model_id(pelement, model);
    if (NULL == pmodel)
    {
        MI_LOG_ERROR("local_model_find: invalid model(0x%08x-%d)", model, elem_index);
        return -3;
    }
    
    *ppmodel = pmodel;
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
    
    mesh_model_p pmodel;
    result = local_model_find(elem_index, company_id, model_id, &pmodel);
    if(result != 0){
        return result;
    }

    uint16_t app_key_index = app_key_index_from_global(appkey_index);
    if (mesh_node.app_key_num == app_key_index)
    {
        MI_LOG_ERROR("mi_app_bind: invalid appkey index(%d-%d)", appkey_index, elem_index);
        return -4;
    }
    
    if(op == MIBLE_MESH_OP_ADD){
        MI_LOG_DEBUG("mi_appkey_bind: bind model(0x%08x-%d) appkey index(%d)", model_id, elem_index, appkey_index);
        mesh_model_bind_one(pmodel->pmodel_info, app_key_index, TRUE);   
#if !MINIMIZE_FLASH_SIZE
    }else{
        MI_LOG_DEBUG("mi_appkey_bind: unbind model(0x%08x-%d) appkey index(%d)", model_id, elem_index, appkey_index);
        mesh_model_bind_one(pmodel->pmodel_info, app_key_index, FALSE);
#endif
    }
    
    mesh_flash_store(MESH_FLASH_PARAMS_MODEL_APP_KEY_BINDING, pmodel);
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
    mesh_model_p pmodel;
    result = local_model_find(element, company_id, model_id, &pmodel);
    if(result != 0){
        return result;
    }
    
    if(op == MIBLE_MESH_OP_ADD){
        MI_LOG_DEBUG("mi_sub_address: sub model(0x%08x-%d) group address(%d)", model_id, element, sub_addr);
        mesh_model_sub(pmodel, sub_addr);
    }else{
        MI_LOG_DEBUG("mi_sub_address: unsub model(0x%08x-%d) group address(%d)", model_id, element, sub_addr);
        mesh_model_unsub(pmodel, sub_addr);
    }
    mesh_flash_store(MESH_FLASH_PARAMS_MODEL_SUBSCRIBE_ADDR, pmodel);
    return 0;
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
    /* default support 12*8 segment message, can be modified*/
    uint8_t msg[96];
    uint32_t opcode;
    uint16_t model_id = 0;
    
    if(param->opcode.company_id == MIBLE_MESH_COMPANY_ID_SIG){
        opcode = param->opcode.opcode;
        switch(opcode){
        case MIBLE_MESH_MSG_GENERIC_ONOFF_STATUS:
            model_id = MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER;
            break;
        case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_STATUS:
            model_id = MIBLE_MESH_MODEL_ID_LIGHTNESS_SERVER;
            break;
        case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_STATUS:
            model_id = MIBLE_MESH_MODEL_ID_CTL_TEMPEATURE_SERVER;
            break;
        default:
            MI_LOG_ERROR("mi_generic_control: invalid opcode (0x%04x-0x%04x-%d)", param->opcode.opcode,
                    param->opcode.company_id, param->meta_data.elem_index);
            return -4;
        }
    }else if(param->opcode.company_id == MIBLE_MESH_COMPANY_ID_XIAOMI){
        opcode = ((param->opcode.opcode & 0xff) << 16) + 
                    ((param->opcode.company_id & 0x00ff) << 8) + 
                    ((param->opcode.company_id & 0xff00) >> 8);
    }else{
        MI_LOG_ERROR("mi_generic_control: invalid opcode (0x%04x-0x%04x-%d)", param->opcode.opcode,
                    param->opcode.company_id, param->meta_data.elem_index);
        return -4;
    }
    ACCESS_OPCODE_BYTE(msg, opcode);
    memcpy(msg + ACCESS_OPCODE_SIZE(opcode), param->buf, param->buf_len);

    mesh_model_p pmodel;
    result = local_model_find(param->meta_data.elem_index, param->opcode.company_id, model_id, &pmodel);
    if(result != 0){
        return result;
    }
    
    mesh_msg_t mesh_msg;
    mesh_msg.pmodel_info = pmodel->pmodel_info;
    access_cfg(&mesh_msg);
    mesh_msg.pbuffer = msg;
    mesh_msg.msg_len = param->buf_len + ACCESS_OPCODE_SIZE(opcode);
    mesh_msg.dst = param->meta_data.dst_addr;
    mesh_msg.app_key_index = param->meta_data.appkey_index;
    mesh_msg.delay_time = 0;
    
    MI_LOG_DEBUG("Generic message send. src %04x, dst %04x, opcode %06x, data:\n",
                param->meta_data.src_addr, param->meta_data.dst_addr, opcode);
    MI_LOG_HEXDUMP(mesh_msg.pbuffer, mesh_msg.msg_len);
    
    result = access_send(&mesh_msg);
    
    return result;
}

/**
 *@brief    get system time.
 *@return   systicks in ms.
 */
uint64_t mible_mesh_get_exact_systicks(void)
{
    //TODO: need 1s poll timer
    uint32_t ticks = RTC_GetCounter() - rtc_cnt;
    //MI_LOG_DEBUG("mible_mesh_get_exact_systicks system %d, ticks %d\n", systime*1000, ticks);
    return (uint64_t)systime*1000 + ticks;
}

void mi_inner_msg_handle(uint8_t event)
{
    if (event != mi_event)
    {
        MI_LOG_ERROR("mi_inner_msg_handle: fail, event(%d) is not mesh event(%d)!", event, mi_event);
        return;
    }

    mi_inner_msg_t inner_msg, *pmsg;
    if (FALSE == plt_os_queue_receive(mi_queue_handle, &inner_msg, 0))
    {
        MI_LOG_ERROR("mi_inner_msg_handle: fail to receive msg!");
        return;
    }

    pmsg = &inner_msg;
    switch (pmsg->type)
    {
    case MI_SCHD_PROCESS:
        mi_schd_process();
        mible_mesh_device_main_thread();
        break;
    default:
        MI_LOG_WARNING("mi_inner_msg_handle: fail, mi queue message unknown type: %d!", pmsg->type);
        break;
    }
}

bool mi_inner_msg_send(mi_inner_msg_t *pmsg)
{
    static uint32_t error_num = 0;

    if (!plt_os_queue_send(mi_queue_handle, pmsg, 0))
    {
        if (++error_num % 20 == 0)
        {
            MI_LOG_ERROR("failed to send msg to mi msg queue");
        }
        return FALSE;
    }

    /* send event to notify app task */
    if (!plt_os_queue_send(mi_event_queue_handle, &mi_event, 0))
    {
        if (++error_num % 20 == 0)
        {
            MI_LOG_ERROR("failed to send msg to mi event queue");
        }
        return FALSE;
    }

    return TRUE;
}

void mi_mesh_start(uint8_t event_mi, void *event_queue)
{
    mi_event = event_mi;
    mi_event_queue_handle = event_queue;
    mi_queue_handle = plt_os_queue_create(MI_INNER_MSG_NUM, sizeof(mi_inner_msg_t));
}

/* RTC parameters */
#define RTC_PRESCALER_VALUE     (32-1)//f = 1000Hz
/* RTC has four comparators.0~3 */
#define RTC_COMP_INDEX          1
#define RTC_INT_CMP_NUM         RTC_INT_CMP1
#define RTC_COMP_VALUE          (10)

/**
  * @brief  Initialize rtc peripheral.
  * @param   No parameter.
  * @return  void
  */
void driver_rtc_init(void)
{
    RTC_DeInit();
    RTC_SetPrescaler(RTC_PRESCALER_VALUE);

    RTC_SetComp(RTC_COMP_INDEX, RTC_COMP_VALUE);
    RTC_MaskINTConfig(RTC_INT_CMP_NUM, DISABLE);
    RTC_CompINTConfig(RTC_INT_CMP_NUM, ENABLE);

    /* Config RTC interrupt */
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_InitStruct.NVIC_IRQChannel = RTC_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPriority = 3;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStruct);

    RTC_RunCmd(ENABLE);
}

void RTC_Handler(void)
{
    if (RTC_GetINTStatus(RTC_INT_CMP_NUM) == SET)
    {
        /* Notes: DBG_DIRECT function is only used for debugging demonstrations, not for application projects.*/
        mi_inner_msg_t msg;
        msg.type = MI_SCHD_PROCESS;
        mi_inner_msg_send(&msg);
        //DBG_DIRECT("[main]RTC_Handler: RTC counter current value = %d", RTC_GetCounter());
        RTC_SetComp(RTC_COMP_INDEX, (RTC_GetCounter() + RTC_COMP_VALUE) & 0xffffff);
        RTC_ClearCompINT(RTC_COMP_INDEX);
    }
}

/**
 * @brief initialize mi gatt process task
 */
void mi_gatts_init(void)
{
    driver_rtc_init();
}

void mi_gatts_suspend(void)
{
    RTC_RunCmd(DISABLE);
    MI_LOG_INFO("RTC disabled!");
}

void mi_gatts_resume(void)
{
    RTC_RunCmd(ENABLE);
    MI_LOG_INFO("RTC enabled!");
}

void mi_startup_delay(void)
{
    if (PROV_NODE == mesh_node.node_state)
    {
        int delay = rand();
        uint32_t real_delay = delay;
        real_delay %= 2000;
        DBG_DIRECT("startup delay: %d ms", real_delay);
        plt_delay_ms(real_delay);
#if (ROM_WATCH_DOG_ENABLE == 1)
        WDG_Restart();
#endif
    }
}

static void poll_second_timeout_handle(void *pargs)
{
    UNUSED(pargs);
    systime ++;
    rtc_cnt = RTC_GetCounter();
    MI_LOG_DEBUG("systime: %u\n!", (uint32_t)systime);
}

static void mible_conn_timeout_cb(void *p_context)
{
    MI_LOG_INFO("[mible_conn_timeout_cb]disconnect handle: %04x\r\n", mible_conn_handle);
    if (0xFFFF != mible_conn_handle)
    {
        mible_gap_disconnect(mible_conn_handle);
    }
}

void mi_handle_gap_msg(T_IO_MSG *pmsg)
{
    T_LE_GAP_MSG gap_msg;
    memcpy(&gap_msg, &pmsg->u.param, sizeof(pmsg->u.param));

    switch (pmsg->subtype)
    {
    case GAP_MSG_LE_DEV_STATE_CHANGE:
        if (!mible_start)
        {
            mible_start = TRUE;

            T_GAP_VENDOR_PRIORITY_PARAM pri_param;
            memset(&pri_param, 0, sizeof(T_GAP_VENDOR_PRIORITY_PARAM));
            pri_param.set_priority_mode = GAP_VENDOR_SET_PRIORITY;
            pri_param.link_priority_mode = GAP_VENDOR_SET_ALL_LINK_PRIORITY;
            pri_param.link_priority_level = GAP_VENDOR_PRIORITY_LEVEL_10;
            pri_param.scan_priority.set_priority_flag = true;
            pri_param.scan_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_0;
            pri_param.adv_priority.set_priority_flag = true;
            pri_param.adv_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_5;
            pri_param.initiate_priority.set_priority_flag = false;
            pri_param.initiate_priority.priority_level = GAP_VENDOR_RESERVED_PRIORITY;
            le_vendor_set_priority(&pri_param);
        
            //TODO: HAVE_MSC
            //mi_scheduler_init(MI_SCHEDULER_INTERVAL, mi_schd_event_handler, &lib_cfg);
            //mible_record_init();
            mi_gatts_init();

            mible_mesh_event_callback(MIBLE_MESH_EVENT_STACK_INIT_DONE, NULL);
            if(is_initialized){
                process_mesh_node_init_event();
            }
            
            poll_second_timer = plt_timer_create("poll_second", 1000, TRUE, 0, poll_second_timeout_handle);
            plt_timer_start(poll_second_timer, 0);
        }
        break;
    case GAP_MSG_LE_CONN_STATE_CHANGE:
        {
            T_GAP_CONN_STATE conn_state = (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state;
            uint8_t conn_id = gap_msg.msg_data.gap_conn_state_change.conn_id;
            MI_LOG_DEBUG("mible_handle_gap_msg: conn_id = %d, new_state = %d, cause = %d",
                         conn_id, conn_state, gap_msg.msg_data.gap_conn_state_change.disc_cause);
            if (conn_id >= GAP_SCHED_LE_LINK_NUM)
            {
                MI_LOG_WARNING("mible_handle_gap_msg: exceed the maximum supported link num %d",
                               GAP_SCHED_LE_LINK_NUM);
                break;
            }

            mible_gap_evt_param_t param;
            memset(&param, 0, sizeof(param));
            param.conn_handle = conn_id;
            if (conn_state == GAP_CONN_STATE_CONNECTED)
            {
                mi_gatts_resume();
                T_GAP_CONN_INFO conn_info;
                le_get_conn_info(conn_id, &conn_info);
                memcpy(param.connect.peer_addr, conn_info.remote_bd, GAP_BD_ADDR_LEN);
                param.connect.type = (mible_addr_type_t)conn_info.remote_bd_type;
                param.connect.role = conn_info.role == GAP_LINK_ROLE_MASTER ? MIBLE_GAP_PERIPHERAL :
                                     MIBLE_GAP_CENTRAL;
                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.connect.conn_param.min_conn_interval, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.connect.conn_param.max_conn_interval, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_LATENCY, &param.connect.conn_param.slave_latency, conn_id);
                le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &param.connect.conn_param.conn_sup_timeout, conn_id);

                mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &param);
                if (NULL == mible_conn_timer)
                {
                    mible_status_t status = mible_timer_create(&mible_conn_timer, mible_conn_timeout_cb,
                                                               MIBLE_TIMER_SINGLE_SHOT);
                    if (MI_SUCCESS != status)
                    {
                        MI_LOG_ERROR("mible_conn_timer: fail, timer is not created");
                        return;
                    }
                    else
                    {
                        MI_LOG_DEBUG("mible_conn_timer: succ, timer is created");
                    }
                }
                mible_conn_handle = conn_id;
                mible_timer_start(mible_conn_timer, MI_GATT_TIMEOUT, NULL);
            }
            else if (conn_state == GAP_CONN_STATE_DISCONNECTED)
            {
                uint16_t disc_cause = gap_msg.msg_data.gap_conn_state_change.disc_cause;
                if (disc_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT))
                {
                    param.disconnect.reason = CONNECTION_TIMEOUT;
                }
                else if (disc_cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
                {
                    param.disconnect.reason = REMOTE_USER_TERMINATED;
                }
                else if (disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
                {
                    param.disconnect.reason = LOCAL_HOST_TERMINATED;
                }

                mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNECT, &param);
                mible_conn_handle = 0xFFFF;
                mible_timer_stop(mible_conn_timer);
            }
        }
        break;
    case GAP_MSG_LE_CONN_PARAM_UPDATE:
        if (GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS == gap_msg.msg_data.gap_conn_param_update.status)
        {
            uint8_t conn_id = gap_msg.msg_data.gap_conn_param_update.conn_id;
            mible_gap_evt_param_t param;
            memset(&param, 0, sizeof(param));
            param.conn_handle = conn_id;

            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.update_conn.conn_param.min_conn_interval,
                              conn_id);
            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.update_conn.conn_param.max_conn_interval,
                              conn_id);
            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &param.update_conn.conn_param.slave_latency, conn_id);
            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &param.update_conn.conn_param.conn_sup_timeout, conn_id);
            mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &param);
        }
        break;
    default:
        break;
    }
}

/**
 * @brief get device id
 * @param[in] opcode: command opcode
 * @param[in] pdata: command data
 * @param[in] len: command length
 */
void get_device_id(uint8_t *pdid)
{
#if (HAVE_OTP_PKI)
    uint8_t buf[512];
    int buf_len;
    msc_crt_t crt;

    buf_len = mi_mesh_otp_read(OTP_DEV_CERT, buf, sizeof(buf));
    if (buf_len > 0)
    {
        mi_crypto_crt_parse_der(buf, buf_len, NULL, &crt);
        if (crt.sn.len <= 8)
        {
            for (int i = 0; i < crt.sn.len; i++)
            {
                pdid[crt.sn.len - 1 - i] = crt.sn.p[i];
            }
        }
    }
#endif
}

#if HAVE_MSC
static int msc_pwr_manage(bool power_stat)
{
    if (power_stat == 1)
    {
        Pad_Config(P0_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_HIGH);
    }
    else
    {
        Pad_Config(P0_4, PAD_SW_MODE, PAD_IS_PWRON, PAD_PULL_NONE, PAD_OUT_ENABLE, PAD_OUT_LOW);
    }
    return 0;
}

static const iic_config_t msc_iic_config =
{
    .scl_pin = P0_5,
    .sda_pin = P0_6,
    .freq = IIC_400K,
};

static mible_libs_config_t lib_cfg =
{
    .msc_onoff = msc_pwr_manage,
    .p_msc_iic_config = (void *) &msc_iic_config,
};
#endif

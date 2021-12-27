#include <stdio.h>
#include <string.h>
#include "mible_mesh_api.h"
#include "mi_config.h"
#include "mible_log.h"
#include "mible_api.h"

//#include <trace.h>
#include "mesh_api.h"
#include "mesh_sdk.h"
#include "mesh_node.h"
#include "health.h"


#include "miot_model.h"
#include "mijia_model.h"

/* default group address */
#define MI_DEFAULT_GROUP_ADDR_NUM                10
#define MI_DEFAULT_PRE_SUB_ADDR_NUM              5
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

static uint8_t is_provisioned = 0;
//static uint8_t conn_handle = 0xFF;        /* handle of the last opened LE connection */
/* mible state, avoid multiple initialize */
static bool mible_start = FALSE;
static bool is_initialized = false;
static bool is_prov_complete = false;
/* connect paramenters */
static uint16_t mible_conn_handle = 0xFFFF;
/* connect timer */
static void *mible_conn_timer = NULL;

/* pre subscribe address */
#if defined(MI_MESH_TEMPLATE_LIGHTNESS) || defined(MI_MESH_TEMPLATE_LIGHTCTL)
static uint16_t pre_sub_addr[] = {0xFE00};
#elif defined(MI_MESH_TEMPLATE_ONE_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_TWO_KEY_SWITCH) || defined(MI_MESH_TEMPLATE_THREE_KEY_SWITCH)
/* low power node */
#if MI_MESH_LOW_POWER_NODE
static uint16_t pre_sub_addr[] = {0xFE01, 0xFE41};
#else
static uint16_t pre_sub_addr[] = {0xFE01};
#endif /* MI_MESH_LOW_POWER_NODE */
#elif defined(MI_MESH_TEMPLATE_FAN)
static uint16_t pre_sub_addr[] = {0xFE03};
#else
static uint16_t pre_sub_addr[] = {};
#endif /* MI_MESH_TEMPLATE */
//static uint16_t pre_sub_addr_cnt = 0;

extern mible_status_t mible_record_init(void);
extern T_GAP_CAUSE le_vendor_set_priority(T_GAP_VENDOR_PRIORITY_PARAM *p_priority_param);

static void process_mesh_node_init_event(void)
{
    mible_mesh_template_map_t node_map[5] = {
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
    };
    mible_mesh_node_init_t node_info;
    node_info.map = (mible_mesh_template_map_t *)&node_map;
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
        for(int i=0; i<sizeof(pre_sub_addr)/2; i++)
            mible_mesh_device_set_presub_address(MIBLE_MESH_OP_ADD, pre_sub_addr[i]);

        /** switch beacon & service adv */
        if (mesh_node.features.snb)
        {
            beacon_start();
        }
        break;
    case PROV_CB_TYPE_FAIL:
        break;
    case PROV_CB_TYPE_PROV:
        /* stack ready */
        is_provisioned = true;
        is_initialized = true;
        rpl_clear(); //clear rpl after reboot
        for(int i=0; i<sizeof(pre_sub_addr)/2; i++)
            mible_mesh_device_set_presub_address(MIBLE_MESH_OP_ADD, pre_sub_addr[i]);
        break;
    default:
        break;
    }
    return true;
}

bool iv_index_cb(iv_index_cb_type_t type, iv_index_cb_data_t data)
{
    MI_LOG_INFO("iv_index_cb: type = %d", type);
    switch(type)
    {
        case MESH_IV_INDEX_UPDATE:{
                MI_LOG_INFO("iv index update: iv_index = 0x%x, iv_update_flag = %d", data.iv_index, data.iv_index_update_flag);
                mible_mesh_event_params_t evt_vendor_param;
                evt_vendor_param.mesh_iv.iv_index = data.iv_index;
                evt_vendor_param.mesh_iv.flags = data.iv_index_update_flag;
                mible_mesh_event_callback(MIBLE_MESH_EVENT_IV_UPDATE, &evt_vendor_param);
            }
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

/**
 *@brief    generic message, Mesh model 3.2, 4.2, 5.2, 6.3, or 7 Summary.
 *          report event: MIBLE_MESH_EVENT_GENERIC_OPTION_CB, data: mible_mesh_access_message_t.
 *@param    [in] param : control parameters corresponding to node
 *          according to opcode, generate a mesh message; extral params: ack_opcode, tid, get_or_set.
 *@return   0: success, negetive value: failure
 */
int rtk_mesh_msg_relay(mesh_msg_t *pargs)
{
    pargs->ttl -= 1;
    pargs->net_trans_count = 7;
    pargs->net_trans_steps = 0;
    MI_LOG_DEBUG("rtk_mesh_msg_relay. src %04x, dst %04x, opcode %06x, data:\n",
                pargs->src, pargs->dst, pargs->access_opcode);
    MI_LOG_HEXDUMP(pargs->pbuffer, pargs->msg_len);
    
    return access_send(pargs);
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
        evt_generic_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
        memcpy(evt_generic_param.generic_msg.buff, pbuffer, evt_generic_param.generic_msg.buf_len);
        
    }
    evt_generic_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_generic_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_generic_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_generic_param.generic_msg.meta_data.elem_index = pmodel_info->element_index;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_generic_param);
    
    /* send relay msg */
#if MI_MESH_LOW_POWER_NODE
    if(pmesh_msg->dst >= 0xC000)
        rtk_mesh_msg_relay(pmesh_msg);
#endif
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
        evt_generic_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
        memcpy(evt_generic_param.generic_msg.buff, pbuffer, evt_generic_param.generic_msg.buf_len);
    }
    evt_generic_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_generic_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_generic_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_generic_param.generic_msg.meta_data.elem_index = pmodel_info->element_index;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_generic_param);
    
    /* send relay msg */
#if MI_MESH_LOW_POWER_NODE
    if(pmesh_msg->dst >= 0xC000)
        rtk_mesh_msg_relay(pmesh_msg);
#endif
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
        evt_generic_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
        memcpy(evt_generic_param.generic_msg.buff, pbuffer, evt_generic_param.generic_msg.buf_len);
    }
    evt_generic_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_SIG;
    evt_generic_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_generic_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_generic_param.generic_msg.meta_data.elem_index = pmodel_info->element_index;
    
    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_generic_param);
    
    /* send relay msg */
#if MI_MESH_LOW_POWER_NODE
    if(pmesh_msg->dst >= 0xC000)
        rtk_mesh_msg_relay(pmesh_msg);
#endif
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
#include "mesh_auth/mible_mesh_vendor.h"
static int32_t miot_server_data(const mesh_model_info_p pmodel_info, uint32_t type,
                         void *pargs)
{
    mesh_msg_t *pmesh_msg = pargs;
    mible_mesh_event_params_t evt_vendor_param;
    
    memset(&evt_vendor_param.generic_msg, 0, sizeof(mible_mesh_access_message_t));
    evt_vendor_param.generic_msg.opcode.opcode = pmesh_msg->access_opcode >> 16;
    
    uint8_t *pbuffer = pmesh_msg->pbuffer + pmesh_msg->msg_offset + ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
#if 0
    static int last_tid_2_1 = -1;
    static int last_tid_3_1 = -1;
    mible_mesh_vendor_message_t *p_vendor = (mible_mesh_vendor_message_t *)pbuffer;
    if(p_vendor->action.siid == 2 && p_vendor->action.aiid == 1){
        if(last_tid_2_1 == p_vendor->action.tid)
            return -1;
        else
            last_tid_2_1 = p_vendor->action.tid;
    }else if(p_vendor->action.siid == 3 && p_vendor->action.aiid == 1){
        if(last_tid_3_1 == p_vendor->action.tid)
            return -1;
        else
            last_tid_3_1 = p_vendor->action.tid;
    }
#endif
    evt_vendor_param.generic_msg.buf_len = pmesh_msg->msg_len - ACCESS_OPCODE_SIZE(pmesh_msg->access_opcode);
    if(evt_vendor_param.generic_msg.buf_len > 8)
        evt_vendor_param.generic_msg.p_buf = pbuffer;
    else
        memcpy(evt_vendor_param.generic_msg.buff, pbuffer, evt_vendor_param.generic_msg.buf_len);
    evt_vendor_param.generic_msg.opcode.company_id = MIBLE_MESH_COMPANY_ID_XIAOMI;
    evt_vendor_param.generic_msg.meta_data.dst_addr = pmesh_msg->dst;
    evt_vendor_param.generic_msg.meta_data.src_addr = pmesh_msg->src;
    evt_vendor_param.generic_msg.meta_data.elem_index = 0;

    mible_mesh_event_callback(MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB, &evt_vendor_param);
    
    /* send relay msg */
#if MI_MESH_LOW_POWER_NODE
    if(pmesh_msg->dst >= 0xC000)
        rtk_mesh_msg_relay(pmesh_msg);
#endif
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
    case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_ADD:
    case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_OVERWRITE:
    case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE:
    case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE_ALL:
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
MS_ACCESS_MODEL_CB         generic_onoff_server_cb;
MS_GENERIC_SERVER_CB       generic_server_appl_cb;
static DECL_CONST UINT32 generic_onoff_server_opcode_list[] =
{
    MS_ACCESS_GENERIC_ONOFF_SET_UNACKNOWLEDGED_OPCODE,
    MS_ACCESS_GENERIC_ONOFF_SET_OPCODE,
    MS_ACCESS_GENERIC_ONOFF_GET_OPCODE,
};

API_RESULT MS_generic_server_register
(
	UINT16 model_id,
	MS_ACCESS_MODEL_CB cb,
	UINT32*  opcodes,
    MS_ACCESS_ELEMENT_HANDLE element_handle,
    /* INOUT */ MS_ACCESS_MODEL_HANDLE* model_handle,
    MS_GENERIC_SERVER_CB appl_cb
)
{
	 API_RESULT retval;
    MS_ACCESS_NODE_ID        node_id;
    MS_ACCESS_MODEL          model;
    /* TBD: Initialize MUTEX and other data structures */
    /* Using default node ID */
    node_id = MS_ACCESS_DEFAULT_NODE_ID;
	 /* Configure Model */
    model.model_id.id = model_id;
    model.model_id.type = MS_ACCESS_MODEL_TYPE_SIG;
    model.elem_handle = element_handle;
    /* Register Callbacks */
    model.cb = cb;  //receive cb
    model.pub_cb = NULL;
    /* List of Opcodes */
    model.opcodes = opcodes;
    model.num_opcodes = sizeof(opcodes) / sizeof(UINT32);
    retval = MS_access_register_model
             (
                 node_id,
                 &model,
                 model_handle
             );
    /* Save Application Callback */
//    generic_onoff_server_appl_cb = appl_cb;
    /* TODO: Remove */
//    generic_onoff_server_model_handle = *model_handle;
	return retval;
}
static int init_models(void)
{
    uint16_t result = 0;

	MS_ACCESS_NODE_ID node_id;
    MS_ACCESS_ELEMENT_DESC   element;
    MS_ACCESS_ELEMENT_HANDLE element_handle;
	MS_ACCESS_ELEMENT_HANDLE element_handle1;
	MS_ACCESS_ELEMENT_HANDLE element_handle2;
    API_RESULT retval;
	
	/* Create Node */
    retval = MS_access_create_node(&node_id);

	/* create elements */
	element.loc = 0x0106;
    retval = MS_access_register_element
             (
                 node_id,
                 &element,
                 &element_handle
              );

	if (API_SUCCESS == retval)
    {
        /* Register foundation model servers */
        retval = UI_register_foundation_model_servers(element_handle);
    }

	//model register 
	//generic onoff example
	if (API_SUCCESS == retval)
    {
    	retval = MS_generic_server_register(MIBLE_MESH_MODEL_ID_GENERIC_ONOFF_SERVER,
			generic_onoff_server_cb,
			generic_onoff_server_opcode_list,
			element_handle,
			&UI_generic_onoff_server_model_handle,
			NULL
			);
    }

    return result;
}

/**
 *@brief    async method, init mesh stack.
 *          report event: MIBLE_MESH_EVENT_STACK_INIT_DONE, data: NULL.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_init_stack(void)
{
	MS_ACCESS_NODE_ID node_id;
    MS_CONFIG* config_ptr;
	API_RESULT retval;
	#ifdef MS_HAVE_DYNAMIC_CONFIG
    MS_CONFIG  config;
    /* Initialize dynamic configuration */
    MS_INIT_CONFIG(config);
    config_ptr = &config;
    #else
    config_ptr = NULL;
	#endif /* MS_HAVE_DYNAMIC_CONFIG */
	
	/* Initialize utilities */
	nvsto_init(NVS_FLASH_BASE1,NVS_FLASH_BASE2);
    /* Initialize Mesh Stack */
    MS_init(config_ptr);

	/* register element and models */
    init_models();
	
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
	return MS_access_cm_set_prov_data(param);
}

/**
 *@brief    mesh provsion done. need update node info and
 *          callback MIBLE_MESH_EVENT_DEVICE_INIT_DONE event
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_provsion_done(void)
{
    is_prov_complete = true;
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
    return MS_common_reset();
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
static void mible_conn_timeout_cb(void *p_context);
int mible_mesh_device_login_done(uint8_t status)
{
    mible_timer_stop(mible_conn_timer);
    if(status){
        MI_LOG_INFO("LOGIN SUCCESS, stop TIMER_ID_CONN_TIMEOUT\n");
    }else{
        MI_LOG_INFO("LOGIN FAIL\n");
        mible_conn_timeout_cb(NULL);
    }
    return 0;
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
	return MS_access_cm_set_transmit_state(MS_NETWORK_TX_STATE,interval_steps);
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
	MS_access_cm_set_features_field(enabled, MS_FEATURE_RELAY);
   	return MS_access_cm_set_transmit_state(MS_RELAY_TX_STATE,interval);
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
	return MS_access_cm_get_transmit_state(MS_RELAY_TX_STATE,step);
}

/**
 *@brief    set node relay onoff.
 *@param    [in] ttl : Time To Live
 *@param    [in] count: Number of net transmissions beyond the initial one. Range: 0-7
 *@param    [in] interval: net retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_nettx(uint8_t ttl, uint8_t count, uint8_t interval)
{
	API_RESULT retval;
	retval = MS_access_cm_set_default_ttl(ttl);
	if(retval == API_SUCCESS)
		return MS_access_cm_set_transmit_state(MS_NETWORK_TX_STATE,interval);
}

/**
 *@brief    get node relay state.
 *@param    [out] ttl : Time To Live
 *@param    [out] count: Number of net transmissions beyond the initial one. Range: 0-7
 *@param    [out] interval: net retransmit interval steps. 10*(1+steps) milliseconds. Range: 0-31.
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_nettx(uint8_t *ttl, uint8_t *count, uint8_t *step)
{
	API_RESULT retval;
	retval = MS_access_cm_get_default_ttl(ttl);
	if(retval == API_SUCCESS)
		return MS_access_cm_get_transmit_state(MS_NETWORK_TX_STATE,step);
}

/**
 *@brief    set seq number.
 *@param    [in] element : model element
 *@param    [in] seq : current sequence numer
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_set_seq(uint16_t element, uint32_t seq)
{
	net_seq_number_state.seq_num = seq;
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
	 if(NULL != seq)
    	*seq = net_seq_number_state.seq_num;
     return MS_access_cm_get_iv_index(iv,flags);  
}

int mible_mesh_device_snb_start(bool enable)
{
	{
		MS_ENABLE_SNB_FEATURE();
		MS_net_start_snb_timer(0);		
	}
	else
	{
		MS_DISABLE_SNB_FEATURE();
        MS_net_stop_snb_timer(0);
	}
    return 0;
}

/**
 *@brief    update iv index, .
 *@param    [in] iv_index : current IV Index
 *@param    [in] flags : contains the Key Refresh Flag and IV Update Flag
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_update_iv_info(uint32_t iv_index, uint8_t flags)
{
//    MI_LOG_WARNING("[mible_mesh_gateway_update_iv_info]  \n");
	return MS_access_cm_set_iv_index(iv_index,flags);
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
	API_RESULT retval;
	MS_SUBNET_HANDLE subnet;
	UINT32 opcode;
	UINT32 index, add_index;
	UCHAR	match_found = MS_FALSE;
	
	//op add/update
	if(op == MIBLE_MESH_OP_ADD)
	{
		add_index = MS_CONFIG_LIMITS(MS_MAX_SUBNETS);
		 for (index = 0; index < MS_CONFIG_LIMITS(MS_MAX_SUBNETS); index ++)
		{
			if ((MS_IS_SUBNET_FREE(index)) && (MS_CONFIG_LIMITS(MS_MAX_SUBNETS) == add_index))
			{
				/* Save first free entry */
				add_index = index;
			}
			else
			{
				/* Check if NetKeyIndex matches */
				if (netkey_index == MS_SUBNET_NETKEY_INDEX(index))
				{
					ACCESS_TRC(
						"[CM] NetKeyIndex already present in subnet handle 0x%04X\n", index);
					match_found = MS_TRUE;
					/* Save subnet handle */
					add_index = index;
					break;
				}
			}
		}
		if (MS_TRUE == match_found)
		{
			opcode = MS_ACCESS_CONFIG_NETKEY_ADD_OPCODE;
		}
		else
		{
			opcode = MS_ACCESS_CONFIG_NETKEY_UPDATE_OPCODE;
		}
		return MS_access_cm_add_update_netkey(netkey_index,opcode,netkey); 
	}
	else if(op == MIBLE_MESH_OP_DELETE) //del
	{
		retval = MS_access_cm_find_subnet(netkey_index,&subnet);
		if(retval == API_SUCCESS)
			return MS_access_cm_delete_netkey(subnet);
		
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
int mible_mesh_device_set_appkey(mible_mesh_op_t op, uint16_t netkey_index, uint16_t appkey_index, uint8_t * appkey)
{
    MI_LOG_WARNING("[mible_mesh_gateway_set_appkey] \n");
	API_RESULT retval;
	MS_SUBNET_HANDLE subnet;
	retval = MS_access_cm_find_subnet(netkey_index,&subnet);
    if(op == MIBLE_MESH_OP_ADD){
        // add app_key, bind netkey index       
		if(retval == API_SUCCESS)
			return MS_access_cm_add_appkey(subnet,appkey_index,appkey);
    }else{
    	// delete netkey, unbind netkey index
    	if(retval == API_SUCCESS)
    	{
    		 UINT32 opcode;
			 opcode = MS_ACCESS_CONFIG_APPKEY_DELETE_OPCODE;
			 return MS_access_cm_update_delete_appkey(subnet,appkey_index,opcode,appkey);
		}   
    }
}

//int local_model_find(uint16_t elem_index, uint16_t company_id, uint16_t model_id, mesh_model_p *ppmodel)
//{
//    mesh_element_p pelement = mesh_element_get(elem_index);
//    if (NULL == pelement){
//        MI_LOG_ERROR("local_model_find: invalid element index(%d)", elem_index);
//        return -1;
//    }
//    uint32_t model = model_id;
//    if(company_id == 0 || company_id == MIBLE_MESH_COMPANY_ID_SIG){
//        /* sig model */
//        model = MESH_MODEL_TRANSFORM(model);
//    }else if(company_id == MIBLE_MESH_COMPANY_ID_XIAOMI){
//        /* vendor model */
//        model <<= 16;
//        model |= company_id;
//    }else{
//        MI_LOG_ERROR("local_model_find: invalid model id(0x%04x-0x%04x-%d)", model_id, company_id, elem_index);
//        return -2;
//    }
//    
//    mesh_model_p pmodel = mesh_model_get_by_model_id(pelement, model);
//    if (NULL == pmodel)
//    {
//        MI_LOG_ERROR("local_model_find: invalid model(0x%08x-%d)", model, elem_index);
//        return -3;
//    }
//    
//    *ppmodel = pmodel;
//    return 0;
//}

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
   API_RESULT retval;
	MS_ACCESS_MODEL_HANDLE model_handle;
	retval = MS_access_get_model_handle(elem_index,model_id,&model_handle);
    if(op == MIBLE_MESH_OP_ADD){
        // bind model appkey_index
        return MS_access_bind_model_app(model_handle,appkey_index);
    }
	else
	{
        // unbind model appkey_index
        return MS_access_unbind_model_app(model_handle,appkey_index);
    }
}

int mible_mesh_device_set_presub_address(mible_mesh_op_t op, uint16_t sub_addr)
{
    if(op == MIBLE_MESH_OP_ADD){
        // add subscription
    }else{
        // delete subscription
    }
    return 0;
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
    API_RESULT retval;
	MS_ACCESS_MODEL_HANDLE model_handle;
	MS_ACCESS_ADDRESS       subp_addr;
	subp_addr.addr = sub_addr;
	retval = MS_access_get_model_handle(element,model_id,&model_handle);
    if(op == MIBLE_MESH_OP_ADD)
	{
        // add subscription
        return MS_access_cm_add_model_subscription(model_handle,&subp_addr);
        
    }
	else if(op == MIBLE_MESH_OP_DELETE)
	{
        // delete subscription
        return MS_access_cm_delete_model_subscription(model_handle,&subp_addr);
    }
	else if(op == MIBLE_MESH_OP_DELETE_ALL)
	{
        return MS_access_cm_delete_all_model_subscription(model_handle);
    }
}

/**
 *@brief    get subscription params.
 *@param    [out] *addr : group_address
 *@param    [in] max_count: the count of addr to put the group address
 *@return   0: success, negetive value: failure
 */
int mible_mesh_device_get_sub_address(uint16_t *addr, uint16_t max_count)
{
    MS_ACCESS_MODEL_HANDLE model_handle;
	for(model_handle=0; model_handle < MS_CONFIG_LIMITS(MS_ACCESS_MODEL_COUNT);model_handle++)
	{
		MS_access_cm_get_model_subscription_list(model_handle,&max_count,addr);
	}
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
    //access_send_pdu
	API_RESULT retval;
	UCHAR* pdata;
	MS_SUBNET_HANDLE sub_handle;

	if(param->buf_len > 8)
        memcpy(pdata, param->p_buf, param->buf_len);
    else
        memcpy(pdata, param->buff, param->buf_len);
	retval = MS_access_cm_find_subnet(param->meta_data.netkey_index,&sub_handle);
	
	if(retval == API_SUCCESS)	
		MS_access_send_pdu
		(
			param->meta_data.src_addr,
			param->meta_data.dst_addr,
			sub_handle,
			param->meta_data.appkey_index,
			param->meta_data.ttl, //default ttl set
			param->opcode.opcode,
			pdata,
			param->buf_len,
			MS_FALSE
		);
	
    return 0;
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
    //RTC_RunCmd(DISABLE);
    //MI_LOG_INFO("RTC disabled!");
}

void mi_gatts_resume(void)
{
    //RTC_RunCmd(ENABLE);
    //MI_LOG_INFO("RTC enabled!");
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

static void mible_conn_timeout_cb(void *p_context)
{
    MI_LOG_INFO("[mible_conn_timeout_cb]disconnect handle: %04x\r\n", mible_conn_handle);
    if (0xFFFF != mible_conn_handle)
    {
        mible_gap_disconnect(mible_conn_handle);
    }
}

//void mi_handle_gap_msg(T_IO_MSG *pmsg)
//{
//    T_LE_GAP_MSG gap_msg;
//    memcpy(&gap_msg, &pmsg->u.param, sizeof(pmsg->u.param));
//
//    switch (pmsg->subtype)
//    {
//    case GAP_MSG_LE_DEV_STATE_CHANGE:
//        if (!mible_start)
//        {
//            mible_start = TRUE;
//
//            T_GAP_VENDOR_PRIORITY_PARAM pri_param;
//            memset(&pri_param, 0, sizeof(T_GAP_VENDOR_PRIORITY_PARAM));
//            pri_param.set_priority_mode = GAP_VENDOR_SET_PRIORITY;
//            pri_param.link_priority_mode = GAP_VENDOR_SET_ALL_LINK_PRIORITY;
//            pri_param.link_priority_level = GAP_VENDOR_PRIORITY_LEVEL_10;
//            pri_param.scan_priority.set_priority_flag = true;
//            pri_param.scan_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_0;
//            pri_param.adv_priority.set_priority_flag = true;
//            pri_param.adv_priority.priority_level = GAP_VENDOR_PRIORITY_LEVEL_5;
//            pri_param.initiate_priority.set_priority_flag = false;
//            pri_param.initiate_priority.priority_level = GAP_VENDOR_RESERVED_PRIORITY;
//            le_vendor_set_priority(&pri_param);
//        
//            //TODO: HAVE_MSC
//            //mi_scheduler_init(MI_SCHEDULER_INTERVAL, mi_schd_event_handler, &lib_cfg);
//            //mible_record_init();
//            mi_gatts_init();
//
//            mible_mesh_event_callback(MIBLE_MESH_EVENT_STACK_INIT_DONE, NULL);
//            if(is_initialized){
//                process_mesh_node_init_event();
//            }
//            
//            mible_systime_utc_set(0);
//        }
//        break;
//    case GAP_MSG_LE_CONN_STATE_CHANGE:
//        {
//            T_GAP_CONN_STATE conn_state = (T_GAP_CONN_STATE)gap_msg.msg_data.gap_conn_state_change.new_state;
//            uint8_t conn_id = gap_msg.msg_data.gap_conn_state_change.conn_id;
//            MI_LOG_DEBUG("mible_handle_gap_msg: conn_id = %d, new_state = %d, cause = %d",
//                         conn_id, conn_state, gap_msg.msg_data.gap_conn_state_change.disc_cause);
//            if (conn_id >= GAP_SCHED_LE_LINK_NUM)
//            {
//                MI_LOG_WARNING("mible_handle_gap_msg: exceed the maximum supported link num %d",
//                               GAP_SCHED_LE_LINK_NUM);
//                break;
//            }
//
//            mible_gap_evt_param_t param;
//            memset(&param, 0, sizeof(param));
//            param.conn_handle = conn_id;
//            if (conn_state == GAP_CONN_STATE_CONNECTED)
//            {
//                //mi_gatts_resume();
//                T_GAP_CONN_INFO conn_info;
//                le_get_conn_info(conn_id, &conn_info);
//                memcpy(param.connect.peer_addr, conn_info.remote_bd, GAP_BD_ADDR_LEN);
//                param.connect.type = (mible_addr_type_t)conn_info.remote_bd_type;
//                param.connect.role = conn_info.role == GAP_LINK_ROLE_MASTER ? MIBLE_GAP_PERIPHERAL :
//                                     MIBLE_GAP_CENTRAL;
//                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.connect.conn_param.min_conn_interval, conn_id);
//                le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.connect.conn_param.max_conn_interval, conn_id);
//                le_get_conn_param(GAP_PARAM_CONN_LATENCY, &param.connect.conn_param.slave_latency, conn_id);
//                le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &param.connect.conn_param.conn_sup_timeout, conn_id);
//
//                mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &param);
//                if (NULL == mible_conn_timer)
//                {
//                    mible_status_t status = mible_timer_create(&mible_conn_timer, mible_conn_timeout_cb,
//                                                               MIBLE_TIMER_SINGLE_SHOT);
//                    if (MI_SUCCESS != status)
//                    {
//                        MI_LOG_ERROR("mible_conn_timer: fail, timer is not created");
//                        return;
//                    }
//                    else
//                    {
//                        MI_LOG_DEBUG("mible_conn_timer: succ, timer is created");
//                    }
//                }
//                mible_conn_handle = conn_id;
//                mible_timer_start(mible_conn_timer, MI_GATT_TIMEOUT, NULL);
//            }
//            else if (conn_state == GAP_CONN_STATE_DISCONNECTED)
//            {
//                uint16_t disc_cause = gap_msg.msg_data.gap_conn_state_change.disc_cause;
//                if (disc_cause == (HCI_ERR | HCI_ERR_CONN_TIMEOUT))
//                {
//                    param.disconnect.reason = CONNECTION_TIMEOUT;
//                }
//                else if (disc_cause == (HCI_ERR | HCI_ERR_REMOTE_USER_TERMINATE))
//                {
//                    param.disconnect.reason = REMOTE_USER_TERMINATED;
//                }
//                else if (disc_cause == (HCI_ERR | HCI_ERR_LOCAL_HOST_TERMINATE))
//                {
//                    param.disconnect.reason = LOCAL_HOST_TERMINATED;
//                }
//
//                mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNECT, &param);
//                mible_conn_handle = 0xFFFF;
//                mible_timer_stop(mible_conn_timer);
//                // prov complete, node init
//                if(is_prov_complete){
//                    is_prov_complete = false;
//                    prov_cb_data_t droppable_data;
//                    droppable_data.pb_generic_cb_type = PB_GENERIC_CB_MSG;
//                    prov_cb(PROV_CB_TYPE_COMPLETE, droppable_data);
//                }
//            }
//        }
//        break;
//    case GAP_MSG_LE_CONN_PARAM_UPDATE:
//        if (GAP_CONN_PARAM_UPDATE_STATUS_SUCCESS == gap_msg.msg_data.gap_conn_param_update.status)
//        {
//            uint8_t conn_id = gap_msg.msg_data.gap_conn_param_update.conn_id;
//            mible_gap_evt_param_t param;
//            memset(&param, 0, sizeof(param));
//            param.conn_handle = conn_id;
//
//            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.update_conn.conn_param.min_conn_interval,
//                              conn_id);
//            le_get_conn_param(GAP_PARAM_CONN_INTERVAL, &param.update_conn.conn_param.max_conn_interval,
//                              conn_id);
//            le_get_conn_param(GAP_PARAM_CONN_LATENCY, &param.update_conn.conn_param.slave_latency, conn_id);
//            le_get_conn_param(GAP_PARAM_CONN_TIMEOUT, &param.update_conn.conn_param.conn_sup_timeout, conn_id);
//            mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &param);
//        }
//        break;
//    default:
//        break;
//    }
//}

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

#if 0
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

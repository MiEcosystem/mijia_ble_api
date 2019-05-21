/* Bluetooth stack headers */
#include "bg_types.h"
#include "native_gecko.h"
#include "gatt_db.h"

#include "mbedtls/aes.h"

#include "efr32_api.h"
#include "em_cmu.h"
#include "em_msc.h"
#include "em_gpio.h"
#include "mible_log.h"
#include "mible_api.h"
#include "efr32_api.h"
#include "stdbool.h"

#include "bg_gattdb_def.h"

#define MAX_TASK_NUM                    4
#define ADV_HANDLE                      0
#define CHAR_TABLE_NUM                  10
#define CHAR_DATA_LEN_MAX               20

// connection context
static struct {
    uint16_t conn_handle;
    mible_gap_connect_t connect;
} m_conn_ctx = {
    .conn_handle = DISCONNECTION
};

// scanning? advertising? .. It could be optimized to use bit mask
uint8_t scanning = 0;
uint8_t advertising = 0;

// All retry caches
uint8_t scan_timeout_for_retry;
uint8_t connect_param_for_retry = 0xFF;

typedef struct {
	uint16_t handle;
	bool rd_author;		// read authorization. Enabel or Disable MIBLE_GATTS_READ_PERMIT_REQ event
	bool wr_author;     // write authorization. Enabel or Disable MIBLE_GATTS_WRITE_PERMIT_REQ event
	uint8_t char_property;
	uint8_t len;
	uint8_t data[CHAR_DATA_LEN_MAX];
} user_char_t;

struct{
	uint8_t num;
	user_char_t item[CHAR_TABLE_NUM];
} m_char_table;

#define TIMER_MAX_NUM 4

typedef struct {
    uint8_t id;
    uint8_t is_avail;
    mible_timer_mode is_single_shot;
    mible_timer_handler handler;
    void * p_ctx;
} timer_item_t;

static timer_item_t m_timer_pool[TIMER_MAX_NUM] = {
    [0] = { .id = 0, .is_avail = 1, },
    [1] = { .id = 1, .is_avail = 1, },
    [2] = { .id = 2, .is_avail = 1, },
    [3] = { .id = 3, .is_avail = 1, },
};

static timer_item_t* acquire_timer()
{
    uint8_t i;
    for (i = 0; i < TIMER_MAX_NUM; i++) {
        if (m_timer_pool[i].is_avail) {
            m_timer_pool[i].is_avail = 0;
            return &m_timer_pool[i];
        }
    }
    return NULL;
}

static int release_timer(void* timer_id)
{
    for (uint8_t i = 0; i < TIMER_MAX_NUM; i++) {
        if (timer_id == &m_timer_pool[i]) {
            m_timer_pool[i].is_avail = 1;
            return i;
        }
    }
    return -1;
}

static int assert_timer(void* timer_id)
{
    for (uint8_t i = 0; i < TIMER_MAX_NUM; i++) {
        if (timer_id == &m_timer_pool[i]) {
            return i;
        }
    }
    return -1;
}

void mible_stack_event_handler(struct gecko_cmd_packet *evt)
{
    uint16_t result;
    mible_gap_evt_param_t gap_evt_param = {0};
    mible_gatts_evt_param_t gatts_evt_param = {0};

    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_le_connection_opened_id:
        m_conn_ctx.connect.role = (mible_gap_role_t) evt->data.evt_le_connection_opened.master;
        if ((evt->data.evt_le_connection_opened.address_type == le_gap_address_type_public) ||
            (evt->data.evt_le_connection_opened.address_type == le_gap_address_type_public_identity)) {
            m_conn_ctx.connect.type = MIBLE_ADDRESS_TYPE_PUBLIC;
        } else {
            m_conn_ctx.connect.type = MIBLE_ADDRESS_TYPE_RANDOM;
        }
        memcpy(m_conn_ctx.connect.peer_addr, evt->data.evt_le_connection_opened.address.addr, 6);
    break;

    case gecko_evt_le_connection_closed_id:
        m_conn_ctx.conn_handle = DISCONNECTION;
        gap_evt_param.conn_handle = evt->data.evt_le_connection_closed.connection;
        if (evt->data.evt_le_connection_closed.reason == bg_err_bt_connection_timeout) {
            gap_evt_param.disconnect.reason = CONNECTION_TIMEOUT;
        } else if (evt->data.evt_le_connection_closed.reason
                == bg_err_bt_remote_user_terminated) {
            gap_evt_param.disconnect.reason = REMOTE_USER_TERMINATED;
        } else if (evt->data.evt_le_connection_closed.reason
                == bg_err_bt_connection_terminated_by_local_host) {
            gap_evt_param.disconnect.reason = LOCAL_HOST_TERMINATED;
        }
        mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNECT, &gap_evt_param);
    break;

    case gecko_evt_le_gap_scan_response_id:
        /* Invalid the connection handle since no connection yet*/
        gap_evt_param.conn_handle = INVALID_CONNECTION_HANDLE;
        memcpy(gap_evt_param.report.peer_addr,
                evt->data.evt_le_gap_scan_response.address.addr, 6);
        gap_evt_param.report.addr_type =
                (mible_addr_type_t) evt->data.evt_le_gap_scan_response.address_type;
        if (evt->data.evt_le_gap_scan_response.packet_type == 0x04) {
            gap_evt_param.report.adv_type = SCAN_RSP_DATA;
        } else {
            gap_evt_param.report.adv_type = ADV_DATA;
        }
        gap_evt_param.report.rssi = evt->data.evt_le_gap_scan_response.rssi;
        memcpy(gap_evt_param.report.data, evt->data.evt_le_gap_scan_response.data.data,
                evt->data.evt_le_gap_scan_response.data.len);
        gap_evt_param.report.data_len = evt->data.evt_le_gap_scan_response.data.len;

        mible_gap_event_callback(MIBLE_GAP_EVT_ADV_REPORT, &gap_evt_param);
    break;

    case gecko_evt_le_connection_parameters_id:
        gap_evt_param.conn_handle =
                evt->data.evt_le_connection_parameters.connection;

        if (m_conn_ctx.conn_handle != DISCONNECTION ) {
            gap_evt_param.update_conn.conn_param.min_conn_interval =
                    evt->data.evt_le_connection_parameters.interval;
            gap_evt_param.update_conn.conn_param.max_conn_interval =
                    evt->data.evt_le_connection_parameters.interval;
            gap_evt_param.update_conn.conn_param.slave_latency =
                    evt->data.evt_le_connection_parameters.latency;
            gap_evt_param.update_conn.conn_param.conn_sup_timeout =
                    evt->data.evt_le_connection_parameters.timeout;

            mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &gap_evt_param);
        } else {
            m_conn_ctx.conn_handle = gap_evt_param.conn_handle;
            gap_evt_param.connect = m_conn_ctx.connect;
            gap_evt_param.connect.conn_param.min_conn_interval =
                    evt->data.evt_le_connection_parameters.interval;
            gap_evt_param.connect.conn_param.max_conn_interval =
                    evt->data.evt_le_connection_parameters.interval;
            gap_evt_param.connect.conn_param.slave_latency =
                    evt->data.evt_le_connection_parameters.latency;
            gap_evt_param.connect.conn_param.conn_sup_timeout =
                    evt->data.evt_le_connection_parameters.timeout;

            mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &gap_evt_param);
        }
    break;

    case gecko_evt_gatt_server_attribute_value_id: {

        uint16_t char_handle = evt->data.evt_gatt_server_attribute_value.attribute;
        mible_gatts_evt_t event;

        gatts_evt_param.conn_handle =
                evt->data.evt_gatt_server_attribute_value.connection;
        gatts_evt_param.write.data =
                evt->data.evt_gatt_server_attribute_value.value.data;
        gatts_evt_param.write.len = evt->data.evt_gatt_server_attribute_value.value.len;
        gatts_evt_param.write.offset = evt->data.evt_gatt_server_attribute_value.offset;
        gatts_evt_param.write.value_handle = char_handle;

        for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
        	if(m_char_table.item[i].handle == char_handle){
        		if(m_char_table.item[i].wr_author == true){
        			event = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
        		}else {
                    event = MIBLE_GATTS_EVT_WRITE;
                }
        		mible_gatts_event_callback(event, &gatts_evt_param);
        		break;
        	}
        }
    }
    break;

    case gecko_evt_gatt_server_user_read_request_id: {

        uint16_t char_handle = 0;
        char_handle = evt->data.evt_gatt_server_user_read_request.characteristic;

        for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){

        	if(m_char_table.item[i].handle == char_handle){

        		if(m_char_table.item[i].rd_author == true){

                    gatts_evt_param.conn_handle =
                            evt->data.evt_gatt_server_user_read_request.connection;

                    gatts_evt_param.read.value_handle =
                            evt->data.evt_gatt_server_user_read_request.characteristic;

                    mible_gatts_event_callback(MIBLE_GATTS_EVT_READ_PERMIT_REQ, &gatts_evt_param);
        		}else{
                    /* Send read response here since no application reaction needed*/
                    if (evt->data.evt_gatt_server_user_read_request.offset>m_char_table.item[i].len) {
                        gecko_cmd_gatt_server_send_user_read_response(
                                evt->data.evt_gatt_server_user_read_request.connection,
                                evt->data.evt_gatt_server_user_read_request.characteristic,
                                (uint8_t)bg_err_att_invalid_offset, 0, NULL);
                    } else {

                        gecko_cmd_gatt_server_send_user_read_response(
                                evt->data.evt_gatt_server_user_read_request.connection,
                                evt->data.evt_gatt_server_user_read_request.characteristic,
                                bg_err_success,
								m_char_table.item[i].len
										- evt->data.evt_gatt_server_user_read_request.offset,
								m_char_table.item[i].data
                                        + evt->data.evt_gatt_server_user_read_request.offset);
                    }
            	}
        		break;
        	}
        }
    }
    break;

    case gecko_evt_gatt_server_user_write_request_id: {

        uint16_t char_handle = evt->data.evt_gatt_server_attribute_value.attribute;
        mible_gatts_evt_t event;

        gatts_evt_param.conn_handle =
                evt->data.evt_gatt_server_attribute_value.connection;
        gatts_evt_param.write.data =
                evt->data.evt_gatt_server_attribute_value.value.data;
        gatts_evt_param.write.len = evt->data.evt_gatt_server_attribute_value.value.len;
        gatts_evt_param.write.offset = evt->data.evt_gatt_server_attribute_value.offset;
        gatts_evt_param.write.value_handle = char_handle;

        for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
        	if(m_char_table.item[i].handle == char_handle){
        		if(m_char_table.item[i].wr_author == true){
        			event = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
        		}else {
                    event = MIBLE_GATTS_EVT_WRITE;
                    if((m_char_table.item[i].char_property & MIBLE_WRITE) != 0){

                    	memcpy(m_char_table.item[i].data + gatts_evt_param.write.offset,
                    			gatts_evt_param.write.data, gatts_evt_param.write.len);

                    	gecko_cmd_gatt_server_send_user_write_response(
                    			evt->data.evt_gatt_server_user_read_request.connection,
								evt->data.evt_gatt_server_user_read_request.characteristic,
                                bg_err_success);
                    }
                }
        		mible_gatts_event_callback(event, &gatts_evt_param);
        		break;
        	}
        }
    }
    break;

    case gecko_evt_gatt_server_characteristic_status_id:

        if (evt->data.evt_gatt_server_characteristic_status.status_flags
                == gatt_server_confirmation) {
            /* Second parameter doesn't have any meaning */
            gatts_evt_param.conn_handle = evt->data.evt_gatt_server_characteristic_status.connection;
            gatts_evt_param.write.value_handle = evt->data.evt_gatt_server_characteristic_status.characteristic;
            mible_gatts_event_callback(MIBLE_GATTS_EVT_IND_CONFIRM, &gatts_evt_param);
        }
    break;

    case gecko_evt_hardware_soft_timer_id: {
        uint8_t handle = evt->data.evt_hardware_soft_timer.handle;
        if (handle == SCAN_TIMEOUT_TIMER_ID) {
            scan_timeout_for_retry = 0;
            mible_gap_scan_stop();
        } else if (handle < MIBLE_TIMER_NUM) {
            m_timer_pool[handle].handler(m_timer_pool[handle].p_ctx);
        }
    }
    break;

    case gecko_evt_system_external_signal_id:
        if (evt->data.evt_system_external_signal.extsignals & START_ADV_RETRY_BIT_MASK) {
            MI_LOG_ERROR("adv retry...\n");
            if (connect_param_for_retry != 0xFF) {
                result =
                gecko_cmd_le_gap_start_advertising(ADV_HANDLE,le_gap_user_data,connect_param_for_retry)->result;
                if (result != bg_err_success) {
                    gecko_external_signal(START_ADV_RETRY_BIT_MASK);
                } else {
                    advertising = 1;
                    connect_param_for_retry = 0xFF;
                }
            }
        }

    break;

    }
    /* Block all stack events */
    return ;
}

/*
 * @brief 	Get BLE mac address.
 * @param 	[out] mac: pointer to data
 * @return  MI_SUCCESS			The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note: 	You should copy gap mac to mac[6]
 * */

mible_status_t mible_gap_address_get(mible_addr_t mac)
{
    struct gecko_msg_system_get_bt_address_rsp_t *ret = gecko_cmd_system_get_bt_address();
    memcpy(mac, ret->address.addr, 6);
    return MI_SUCCESS;
}

/* GAP related function.  You should complement these functions. */

/*
 * @brief	Start scanning
 * @param 	[in] scan_type:	passive or active scaning
 * 			[in] scan_param: scan parameters including interval, windows
 * and timeout
 * @return  MI_SUCCESS             Successfully initiated scanning procedure.
 *          MI_ERR_INVALID_STATE   Has initiated scanning procedure.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * 		    MI_ERR_BUSY 		   The stack is busy, process pending
 * events and retry.
 * @note 	Other default scanning parameters : public address, no
 * whitelist.
 * 	        The scan response is given through
 * MIBLE_GAP_EVT_ADV_REPORT event
 */
mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
        mible_gap_scan_param_t scan_param)
{
    uint16 scan_interval, scan_window;
    uint16_t result;

    if (scanning) {
        return MI_ERR_INVALID_STATE;
    }

    if (scan_type != MIBLE_SCAN_TYPE_PASSIVE ||
        scan_type != MIBLE_SCAN_TYPE_ACTIVE) {
        return MI_ERR_INVALID_PARAM;
    }

    scan_interval = scan_param.scan_interval;
    scan_window = scan_param.scan_window;

    gecko_cmd_le_gap_set_discovery_type(1, (uint8_t)scan_type);
    result = gecko_cmd_le_gap_set_discovery_timing(1, scan_interval, scan_window)->result;
    if (result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    }

    result = gecko_cmd_le_gap_start_discovery(1, le_gap_discover_observation)->result;

    if (result == bg_err_success && scan_param.timeout != 0) {
        gecko_cmd_hardware_set_soft_timer(32768 * scan_param.timeout,
                SCAN_TIMEOUT_TIMER_ID, 1);
    }

    scanning = 1;
    return MI_SUCCESS;
}

/*
 * @brief	Stop scanning
 * @param 	void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */

mible_status_t mible_gap_scan_stop(void)
{
    if (!scanning) {
        return MI_ERR_INVALID_STATE;
    }
    MI_LOG_ERROR("stop scanning\n");
    gecko_cmd_le_gap_end_procedure();
    scanning = 0;
    return MI_SUCCESS;
}

typedef struct {
    uint8_t len;
    uint8_t data[31];
} mible_adv_data_t;

static mible_adv_data_t last_adv_data;
static mible_adv_data_t last_scan_rsp;

/*
 * @brief	Start advertising
 * @param 	[in] p_adv_param : pointer to advertising parameters, see
 * mible_gap_adv_param_t for details
 * @return  MI_SUCCESS             Successfully initiated advertising procedure.
 *          MI_ERR_INVALID_STATE   Initiated connectable advertising procedure
 * when connected.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 * retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again.
 * @note	Other default advertising parameters: local public address , no
 * filter policy
 * */
mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
    uint16_t result;
    uint8 channel_map = 0, connect = 0;

    if (p_param->ch_mask.ch_37_off != 1) {
        channel_map |= 0x01;
    }
    if (p_param->ch_mask.ch_38_off != 1) {
        channel_map |= 0x02;
    }
    if (p_param->ch_mask.ch_39_off != 1) {
        channel_map |= 0x04;
    }

    if ((m_conn_ctx.conn_handle != DISCONNECTION)
            && (p_param->adv_type == MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED)) {
        return MI_ERR_INVALID_STATE;
    }

    result = gecko_cmd_le_gap_set_advertise_timing(ADV_HANDLE, p_param->adv_interval_min,
            p_param->adv_interval_max, 0, 0)->result;
    MI_ERR_CHECK(result);
    if (result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    }

    result = gecko_cmd_le_gap_set_advertise_channel_map(ADV_HANDLE, channel_map)->result;
    MI_ERR_CHECK(result);
    if (result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    }

    if (p_param->adv_type == MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_connectable_scannable;
    } else if (p_param->adv_type == MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED) {
        connect = le_gap_scannable_non_connectable;
    } else if (p_param->adv_type == MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED) {
        connect = le_gap_non_connectable;
    } else {
        return MI_ERR_INVALID_PARAM;
    }

    if (last_adv_data.len != 0) {
        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0,
                last_adv_data.len, last_adv_data.data)->result;
        if (result != bg_err_success)
            return MI_ERR_BUSY;
    }
    if (last_scan_rsp.len != 0) {
        result = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1,
                last_scan_rsp.len, last_scan_rsp.data)->result;
        if (result != bg_err_success)
            return MI_ERR_BUSY;
    }
    result = gecko_cmd_le_gap_start_advertising(ADV_HANDLE, le_gap_user_data, connect)->result;
    MI_ERR_CHECK(result);
    if (result == bg_err_success) {
        advertising = 1;
        return MI_SUCCESS;
    } else {
        connect_param_for_retry = connect;
        gecko_external_signal(START_ADV_RETRY_BIT_MASK);
        return MI_ERR_BUSY;
    }
}

/**
 * @brief   Config advertising data
 * @param   [in] p_data : Raw data to be placed in advertising packet. If NULL, no changes are made to the current advertising packet.
 * @param   [in] dlen   : Data length for p_data. Max size: 31 octets. Should be 0 if p_data is NULL, can be 0 if p_data is not NULL.
 * @param   [in] p_sr_data : Raw data to be placed in scan response packet. If NULL, no changes are made to the current scan response packet data.
 * @param   [in] srdlen : Data length for p_sr_data. Max size: BLE_GAP_ADV_MAX_SIZE octets. Should be 0 if p_sr_data is NULL, can be 0 if p_data is not NULL.
 * @return  MI_SUCCESS             Successfully set advertising data.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * */
mible_status_t mible_gap_adv_data_set(uint8_t const * p_data, uint8_t dlen,
        uint8_t const *p_sr_data, uint8_t srdlen)
{
    struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t *ret;

    if (p_data != NULL && dlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set advertisement data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 0, dlen, p_data);
        if (ret->result == bg_err_invalid_param) {
            return MI_ERR_INVALID_PARAM;
        }
        memcpy(last_adv_data.data, p_data, dlen);
        last_adv_data.len = dlen;
    }

    if (p_sr_data != NULL && srdlen <= 31) {
        /* 0 - advertisement, 1 - scan response, set scan response data here */
        ret = gecko_cmd_le_gap_bt5_set_adv_data(ADV_HANDLE, 1, srdlen, p_sr_data);
        if (ret->result == bg_err_invalid_param) {
            return MI_ERR_INVALID_PARAM;
        }
        memcpy(last_scan_rsp.data, p_sr_data, srdlen);
        last_scan_rsp.len = srdlen;
    }

    return MI_SUCCESS;
}

/*
 * @brief	Stop advertising
 * @param	void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void)
{
    struct gecko_msg_le_gap_stop_advertising_rsp_t *ret;
    if (!advertising) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_gap_stop_advertising(ADV_HANDLE);
    MI_ERR_CHECK(ret->result);
    if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else {
        return MIBLE_ERR_UNKNOWN;
    }
}

/*
 * @brief  	Create a Direct connection
 * @param   [in] scan_param : scanning parameters, see TYPE
 * mible_gap_scan_param_t for details.
 * 			[in] conn_param : connection parameters, see TYPE
 * mible_gap_connect_t for details.
 * @return  MI_SUCCESS             Successfully initiated connection procedure.
 *          MI_ERR_INVALID_STATE   Initiated connection procedure in connected state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again
 *          MIBLE_ERR_GAP_INVALID_BLE_ADDR    Invalid Bluetooth address
 * supplied.
 * @note 	Own and peer address are both public.
 * 			The connection result is given by MIBLE_GAP_EVT_CONNECTED
 * event
 * */
mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
        mible_gap_connect_t conn_param)
{
// TODO
    return MI_ERR_INVALID_STATE;
}

/*
 * @brief	Disconnect from peer
 * @param 	[in] conn_handle: the connection handle
 * @return  MI_SUCCESS             Successfully disconnected.
 *          MI_ERR_INVALID_STATE   Not in connnection.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note 	This function can be used by both central role and periphral
 * role.
 * */
mible_status_t mible_gap_disconnect(uint16_t conn_handle)
{
    struct gecko_msg_le_connection_close_rsp_t *ret;
    if (m_conn_ctx.conn_handle == DISCONNECTION) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_connection_close(conn_handle);
    if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else if (ret->result == bg_err_invalid_conn_handle) {
        return MIBLE_ERR_INVALID_CONN_HANDLE;
    } else if (ret->result == bg_err_not_connected) {
        return MI_ERR_INVALID_STATE;
    } else {
        return MIBLE_ERR_UNKNOWN;
    }
}

/*
 * @brief	Update the connection parameters.
 * @param  	[in] conn_handle: the connection handle.
 *			[in] conn_params: the connection parameters.
 * @return  MI_SUCCESS             The Connection Update procedure has been
 *started successfully.
 *          MI_ERR_INVALID_STATE   Initiated this procedure in disconnected
 *state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 *retry.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note  	This function can be used by both central role and peripheral
 *role.
 * */
mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
        mible_gap_conn_param_t conn_params)
{
    struct gecko_msg_le_connection_set_parameters_rsp_t *ret;
    if (m_conn_ctx.conn_handle == DISCONNECTION) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_le_connection_set_parameters(conn_handle,
            conn_params.min_conn_interval, conn_params.max_conn_interval,
            conn_params.slave_latency, conn_params.conn_sup_timeout);
    if (ret->result == bg_err_success) {
        return MI_SUCCESS;
    } else if (ret->result == bg_err_invalid_conn_handle) {
        return MIBLE_ERR_INVALID_CONN_HANDLE;
    } else if (ret->result == bg_err_invalid_param) {
        return MI_ERR_INVALID_PARAM;
    } else if (ret->result == bg_err_wrong_state) {
        return MI_ERR_INVALID_STATE;
    }
    return MI_SUCCESS;
}

/* GATTS related function  */
static uint16_t get_uuid_index(const struct bg_gattdb_def *bg_gattdb, mible_uuid_t *p_uuid)
{
    uint16_t index;
    if (p_uuid->type == 0) {
        // UUID16
        for (index = 0; index < bg_gattdb->uuidtable_16_size ; index++)
            if (bg_gattdb->uuidtable_16[index] == p_uuid->uuid16)
                return index;

    } else {
        // UUID128
        for (index = 0; index < bg_gattdb->uuidtable_128_size ; index++)
            if (memcmp(bg_gattdb->uuidtable_128 + index * 16, p_uuid->uuid128, 16) == 0)
                return index;
    }

    return -1;
}

static uint16_t iterate_get_att_handle(uint16_t uuid_idx, uint16_t begin, uint16_t end)
{
    uint16_t handle;
    for (handle = begin; handle <= end; handle++)
        if (bg_gattdb->attributes[handle].uuid == uuid_idx)
            return handle;

    return -1;
}

static int extract_service_region(mible_uuid_t *p_srv_uuid, uint16_t *begin_handle, uint16_t *end_handle)
{
    mible_uuid_t pri_srv_uuid = {
            .uuid16 = 0x2800,
    };
    uint16_t pri_srv_uuid_idx = get_uuid_index(bg_gattdb, &pri_srv_uuid);

    uint16_t handle = 0;
    do {
        handle = iterate_get_att_handle(pri_srv_uuid_idx, handle, bg_gattdb->attributes_max);
        if (memcmp(bg_gattdb->attributes[handle].constdata->data, p_srv_uuid->uuid128, p_srv_uuid->type ? 16 : 2) == 0) {
            *begin_handle = handle;
            *end_handle = iterate_get_att_handle(pri_srv_uuid_idx, handle+1, bg_gattdb->attributes_max);
            return 0;
        } else {
            handle++;
        }
    } while (handle != -1);

    return -1;
}

static uint16_t search_char_handle(mible_uuid_t *p_char_uuid, uint16_t begin_handle, uint16_t end_handle)
{
    uint16_t char_uuid_idx = get_uuid_index(bg_gattdb, p_char_uuid);
    if (p_char_uuid->type == 1)
        char_uuid_idx |= 0x8000;
    return iterate_get_att_handle(char_uuid_idx, begin_handle, end_handle);
}

/*
 * @brief	Add a Service to a GATT server
 * @param 	[in|out] p_server_db: pointer to mible service data type
 * of mible_gatts_db_t, see TYPE mible_gatts_db_t for details.
 * @return  MI_SUCCESS             Successfully added a service declaration.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_NO_MEM	       Not enough memory to complete operation.
 * @note    This function can be implemented asynchronous. When service inition complete, call mible_arch_event_callback function and pass in MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP event and result.
 * */
mible_status_t mible_gatts_service_init(mible_gatts_db_t *p_server_db)
{
    mible_status_t ret = MI_SUCCESS;
    mible_arch_evt_param_t param;
    uint16_t begin, end;

    extract_service_region(&p_server_db->p_srv_db->srv_uuid, &begin, &end);

    mible_gatts_char_db_t * p_char_db = p_server_db->p_srv_db->p_char_db;
    for(int n = 0; n < p_server_db->p_srv_db->char_num && m_char_table.num < CHAR_TABLE_NUM; n++, p_char_db++) {
        uint16_t handle = search_char_handle(&p_char_db->char_uuid, begin, end);
        if (handle != -1) {
            p_char_db->char_value_handle = handle + 1;
            uint8_t idx = m_char_table.num;
            m_char_table.item[idx].handle = p_char_db->char_value_handle;
            m_char_table.item[idx].rd_author = p_char_db->rd_author;
            m_char_table.item[idx].wr_author = p_char_db->wr_author;
            m_char_table.item[idx].char_property = p_char_db->char_property;
            m_char_table.item[idx].len = p_char_db->char_value_len;
            memcpy(m_char_table.item[idx].data, p_char_db->p_value, p_char_db->char_value_len);
            m_char_table.num++;
        } else {
            MI_LOG_ERROR("no char %d found.\n", p_char_db->char_uuid.uuid16);
            ret = MI_ERR_INTERNAL;
            goto exception;
        }
    }

    param.srv_init_cmp.p_gatts_db = p_server_db;
    param.srv_init_cmp.status = ret;
    mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);

exception:
    return ret;
}

static bool is_vaild_handle(uint16_t handle)
{
    bool handle_exist = false;
    for (uint8_t i = 0; i < CHAR_TABLE_NUM; i++) {
        if (m_char_table.item[i].handle == handle) {
            handle_exist = true;
        }
    }
    return handle_exist;
}

/*
 * @brief	Set characteristic value
 * @param	[in] srv_handle: service handle
 *			[in] value_handle: characteristic value handle
 *			[in] offset: the offset from which the attribute value has
 *to be updated
 *			[in] p_value: pointer to data
 *			[in] len: data length
 * @return  MI_SUCCESS             Successfully retrieved the value of the
 *attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE  Attributes are not modifiable by
 *the application.
 * */
mible_status_t mible_gatts_value_set(uint16_t srv_handle, uint16_t value_handle,
        uint8_t offset, uint8_t* p_value, uint8_t len)
{
    if (p_value == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    if(!is_vaild_handle(value_handle)){
    	return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
    }


    for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
    	if(m_char_table.item[i].handle == value_handle){
    		if(m_char_table.item[i].len >= offset + len){
    			memcpy(m_char_table.item[i].data + offset, p_value, len);
    			return MI_SUCCESS;
    		}else{
    			return MI_ERR_INVALID_LENGTH;
    		}
    	}
    }
    return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
}


/*
 * @brief	Get charicteristic value as a GATTS.
 * @param 	[in] srv_handle: service handle
 * 			[in] value_handle: characteristic value handle
 *			[out] p_value: pointer to data which stores characteristic value
 * 			[out] p_len: pointer to data length.
 * @return  MI_SUCCESS             Successfully get the value of the attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 **/

/*
 * Keivn: offset is not provided, MI_ERR_INVALID_PARAM is not able to check
 */
mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t value_handle,
        uint8_t* p_value, uint8_t *p_len)
{
    if (p_value == NULL || p_len == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    if(!is_vaild_handle(value_handle)){
    	return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
    }

    for(uint8_t i=0; i<CHAR_TABLE_NUM; i++){
    	if(m_char_table.item[i].handle == value_handle){
    		*p_len = m_char_table.item[i].len;
    		memcpy(p_value, m_char_table.item[i].data, *p_len);
    		return MI_SUCCESS;
    	}
    }
    return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
}

// this function set char value and notify/indicate it to client
/*
 * @brief 	Set characteristic value and notify it to client.
 * @param 	[in] conn_handle: conn handle
 *          [in] srv_handle: service handle
 * 			[in] char_value_handle: characteristic handle
 * 			[in] offset: the offset from which the attribute value has to
 * be updated
 * 			[in] p_value: pointer to data
 * 			[in] len: data length
 *          [in] type : notification = 1; indication = 2;
 *
 * @return  MI_SUCCESS             Successfully queued a notification or
 * indication for transmission,
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State or notifications
 * and/or indications not enabled in the CCCD.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE  Attributes are not modifiable by
 * the application.
 * @note    This function checks for the relevant Client Characteristic
 * Configuration descriptor
 *          value to verify that the relevant operation (notification or
 * indication) has been enabled by the client.
 * */
mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
        uint16_t char_value_handle, uint8_t offset, uint8_t* p_value, uint8_t len,
        uint8_t type)
{
    struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t *ret;

    if (p_value == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    if ((uint8_t) conn_handle == DISCONNECTION) {
        return MI_ERR_INVALID_STATE;
    }

    ret = gecko_cmd_gatt_server_send_characteristic_notification(conn_handle,
            char_value_handle, len, p_value);

    if (ret->result == bg_err_wrong_state) {
        // CCC not enabled
        return MI_ERR_INVALID_STATE;
    } else if (ret->result == bg_err_success) {
#if DEBUG_MIBLE
        MI_LOG_DEBUG("notify: ");
        MI_LOG_HEXDUMP(p_value, len);
#endif
        return MI_SUCCESS;
    } else {
#if DEBUG_MIBLE
        MI_LOG_ERROR("notify error 0x%X\n", ret->result);
#endif
        return MIBLE_ERR_UNKNOWN;
    }

}

/**
 * @brief   Respond to a Read/Write user authorization request.
 * @param   [in] conn_handle: conn handle
 *          [in] status:  1: permit to change value ; 0: reject to change value
 *          [in] char_value_handle: characteristic handle
 *          [in] offset: the offset from which the attribute value has to
 * be updated
 *          [in] p_value: Pointer to new value used to update the attribute value.
 *          [in] len: data length
 *          [in] type : read response = 1; write response = 2;
 *
 * @return  MI_SUCCESS             Successfully queued a response to the peer, and in the case of a write operation, GATT updated.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State or no authorization request pending.
 *          MI_ERR_INVALID_LENGTH  Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 * @note    This call should only be used as a response to a MIBLE_GATTS_EVT_READ/WRITE_PERMIT_REQ
 * event issued to the application.
 * */
mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle,
        uint8_t permit, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type)
{
    uint16_t result;
    if (permit == 1 && p_value == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    switch(type) {
    case 1:
    result =
    gecko_cmd_gatt_server_send_user_read_response(conn_handle,
                                                  char_value_handle,
                                                  permit ? 0 : bg_err_att_read_not_permitted,
                                                  len,
                                                  p_value)->result;
        break;

    case 2:
    result =
    gecko_cmd_gatt_server_send_user_write_response(conn_handle,
                                                   char_value_handle,
                                                   permit ? 0 : bg_err_att_write_not_permitted)->result;
        break;
    }

    return result == bg_err_success ? MI_SUCCESS : result;
}

/*TIMER related function*/

/*
 * @brief 	Create a timer.
 * @param 	[out] pp_timer: a pointer to timer handle address which can uniquely
 *  identify the timer.
 * 			[in] timeout_handler: a function will be called when the timer expires.
 * 			[in] mode: repeated or single shot.
 * @return  MI_SUCCESS             If the timer was successfully created.
 *          MI_ERR_INVALID_PARAM   Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   timer module has not been initialized or the
 * timer is running.
 *          MI_ERR_NO_MEM          timer pool is full.
 *
 * */
mible_status_t mible_timer_create(void** pp_timer, mible_timer_handler timeout_handler,
        mible_timer_mode mode)
{
    if (pp_timer == NULL || timeout_handler == NULL)
        return MI_ERR_INVALID_PARAM;

    *pp_timer = acquire_timer();
    if (*pp_timer == NULL)
        return MI_ERR_NO_MEM;

    timer_item_t * p_timer = *pp_timer;
    p_timer->handler = timeout_handler;
    p_timer->is_single_shot = mode == MIBLE_TIMER_SINGLE_SHOT;
    return MI_SUCCESS;
}

/*
 * @brief 	Delete a timer.
 * @param 	[in] timer_handle: unique index of the timer.
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void* timer_handle)
{
    if (release_timer(timer_handle) == -1)
        return MI_ERR_INVALID_PARAM;
    else
        return MI_SUCCESS;
}

/*
 * @brief 	Start a timer.
 * @param 	[in] timer_handle: unique index of the timer.
 *          [in] time_ms: number of milliseconds to time-out event
 * (minimum 10 ms).
 * 			[in] p_context: parameters that can be passed to
 * timeout_handler
 *
 * @return  MI_SUCCESS             If the timer was successfully started.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   If the application timer module has not been
 * initialized or the timer has not been created.
 *         	MI_ERR_NO_MEM          If the timer operations queue was full.
 * @note 	If the timer has already started, it will start counting again.
 * */
mible_status_t mible_timer_start(void* timer_handle, uint32_t time_ms, void* p_context)
{
    if (assert_timer(timer_handle) == -1)
        return MI_ERR_INVALID_PARAM;

    timer_item_t * p_timer = timer_handle;
    int ret = gecko_cmd_hardware_set_soft_timer(
                    TIMER_MS_2_TIMERTICK(time_ms),
                    p_timer->id,
                    p_timer->is_single_shot)->result;

    return ret == 0 ? MI_SUCCESS : MI_ERR_INVALID_STATE;
}

/*
 * @brief 	Stop a timer.
 * @param 	[in] timer_handle: unique index of the timer.
 * @return  MI_SUCCESS             If the timer was successfully stopped.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *
 * */
mible_status_t mible_timer_stop(void* timer_handle)
{
    if (assert_timer(timer_handle) == -1)
        return MI_ERR_INVALID_PARAM;

    timer_item_t * p_timer = timer_handle;
    int ret = gecko_cmd_hardware_set_soft_timer(
                    0,
                    p_timer->id,
                    p_timer->is_single_shot)->result;

    return ret == 0 ? MI_SUCCESS : MI_ERR_INVALID_PARAM;
}

/* FLASH related function*/

/*
 * @brief 	Create a record in flash
 * @param 	[in] record_id: identify a record in flash
 * 			[in] len: record length
 * @return 	MI_SUCCESS 				Create successfully.
 * 			MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *   		MI_ERR_NO_MEM,			Not enough flash memory to be assigned
 *
 * */
mible_status_t mible_record_create(uint16_t record_id, uint8_t len)
{
    return MI_SUCCESS;
}

/*
 * @brief  	Delete a record in flash
 * @param 	[in] record_id: identify a record in flash
 * @return 	MI_SUCCESS 				Delete successfully.
 * 			MI_ERR_INVALID_PARAM   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
    if (record_id > 127) {
        return MI_ERR_INVALID_PARAM;
    }

    struct gecko_msg_flash_ps_erase_rsp_t *p_rsp;
    p_rsp = gecko_cmd_flash_ps_erase(record_id + 0x4000);

    mible_arch_evt_param_t arch_evt_param;
    arch_evt_param.record.id = record_id;
    arch_evt_param.record.status = p_rsp->result == bg_err_success ? MI_SUCCESS : MI_ERR_RESOURCES;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE, &arch_evt_param);
    return MI_SUCCESS;
}

/*
 * @brief 	Restore data to flash
 * @param 	[in] record_id: identify an area in flash
 * 			[out] p_data: pointer to data
 *			[in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAM   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data, uint8_t len)
{
    // TODO: support length longer than 56.
    if (len > MAX_SINGLE_PS_LENGTH){
        return MI_ERR_INVALID_LENGTH;
    } else if (record_id > 127) {
        return MI_ERR_INVALID_PARAM;
    } else if (p_data == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    struct gecko_msg_flash_ps_load_rsp_t *p_rsp;
    p_rsp = gecko_cmd_flash_ps_load(record_id + 0x4000);

    if (p_rsp->result == bg_err_success) {
        memcpy(p_data, p_rsp->value.data, len);
        return MI_SUCCESS;
    } else {
        return MI_ERR_INVALID_PARAM;
    }
}

/*
 * @brief 	Store data to flash
 * @param 	[in] record_id: identify an area in flash
 * 			[in] p_data: pointer to data
 * 			[in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 * allowed size.
 *          MI_ERR_INVALID_PARAM   p_data is not aligned to a 4 byte boundary.
 * @note  	Should use asynchronous mode to implement this function.
 *          The data to be written to flash has to be kept in memory until the
 * operation has terminated, i.e., an event is received.
 * 			When record writing complete , call mible_arch_event_callback function and pass MIBLE_ARCH_EVT_RECORD_WRITE_CMP event and result.
 * */
mible_status_t mible_record_write(uint16_t record_id, const uint8_t* p_data, uint8_t len)
{
    // TODO: support length longer than 56.
    if (len > MAX_SINGLE_PS_LENGTH){
        return MI_ERR_INVALID_LENGTH;
    } else if (p_data == NULL || record_id > 127) {
        return MI_ERR_INVALID_PARAM;
    }

    struct gecko_msg_flash_ps_save_rsp_t *p_rsp;
    p_rsp = gecko_cmd_flash_ps_save(record_id + 0x4000, len, p_data);

    mible_arch_evt_param_t arch_evt_param;
    arch_evt_param.record.id = record_id;
    arch_evt_param.record.status = p_rsp->result == bg_err_success ? MI_SUCCESS : MI_ERR_RESOURCES;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &arch_evt_param);
    return MI_SUCCESS;
}

/*
 * @brief 	Get ture random bytes .
 * @param 	[out] p_buf: pointer to data
 * 			[in] len: Number of bytes to take from pool and place in
 * p_buff
 * @return  MI_SUCCESS			The requested bytes were written to
 * p_buff
 *          MI_ERR_NO_MEM       No bytes were written to the buffer, because
 * there were not enough random bytes available.
 * @note  	SHOULD use TRUE random num generator
 * */
mible_status_t mible_rand_num_generator(uint8_t* p_buf, uint8_t len)
{
    struct gecko_msg_system_get_random_data_rsp_t *ret;
    uint8_t times;
    uint8_t rest;

    times = len / MAX_SINGLE_RANDOM_DATA_LENGTH;
    rest = len % MAX_SINGLE_RANDOM_DATA_LENGTH;

    for (uint8_t i = 0; i < times; i++) {
        ret = gecko_cmd_system_get_random_data(MAX_SINGLE_RANDOM_DATA_LENGTH);
        if (ret->result == bg_err_success) {
            memcpy(p_buf + MAX_SINGLE_RANDOM_DATA_LENGTH * i, ret->data.data,
                    ret->data.len);
        } else {
            return MI_ERR_NO_MEM;
        }
    }

    ret = gecko_cmd_system_get_random_data(rest);
    if (ret->result == bg_err_success) {
        memcpy(p_buf + MAX_SINGLE_RANDOM_DATA_LENGTH * times, ret->data.data,
                ret->data.len);
    } else {
        return MI_ERR_NO_MEM;
    }
    return MI_SUCCESS;
}

/*
 * @brief 	Encrypts a block according to the specified parameters. 128-bit
 * AES encryption.
 * @param 	[in] key: encryption key
 * 			[in] plaintext: pointer to plain text
 * 			[in] plen: plain text length
 *          [out] ciphertext: pointer to cipher text
 * @return  MI_SUCCESS              The encryption operation completed.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE    Encryption module is not initialized.
 *          MI_ERR_INVALID_LENGTH   Length bigger than 16.
 *          MI_ERR_BUSY             Encryption module already in progress.
 * @note  	SHOULD use synchronous mode to implement this function
 * */
mible_status_t mible_aes128_encrypt(const uint8_t* key, const uint8_t* plaintext,
        uint8_t plen, uint8_t* ciphertext)
{
    uint8_t tmpPlain[16];
    uint8_t tmpCipher[16];
    mbedtls_aes_context aes_ctx;

    if (key == NULL || plaintext == NULL || ciphertext == NULL) {
        return MI_ERR_INVALID_ADDR;
    }

    if (plen > 16) {
        return MI_ERR_INVALID_LENGTH;
    }

    mbedtls_aes_init(&aes_ctx);
    memset(tmpPlain, 0, 16);
    memset(tmpCipher, 0, 16);

    memcpy(tmpPlain, plaintext, plen);
    mbedtls_aes_setkey_enc(&aes_ctx, key, 128);
    mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, tmpPlain, tmpCipher);

    memcpy(ciphertext, tmpCipher, 16);

    return MI_SUCCESS;
}

/* TASK schedulor related function  */
#include "common/queue.h"
typedef struct {
    mible_handler_t handler;
    void *arg;
} mible_task_t;

static mible_task_t task_buf[MAX_TASK_NUM];
static queue_t task_queue;
/*
 * @brief 	Post a task to a task quene, which can be executed in a right place(maybe a task in RTOS or while(1) in the main function).
 * @param 	[in] handler: a pointer to function
 * 			[in] param: function parameters
 * @return 	MI_SUCCESS 				Successfully put the handler to quene.
 * 			MI_ERR_NO_MEM			The task quene is full.
 * 			MI_ERR_INVALID_PARAM    Handler is NULL
 * */
mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
    static uint8_t task_queue_is_init = 0;
    uint32_t errno;
    if (!task_queue_is_init){
        task_queue_is_init = 1;
        errno = queue_init(&task_queue, task_buf,
                            sizeof(task_buf) / sizeof(task_buf[0]), sizeof(task_buf[0]));
        MI_ERR_CHECK(errno);
    }

    mible_task_t task = {
        .handler = handler,
        .arg     = arg,
    };

    errno = enqueue(&task_queue, &task);
    return errno;
}

void mible_tasks_exec(void)
{
    uint32_t errno = 0;
    mible_task_t task;
    while(!errno) {
        errno = dequeue(&task_queue, &task);
        if (errno == MI_SUCCESS)
            task.handler(task.arg);
    }
}

/**
 *        IIC APIs
 */
#include "em_i2c.h"
static bool iic_is_busy;
static const iic_config_t * m_p_iic_config;
static mible_handler_t m_iic_handler;
static I2C_TransferSeq_TypeDef seq;
/*****************************************************************************
 * @brief  Handles various I2C events and errors
 *
 * When a STOP condition has been successfully sent, the MSTOP
 * interrupt is triggered and the transfer is marked as complete.
 *
 * The three errors: ARBLOST, BUSERR, and NACK are handled here.
 * In all cases, the current transfer is aborted, and the error
 * flag is set to inform the main loop that an error occured
 * during the transfer.
 *****************************************************************************/
void I2C0_IRQHandler(void)
{
    iic_event_t event;
    I2C_TransferReturn_TypeDef xfer_stat;
    xfer_stat = I2C_Transfer(I2C0);
    switch(xfer_stat){
    case i2cTransferInProgress:
        break;
    case i2cTransferDone:
        event = IIC_EVT_XFER_DONE;
        m_iic_handler(&event);
        break;
    case i2cTransferNack:
        event = IIC_EVT_DATA_NACK;
        m_iic_handler(&event);
        break;
    default:
        MI_LOG_ERROR("iic bus errno %d.\n", xfer_stat);
        event = IIC_EVT_ADDRESS_NACK;
        m_iic_handler(&event);
        break;
    }
}
/**
 * @brief   Function for initializing the IIC driver instance.
 * @param   [in] p_config: Pointer to the initial configuration.
 *          [in] handler: Event handler provided by the user.
 * @return  MI_SUCCESS              Initialized successfully.
 *          MI_ERR_INVALID_PARAM    p_config or handler is a NULL pointer.
 *
 * */
mible_status_t mible_iic_init(const iic_config_t * p_config,
        mible_handler_t handler)
{
    if (p_config != NULL && handler != NULL) {
        m_p_iic_config = p_config;
        m_iic_handler = handler;
    } else {
        return MI_ERR_INVALID_PARAM;
    }

    CMU_ClockEnable(cmuClock_HFPER, true);
    /* Select I2C peripheral clock */
    CMU_ClockEnable(cmuClock_I2C0, true);

    /* Output value must be set to 1 to not drive lines low. Set
       SCL first, to ensure it is high before changing SDA. */
    GPIO_PinModeSet(p_config->scl_port, p_config->scl_pin, gpioModeWiredAndPullUp, 1);
    GPIO_PinModeSet(p_config->sda_port, p_config->sda_pin, gpioModeWiredAndPullUp, 1);

//    /* In some situations, after a reset during an I2C transfer, the slave
//       device may be left in an unknown state. Send 9 clock pulses to
//       set slave in a defined state. */
//    for (i = 0; i < 9; i++) {
//      GPIO_PinOutSet(p_config->scl_port, p_config->scl_pin);
//      GPIO_PinOutClear(p_config->scl_port, p_config->scl_pin);
//    }

    /* Enable pins and set location */
  #if defined (_I2C_ROUTEPEN_MASK)
    I2C0->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
    I2C0->ROUTELOC0 = (p_config->sda_extra_conf << _I2C_ROUTELOC0_SDALOC_SHIFT)
                    | (p_config->scl_extra_conf << _I2C_ROUTELOC0_SCLLOC_SHIFT);
  #else
    I2C0->ROUTE = I2C_ROUTE_SDAPEN
                | I2C_ROUTE_SCLPEN
                | (p_config->scl_extra_conf << _I2C_ROUTE_LOCATION_SHIFT);
  #endif

    /* Set emlib i2c0 init parameters */
    I2C_Init_TypeDef i2cInit = {0};
    i2cInit.enable = true;
    i2cInit.master = true; /* master mode only */
    i2cInit.refFreq = 0;
    i2cInit.freq = p_config->freq == IIC_100K ? I2C_FREQ_STANDARD_MAX : I2C_FREQ_FAST_MAX;
    i2cInit.clhr = i2cClockHLRStandard;
    I2C_Init(I2C0, &i2cInit);
    iic_is_busy = true;

    return MI_SUCCESS;
}

/**
 * @brief   Function for uninitializing the IIC driver instance.
 *
 *
 * */
void mible_iic_uninit(void)
{
    NVIC_DisableIRQ(I2C0_IRQn);
    I2C_Reset(I2C0);
    GPIO_PinModeSet(m_p_iic_config->scl_port, m_p_iic_config->scl_pin, gpioModeDisabled, 0);
    GPIO_PinModeSet(m_p_iic_config->sda_port, m_p_iic_config->sda_pin, gpioModeDisabled, 0);
    CMU_ClockEnable(cmuClock_I2C0, false);
    iic_is_busy = false;
}

/**
 * @brief   Function for sending data to a IIC slave.
 * @param   [in] addr:   Address of a specific slave device (only 7 LSB).
 *          [in] p_out:  Pointer to tx data
 *          [in] len:    Data length
 *          [in] no_stop: If set, the stop condition is not generated on the bus
 *          after the transfer has completed successfully (allowing for a repeated start in the next transfer).
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_out is not vaild address.
 * @note    This function should be implemented in non-blocking mode.
 *          When tx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument.
 * */
mible_status_t mible_iic_tx(uint8_t addr, uint8_t * p_out, uint16_t len,
bool no_stop)
{
    uint32_t errno;
    if (p_out == NULL && len > 512) {
        return MI_ERR_INVALID_PARAM;
    }

    // Enable interrupts in NVIC
    NVIC_ClearPendingIRQ(I2C0_IRQn);
    NVIC_EnableIRQ(I2C0_IRQn);

    // Enable i2c interrupts
    I2C_IntClear(I2C0, I2C_IEN_ARBLOST|I2C_IEN_BUSERR|I2C_IEN_NACK|I2C_IEN_ACK|
            I2C_IEN_RXDATAV|I2C_IEN_MSTOP);
    I2C_IntEnable(I2C0, I2C_IEN_ARBLOST|I2C_IEN_BUSERR|I2C_IEN_NACK|I2C_IEN_ACK|
            I2C_IEN_RXDATAV|I2C_IEN_MSTOP);

    seq = (I2C_TransferSeq_TypeDef){
            .addr = addr << 1,
            .flags = I2C_FLAG_WRITE,
            .buf[0].data = p_out,
            .buf[0].len = len
    };

    errno = I2C_TransferInit(I2C0, &seq);

    if (errno != i2cTransferInProgress) {
        MI_LOG_INFO("cant start iic tx. errno: %d\n", errno);
        return errno;
    } else {
        return MI_SUCCESS;
    }
}

/**
 * @brief   Function for receiving data from a IIC slave.
 * @param   [in] addr:   Address of a specific slave device (only 7 LSB).
 *          [out] p_in:  Pointer to rx data
 *          [in] len:    Data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_BUSY             If a transfer is ongoing.
 *          MI_ERR_INVALID_PARAM    p_in is not vaild address.
 * @note    This function should be implemented in non-blocking mode.
 *          When rx procedure complete, the handler provided by mible_iic_init() should be called,
 * and the iic event should be passed as a argument.
 * */
mible_status_t mible_iic_rx(uint8_t addr, uint8_t * p_in, uint16_t len)
{
    uint32_t errno;
    if (p_in == NULL && len > 512) {
        return MI_ERR_INVALID_PARAM;
    }

    // Enable interrupts in NVIC
    NVIC_ClearPendingIRQ(I2C0_IRQn);
    NVIC_EnableIRQ(I2C0_IRQn);

    // Enable i2c interrupts
    I2C_IntClear(I2C0, I2C_IEN_ARBLOST|I2C_IEN_BUSERR|I2C_IEN_NACK|I2C_IEN_ACK|
            I2C_IEN_RXDATAV|I2C_IEN_MSTOP);
    I2C_IntEnable(I2C0, I2C_IEN_ARBLOST|I2C_IEN_BUSERR|I2C_IEN_NACK|I2C_IEN_ACK|
            I2C_IEN_RXDATAV|I2C_IEN_MSTOP);

    seq = (I2C_TransferSeq_TypeDef){
            .addr = addr << 1,
            .flags = I2C_FLAG_READ,
            .buf[0].data = p_in,
            .buf[0].len = len
    };

    errno = I2C_TransferInit(I2C0, &seq);

    if (errno != i2cTransferInProgress) {
        MI_LOG_INFO("cant start iic rx. errno: %d\n", errno);
        return errno;
    } else {
        return MI_SUCCESS;
    }
}

/**
 * @brief   Function for checking IIC SCL pin.
 * @param   [in] port:   SCL port
 *          [in] pin :   SCL pin
 * @return  1: High (Idle)
 *          0: Low (Busy)
 * */
int mible_iic_scl_pin_read(uint8_t port, uint8_t pin)
{
    return GPIO_PinInGet(port, pin);
}

mible_status_t mible_nvm_init(void)
{
    static bool enable = false;

    if (!enable) {
        MSC_Init();
        enable = true;
    }

    return MI_SUCCESS;
}

/**
 * @brief   Function for loadind data from Non-Volatile Memory.
 * @param   [out] p_data:  Pointer to data to be restored.
 *          [in] length:   Data size in bytes.
 *          [in] address:  Address in Non-Volatile Memory to read.
 * @return  MI_ERR_INTERNAL:  invalid NVM address.
 *          MI_SUCCESS
 * */
mible_status_t mible_nvm_read(void * p_data, uint32_t length, uint32_t address)
{
    if (MSC->STATUS & MSC_STATUS_BUSY) {
        MI_LOG_WARNING("%s -- flash is busy\n", __func__);
        return MI_ERR_BUSY;
    }

    if (NULL == p_data || 0 == length) {
        MI_LOG_WARNING("%s -- invalid parameter\n", __func__);
        return MI_ERR_INVALID_PARAM;
    }

    memcpy(p_data, (void *)(address), length);
    return MI_SUCCESS;
}

static bool nvm_is_erased(uint32_t address, uint32_t length)
{
    for (uint32_t i = 0; i < length; i += 4) {
        if (*(uint32_t *)(address + i) != 0xFFFFFFFF) {
            return false;
        }
    }

    return true;
}

static bool nvm_part_erase(uint32_t address)
{
    MSC_Status_TypeDef ret;

    if (0 == address % FLASH_PAGE_SIZE) {
        return true;
    }

    if (MSC->STATUS & MSC_STATUS_BUSY) {
        MI_LOG_WARNING("%s -- flash is busy\n", __func__);
        return false;
    }

    uint32_t buffer[(address % FLASH_PAGE_SIZE) / 4 + 1];

    memcpy(buffer, (const void *)(address - address % FLASH_PAGE_SIZE),
                address % FLASH_PAGE_SIZE);
    ret = MSC_ErasePage((uint32_t *)(address - address % FLASH_PAGE_SIZE));
    if (mscReturnOk != ret) {
        MI_LOG_WARNING("%s -- erase error code %d\n", __func__, ret);
        return false;
    }

    ret = MSC_WriteWord((uint32_t *)(address - address % FLASH_PAGE_SIZE),
                                    buffer, address % FLASH_PAGE_SIZE);
    if (mscReturnOk != ret) {
        MI_LOG_WARNING("%s -- write error code %d\n", __func__, ret);
        return false;
    }

    return true;
}

/**
 * @brief   Function for storing data into Non-Volatile Memory.
 * @param   [in] p_data:   Pointer to data to be stored.
 *          [in] length:   Data size in bytes.
 *          [in] address:  Start address used to store data.
 * @return  MI_ERR_INTERNAL:  invalid NVM address.
 *          MI_SUCCESS
 * */
mible_status_t mible_nvm_write(void * p_data, uint32_t length, uint32_t address)
{
    MSC_Status_TypeDef ret;
    MSC_Init();
    if (MSC->STATUS & MSC_STATUS_BUSY) {
        return MI_ERR_BUSY;
    }

    if (NULL == p_data || 0 == length) {
        return MI_ERR_INVALID_PARAM;
    }

    if (0 == address % FLASH_PAGE_SIZE) {
        ret = MSC_ErasePage((uint32_t *)address);
        if (mscReturnOk != ret) {
            MI_LOG_WARNING("%s -- erase error code %d\n", __func__, ret);
            return MI_ERR_INTERNAL;
        }
    } else if (!nvm_is_erased(address, length)) {
        if (!nvm_part_erase(address)) {
            return MI_ERR_INTERNAL;
        }
    }

    ret = MSC_WriteWord((uint32_t *)address, p_data, length);
    if (ret == mscReturnOk) {
        return MI_SUCCESS;
    } else {
        MI_LOG_WARNING("%s -- write error code %d\n", __func__, ret);
        return MI_ERR_INTERNAL;
    }
}


#include "btl_interface.h"
mible_status_t mible_upgrade_firmware(void)
{
    uint32 errno = bootloader_init();
    MI_ERR_CHECK(errno);
    bootloader_rebootAndInstall();

    return MI_SUCCESS;
}

#ifndef MIBLE_TYPE_H__
#define MIBLE_TYPE_H__

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

#include "mible_port.h"

#define MIBLE_GAP_EVT_BASE   0x00
#define MIBLE_GATTS_EVT_BASE 0x40
#define MIBLE_GATTC_EVT_BASE 0x80

#define MIBLE_ADDR_LEN                          6
#define MIBLE_HANDLE_INVALID                    0xFFFF

typedef uint8_t mible_addr_t[6];

typedef uint32_t mible_cfm_t;

typedef struct {
    uint16_t begin_handle;
    uint16_t end_handle;
} mible_handle_range_t;

typedef enum {
    MIBLE_ADDRESS_TYPE_PUBLIC, // public address
    MIBLE_ADDRESS_TYPE_RANDOM, // random address
} mible_addr_type_t;

/* GAP related */
typedef enum {
    MIBLE_SCAN_TYPE_PASSIVE,  // passive scanning
    MIBLE_SCAN_TYPE_ACTIVE,   // active scanning
} mible_gap_scan_type_t;

typedef struct {
    uint16_t scan_interval;                   // Range: 0x0004 to 0x4000 Time = N * 0.625 msec Time Range: 2.5 msec to 10.24 sec
    uint16_t scan_window;                     // Range: 0x0004 to 0x4000 Time = N * 0.625 msec Time Range: 2.5 msec to 10.24 seconds
    uint16_t timeout;                         // Scan timeout between 0x0001 and 0xFFFF in seconds, 0x0000 disables timeout.
} mible_gap_scan_param_t;

typedef enum {
    MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED=0,    // ADV_IND
    MIBLE_ADV_TYPE_CONNECTABLE_DIRECTED,        // ADV_DIRECT_IND
    MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED,        // ADV_SCAN_IND
    MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED,  // ADV_NONCONN_IND
} mible_gap_adv_type_t;

typedef struct {
    uint16_t adv_interval_min;               // Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec
    uint16_t adv_interval_max;               // Range: 0x0020 to 0x4000  Time = N * 0.625 msec Time Range: 20 ms to 10.24 sec
    mible_gap_adv_type_t adv_type;

    struct {
        uint8_t ch_37_off : 1;  /**< Setting this bit to 1 will turn off advertising on channel 37 */
        uint8_t ch_38_off : 1;  /**< Setting this bit to 1 will turn off advertising on channel 38 */
        uint8_t ch_39_off : 1;  /**< Setting this bit to 1 will turn off advertising on channel 39 */
    } ch_mask;
} mible_gap_adv_param_t;

typedef enum {
    ADV_DATA,           // advertising data
    SCAN_RSP_DATA,      // response data from active scanning
} mible_gap_adv_data_type_t;

typedef struct {
    mible_addr_t peer_addr;
    mible_addr_type_t addr_type;
    mible_gap_adv_data_type_t adv_type;
    int8_t rssi;
    uint8_t data_len;
    uint8_t data[31];
} mible_gap_adv_report_t;

typedef enum {
    UNKNOWN_OTHER_ERROR=0,
    CONNECTION_TIMEOUT = 1,
    REMOTE_USER_TERMINATED,
    LOCAL_HOST_TERMINATED,
} mible_gap_disconnect_reason_t;

typedef struct {
    uint16_t min_conn_interval;    // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4 seconds.
    uint16_t max_conn_interval;    // Range: 0x0006 to 0x0C80, Time = N * 1.25 msec, Time Range: 7.5 msec to 4 seconds.
    uint16_t slave_latency;        // Range: 0x0000 to 0x01F3
    uint16_t conn_sup_timeout;     // Range: 0x000A to 0x0C80, Time = N * 10 msec, Time Range: 100 msec to 32 seconds
} mible_gap_conn_param_t;

typedef enum {
    MIBLE_GAP_CENTRAL = 0x00,
    MIBLE_GAP_PERIPHERAL = 0x01,
} mible_gap_role_t;

typedef struct {
    mible_addr_t peer_addr;
    mible_addr_type_t type;
    mible_gap_role_t role;
    mible_gap_conn_param_t conn_param;
} mible_gap_connect_t;

typedef struct {
    mible_gap_disconnect_reason_t reason;
} mible_gap_disconnect_t;

typedef struct {
    mible_gap_conn_param_t conn_param;
} mible_gap_connect_update_t;

typedef struct {
    uint16_t conn_handle;
    uint16_t reserved;
    union {
        mible_gap_connect_t connect;
        mible_gap_disconnect_t disconnect;
        mible_gap_adv_report_t report;
        mible_gap_connect_update_t update_conn;
    };
} mible_gap_evt_param_t;

typedef enum {
    MIBLE_GAP_EVT_CONNECTED = MIBLE_GAP_EVT_BASE, /**< Generated when a connection is established.*/
    MIBLE_GAP_EVT_DISCONNECTED, /**< Generated when a connection is terminated.*/
    MIBLE_GAP_EVT_CONN_PARAM_UPDATED,
    MIBLE_GAP_EVT_ADV_REPORT,
} mible_gap_evt_t;

/*GATTS related*/

// GATTS database
typedef enum {
    MIBLE_UUID_16 = 0,
    MIBLE_UUID_128 = 1,
} mible_uuid_type_t;

typedef struct {
    uint32_t type;                                     // MIBLE_UUID_16 = 0 MIBLE_UUID_128 = 1
    union {
        uint16_t uuid16;
        uint8_t uuid128[16];
    };
} mible_uuid_t;

typedef enum {
    MIBLE_PRIMARY_SERVICE = 1,
    MIBLE_SECONDARY_SERVICE,
} mible_gatts_service_t;

typedef struct{
    uint16_t reliable_write     :1;
    uint16_t writeable          :1;
} mible_gatts_char_desc_ext_prop_t;

typedef struct{
    char *string;
    uint8_t len;
} mible_gatts_char_desc_user_desc_t;

typedef struct{
    uint8_t  format;
    uint8_t  exponent;
    uint16_t unit;
    uint8_t  name_space;
    uint16_t desc;
} mible_gatts_char_desc_cpf_t;

/*
 * NOTE: if char property contains notify , then SHOULD include cccd(client characteristic configuration descriptor automatically). The same to sccd when BROADCAST enabled
 * */
typedef struct{
    mible_gatts_char_desc_ext_prop_t  *extend_prop;
    mible_gatts_char_desc_cpf_t       *char_format;     // See more details at Bluetooth SPEC 4.2 [Vol 3, Part G] Page 539
    mible_gatts_char_desc_user_desc_t *user_desc;       // read only
} mible_gatts_char_desc_db_t;

// gatts characteristic
// default:  no authentication ; no encrption; configurable authorization

typedef struct{
    mible_uuid_t char_uuid;
    uint8_t char_property;                             // See TYPE mible_gatts_char_property for details
    uint8_t *p_value;                                  // initial characteristic value
    uint8_t char_value_len;
    uint16_t char_value_handle;                        // [out] where the assigned handle be stored.
    bool is_variable_len;
    bool rd_author;                                    // read authorization. Enabel or Disable MIBLE_GATTS_READ_PERMIT_REQ event
    bool wr_author;                                    // write authorization. Enabel or Disable MIBLE_GATTS_WRITE_PERMIT_REQ event
    mible_gatts_char_desc_db_t char_desc_db;
} mible_gatts_char_db_t;

typedef struct{
    mible_gatts_service_t srv_type;                    // primary service or secondary service
    uint16_t srv_handle;                               // [out] dynamically allocated
    mible_uuid_t srv_uuid;                             // 16-bit or 128-bit uuid
    uint8_t char_num;
    mible_gatts_char_db_t *p_char_db;                  // p_char_db[charnum-1]
} mible_gatts_srv_db_t;                                // Regardless of service inclusion service

typedef struct{
    mible_gatts_srv_db_t *p_srv_db;                    // p_srv_db[srv_num]
    uint8_t srv_num;
} mible_gatts_db_t;


typedef enum {
    MIBLE_BROADCAST           = 0x01,
    MIBLE_READ                = 0x02,
    MIBLE_WRITE_WITHOUT_RESP  = 0x04,
    MIBLE_WRITE               = 0x08,
    MIBLE_NOTIFY              = 0x10,
    MIBLE_INDICATE            = 0x20,
    MIBLE_AUTH_SIGNED_WRITE   = 0x40,
} mible_gatts_char_property;

typedef enum {
    MIBLE_GATTS_EVT_WRITE = MIBLE_GATTS_EVT_BASE,      // When this event is called, the characteristic has been modified.
    MIBLE_GATTS_EVT_READ_PERMIT_REQ,                   // If charicteristic's rd_auth = TRUE, this event will be generated.
    MIBLE_GATTS_EVT_WRITE_PERMIT_REQ,                  // If charicteristic's wr_auth = TRUE, this event will be generated, meanwhile the char value hasn't been modified. mible_gatts_rw_auth_reply().
    MIBLE_GATTS_EVT_IND_CONFIRM
} mible_gatts_evt_t;

/*
 * MIBLE_GATTS_EVT_WRITE and MIBLE_GATTS_EVT_WRITE_PERMIT_REQ events callback
 * parameters
 * NOTE: Stack SHOULD decide whether to response to gatt client. And if need to reply, just reply success or failure according to [permit]
 * */
typedef struct {
    uint16_t value_handle; // char value_handle
    uint8_t offset;
    uint8_t* data;
    uint8_t len;
} mible_gatts_write_t;

/*
 * MIBLE_GATTS_EVT_READ_PERMIT_REQ event callback parameters
 * NOTE: Stack SHOULD decide to reply the char value or refuse according to [permit]
 * */
typedef struct {
    uint16_t value_handle;  // char value handle
} mible_gatts_read_t;

/*
 * GATTS event callback parameters union
 * */
typedef struct {
    uint16_t conn_handle;
    union {
        mible_gatts_write_t write;
        mible_gatts_read_t read;
    };
} mible_gatts_evt_param_t;

/*GATTC related*/

/*
 * GATTC event
 * */
typedef enum {
    // this event generated in responses to a discovery service
    MIBLE_GATTC_EVT_DISCOVERY_RSP,
    // this event generated in responses to a read char
    MIBLE_GATTC_EVT_READ_RSP,
    // this event generated in responses to a write_charicteristic_value_with_response procedure.
    MIBLE_GATTC_EVT_WRITE_RSP,
    // this event is generated when peer gatts device send a notification.
    MIBLE_GATTC_EVT_NOTIFICATION,
    // this event is generated when peer gatts device send a indication.
    MIBLE_GATTC_EVT_INDICATION,
} mible_gattc_evt_t;

typedef struct{
    uint16_t handle;
    uint16_t rsvd;
    mible_uuid_t uuid;
} mible_handle_uuid_t;

typedef struct{
    mible_uuid_t char_uuid;
    uint16_t char_handle;
    uint8_t char_property;
    uint8_t desc_num;
    mible_handle_uuid_t desc_list[4];
} mible_gattc_char_db_t;

typedef struct{
    mible_uuid_t srv_uuid;
    uint16_t srv_begin_handle;
    uint16_t srv_end_handle;
    uint32_t char_num;
    mible_gattc_char_db_t char_list[0];
} mible_gattc_service_db_t;

typedef struct{
    uint32_t status;	//mible_status_t
    mible_gattc_service_db_t *p_gattc_db;
}mible_gattc_discovery_rsp_t;

/*
 * MIBLE_GATTC_EVT_READ_CHAR_VALUE_BY_UUID_RESP event callback paramters
 * */
typedef struct {
    bool succ; // true: exist the specified characteristic and return correctly
    uint16_t value_handle;
    uint8_t len;
    uint8_t* data;
} mible_gattc_read_rsp_t;

/*
 * MIBLE_GATTC_EVT_WRITE_RESP event callback parameters
 *  */
typedef struct {
    uint16_t value_handle;
    bool succ;
} mible_gattc_write_rsp_t;

/*
 * MIBLE_GATTC_EVT_NOTIFICATION or MIBLE_GATTC_EVT_INDICATION event callback parameters
 *  */
typedef struct {
    uint16_t value_handle;
    uint8_t  len;
    uint8_t  *data;
} mible_gattc_notification_or_indication_t;

/*
 * GATTC callback parameters union
 * */
typedef struct {
    uint16_t conn_handle;
    uint16_t rsvd;
    union {
        mible_gattc_discovery_rsp_t discovery_rsp;
        mible_gattc_read_rsp_t read_rsp;
        mible_gattc_write_rsp_t write_rsp;
        mible_gattc_notification_or_indication_t notification;
    };
} mible_gattc_evt_param_t;

typedef struct {
    uint8_t addr[MIBLE_ADDR_LEN];
    uint8_t type; //mible_addr_type_t type;
    uint8_t rsvd;
} mible_device_param_t;

typedef enum {
    DISABLE_NOTIFICATION_VALUE=0x00,   /*Value used to disable notifications or indicatinos*/
    ENABLE_NOTIFICATION_VALUE=0x01,      /*Value used to enable notification for a CCCD*/
    ENABLE_INDICATION_VALUE=0x02,        /*Value used to enable indication for a CCCD*/
}mible_notify_value_t;

typedef enum {
    MIBLE_DISCONNECTED=0,
    MIBLE_CONNECTING,
    MIBLE_CONNECTED,
    MIBLE_DISCONNECTING,
} mible_conn_state_t;


/* TIMER related */
typedef void (*mible_timer_handler)(void*);

typedef enum {
    MIBLE_TIMER_SINGLE_SHOT,
    MIBLE_TIMER_REPEATED,
} mible_timer_mode;

/* IIC related */
typedef enum {
    IIC_100K = 1,
    IIC_400K,
} iic_freq_t;

typedef struct {
    uint8_t scl_port;
	uint8_t scl_pin;
	uint8_t scl_extra_conf;
	uint8_t sda_port;
    uint8_t sda_pin;
	uint8_t sda_extra_conf;
    iic_freq_t freq;
} iic_config_t;

typedef enum {
    IIC_EVT_XFER_DONE,
    IIC_EVT_ADDRESS_NACK,
    IIC_EVT_DATA_NACK
} iic_event_t;

typedef enum {
    MI_SUCCESS = 0,
    MI_ERR_INTERNAL = -1,
    MI_ERR_TIMEOUT = -2,
    MI_ERR_FULL = -3,
    MI_ERR_EMPTY = -4,
    MI_ERR_NO_MEM = -5,
    MI_ERR_NOSYS = -6,
    MI_ERR_BUSY = -7,
    MI_ERR_TRYOUT = -8,
    MI_ERR_NOT_FOUND = -9,
    MI_ERR_INVALID_PARAM = -10,
    MI_ERR_INVALID_LENGTH = -21,
    MI_ERR_INVALID_STATE = -22,
    MI_ERR_INVALID_ADDR = -23,
    MI_ERR_NO_EVENT = -24,
    MI_ERR_DATA_SIZE = -11,
    MI_ERR_NOTREADY = -12,
    MI_ERR_RESOURCES = -13,
    MIBLE_ERR_INVALID_CONN_HANDLE = -31,
    MIBLE_ERR_ATT_INVALID_ATT_HANDLE = -32,
    MIBLE_ERR_GAP_INVALID_BLE_ADDR = -33,
    MIBLE_ERR_GATT_INVALID_ATT_TYPE = -34,
    MIBLE_ERR_UNKNOWN = -35,
} mible_status_t;

typedef void (*mible_handler_t) (void* arg);

typedef enum{
    MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP,
    MIBLE_ARCH_EVT_RECORD_WRITE,
    MIBLE_ARCH_EVT_RECORD_DELETE,
} mible_arch_event_t;

typedef struct{
    mible_status_t status;
    mible_gatts_db_t *p_gatts_db;
}mible_arch_gatts_srv_init_cmp_t;

typedef struct{
    uint16_t id;
    mible_status_t status;
}mible_arch_record_t;

typedef struct{
    union {
        mible_arch_gatts_srv_init_cmp_t srv_init_cmp;
        mible_arch_record_t record;
    };
}mible_arch_evt_param_t;

typedef void (*mible_gap_callback_t)(mible_gap_evt_t evt,
    mible_gap_evt_param_t* param);

typedef void (*mible_gatts_callback_t)(mible_gatts_evt_t evt,
    mible_gatts_evt_param_t* param);

typedef void (*mible_gattc_callback_t)(mible_gattc_evt_t evt,
    mible_gattc_evt_param_t* param);

typedef void (*mible_arch_callback_t)(mible_arch_event_t evt,
        mible_arch_evt_param_t* param);


#endif

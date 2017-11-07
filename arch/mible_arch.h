#ifndef MIBLE_ARCH_H__
#define MIBLE_ARCH_H__

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
#include "mible_type.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

void mible_gap_event_callback(mible_gap_evt_t evt,
    mible_gap_evt_param_t* param);

void mible_gatts_event_callback(mible_gatts_evt_t evt,
    mible_gatts_param_t* param);

void mible_gattc_event_callback(mible_gattc_evt_t evt,
    mible_gattc_param_t* param);

mible_status_t mible_gap_address_get(mible_addr_t* p_mac);

mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
    mible_gap_scan_param_t scan_param);

mible_status_t mible_gap_scan_stop(void);

mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_adv_param);

mible_status_t mible_gap_adv_stop(void);

mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
    mible_gap_connect_t conn_param);

mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
    mible_gap_conn_param_t conn_params);

mible_status_t mible_gap_disconnect(uint16_t conn_handle);

mible_status_t mible_gatts_service_init(mible_gatts_db_t *mible_service_database);

mible_status_t mible_gatts_value_set(uint16_t srv_handle, uint16_t char_handle,
    uint8_t offset, uint8_t* buf, uint8_t len);

mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t char_handle,
    uint8_t* buf, uint8_t len);

mible_status_t mible_gatts_notify_or_indicate(uint16_t srv_handle,
    uint16_t char_handle,
    uint8_t offset, uint8_t* p_value,
    uint8_t len);

mible_status_t
mible_gattc_primary_service_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t* p_srv_uuid);

mible_status_t
mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t* p_char_uuid);

mible_status_t
mible_gattc_clt_cfg_descriptor_discover(uint16_t conn_handle,
    mible_handle_range_t handle_range);

mible_status_t
mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t char_uuid);

mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle, uint16_t handle,
    uint8_t* p_value, uint8_t len);

mible_status_t mible_gattc_write_cmd(uint16_t conn_handle, uint16_t handle,
    uint8_t* p_value, uint8_t len);

mible_status_t mible_timer_create(void* timer_id,
    mible_timer_handler timeout_handler,
    mible_timer_mode mode);

mible_status_t mible_timer_delete(void* timer_id);

mible_status_t mible_timer_start(void* timer_id, uint32_t timeout_value,
    void* p_context);

mible_status_t mible_timer_stop(void* timer_id);

mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data,
    uint8_t len);

mible_status_t mible_record_write(uint16_t record_id, uint8_t* p_data,
    uint8_t len);

mible_status_t mible_rand_num_generater(uint8_t* p_buf, uint8_t len);

mible_status_t mible_ecb128_encrypt(const uint8_t* key,
    const uint8_t* plaintext, uint8_t plen,
    uint8_t* ciphertext);
mible_status_t mible_task_post(mible_handler_t handler, void *param);
#endif

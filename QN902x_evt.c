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

#include "mible_api.h"
#include "mible_port.h"
#include "mible_type.h"
/*
 * Add your own include file
 *
 * */
#include "app_env.h"

mible_gatts_evt_param_t gatts_params = {0};
void mible_Callback (ke_msg_id_t const msgid, void const *param)
{
    mible_gap_evt_param_t   mible_param;
    
    switch(msgid)
    {
        case GAP_DISCON_CMP_EVT:
            mible_param.conn_handle       = ((struct gap_discon_cmp_evt *)param)->conhdl;
            mible_param.disconnect.reason = (mible_gap_disconnect_reason_t)((struct gap_discon_cmp_evt *)param)->reason;
            
            mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNET, &mible_param);
            break;

        case GAP_LE_CREATE_CONN_REQ_CMP_EVT:
        {
            struct gap_link_info *p_conn_info = &((struct gap_le_create_conn_req_cmp_evt *)param)->conn_info;
            if(p_conn_info->status == CO_ERROR_NO_ERROR)
            {
                mible_param.conn_handle = p_conn_info->conhdl;
                mible_param.connect.conn_param.conn_sup_timeout     = p_conn_info->sup_to;
                mible_param.connect.conn_param.max_conn_interval    = p_conn_info->con_interval;
                mible_param.connect.conn_param.min_conn_interval    = p_conn_info->con_interval;
                mible_param.connect.conn_param.slave_latency        = p_conn_info->con_latency;
                memcpy(mible_param.connect.peer_addr, p_conn_info->peer_addr.addr, BD_ADDR_LEN);
                mible_param.connect.role = MIBLE_GAP_CENTRAL;
                mible_param.connect.type = (mible_addr_type_t)p_conn_info->peer_addr_type;
                mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &mible_param) ;
            }
            break;
        }
        case GAP_CHANGE_PARAM_REQ_CMP_EVT:
        {
            struct gap_change_param_req_cmp_evt *p_info = (struct gap_change_param_req_cmp_evt *)param;
            
            mible_param.update_conn.conn_param.conn_sup_timeout     = p_info->sup_to;
            mible_param.update_conn.conn_param.max_conn_interval    = p_info->con_interval;
            mible_param.update_conn.conn_param.min_conn_interval    = p_info->con_interval;
            mible_param.update_conn.conn_param.slave_latency        = p_info->con_latency;
            
            mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &mible_param);
            break;
        }
        case GATT_WRITE_CMD_IND:
        {
            struct gatt_write_cmd_ind *write_cmd_ind_param = (struct gatt_write_cmd_ind *)param;
            gatts_params.conn_handle = write_cmd_ind_param->conhdl;
            gatts_params.write.len = write_cmd_ind_param->length;
            gatts_params.write.data = write_cmd_ind_param->value;
            gatts_params.write.offset = write_cmd_ind_param->offset;
//            gatts_params.write.permit;
            gatts_params.write.value_handle = write_cmd_ind_param->handle;
            mible_gatts_event_callback(MIBLE_GATTS_EVT_WRITE, &gatts_params);
            break;
        }
        default:
            break;
    }
}


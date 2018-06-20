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
#include "gatt_client_interface.h"
#include "gatt_server_interface.h"

void mible_ConnectionCallback (deviceId_t peerDeviceId, gapConnectionEvent_t* pConnectionEvent)
{
    mible_gap_evt_param_t   mible_param;
    
    mible_param.conn_handle = peerDeviceId;
    
    switch (pConnectionEvent->eventType)
    {
        case gConnEvtConnected_c:
        {
            gapConnectedEvent_t* pGapConn = &(pConnectionEvent->eventData.connectedEvent);

            mible_param.connect.conn_param.conn_sup_timeout     = pGapConn->connParameters.supervisionTimeout;
            mible_param.connect.conn_param.max_conn_interval    = pGapConn->connParameters.connInterval;
            mible_param.connect.conn_param.min_conn_interval    = pGapConn->connParameters.connInterval;
            mible_param.connect.conn_param.slave_latency        = pGapConn->connParameters.connLatency;
            FLib_MemCpy(mible_param.connect.peer_addr, pGapConn->peerAddress, gcBleDeviceAddressSize_c);
            mible_param.connect.role = MIBLE_GAP_CENTRAL;
            mible_param.connect.type = (mible_addr_type_t)pGapConn->peerAddressType;
            mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &mible_param) ;
        }
        break;
        
        case gConnEvtDisconnected_c:
        {
            gapDisconnectedEvent_t* pGapDisconn = &(pConnectionEvent->eventData.disconnectedEvent);
            mible_param.disconnect.reason = (mible_gap_disconnect_reason_t)pGapDisconn->reason;
            
            mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNET, &mible_param);
        }
        break;
        
        case gConnEvtParameterUpdateComplete_c:
        {
            gapConnParamsUpdateComplete_t* pGapParaUpdate = &(pConnectionEvent->eventData.connectionUpdateComplete);
            
            mible_param.update_conn.conn_param.conn_sup_timeout     = pGapParaUpdate->supervisionTimeout;
            mible_param.update_conn.conn_param.max_conn_interval    = pGapParaUpdate->connInterval;
            mible_param.update_conn.conn_param.min_conn_interval    = pGapParaUpdate->connInterval;
            mible_param.update_conn.conn_param.slave_latency        = pGapParaUpdate->connLatency;
            
            mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &mible_param);
        }
        break;
    default:
        break;
    }
}
mible_gatts_evt_param_t gatts_params = {0};
void mible_GattServerCallback (deviceId_t deviceId, gattServerEvent_t* pServerEvent)
{
    
    gatts_params.conn_handle = deviceId;
    
    switch (pServerEvent->eventType)
    {
        case gEvtAttributeWritten_c:
        {
            gatts_params.write.len = pServerEvent->eventData.attributeWrittenEvent.cValueLength;
            gatts_params.write.data = pServerEvent->eventData.attributeWrittenEvent.aValue;
            gatts_params.write.offset = 0;
//            gatts_params.write.permit;
            gatts_params.write.value_handle = pServerEvent->eventData.attributeWrittenEvent.handle;;
            mible_gatts_event_callback(MIBLE_GATTS_EVT_WRITE, &gatts_params);
        }
        break;
        
        case gEvtAttributeWrittenWithoutResponse_c:
        {
            gatts_params.write.len = pServerEvent->eventData.attributeWrittenEvent.cValueLength;
            gatts_params.write.data = pServerEvent->eventData.attributeWrittenEvent.aValue;
            gatts_params.write.offset = 0;
//            gatts_params.write.permit;
            gatts_params.write.value_handle = pServerEvent->eventData.attributeWrittenEvent.handle;;
            mible_gatts_event_callback(MIBLE_GATTS_EVT_WRITE, &gatts_params);
        }
        break;
        
    default:
        break;
    }
}


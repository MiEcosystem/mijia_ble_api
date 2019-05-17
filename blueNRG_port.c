/******************** (C) COPYRIGHT 2015 STMicroelectronics ********************
* File Name          : blueNRG_port.c
* Author             : AMG - GCSA Shanghai
* Version            : V0.0.1
* Date               : 16-Nov-2017
* Description        : File to port BLUENRG SDK with MiBle eco
********************************************************************************
* THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
* WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE TIME.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*******************************************************************************/
#include <stdio.h>
#include <string.h>
#include "mible_api.h"
#include "mible_log.h"
#include "mible_type.h"

#include "ble_const.h" 
#include "bluenrg1_stack.h"
#include "gp_timer.h"
#include "SDK_EVAL_Config.h"
#include "mible_type.h"

#include "osal.h"
#include "blueNRG_port.h"

static mible_uuid_t bnUUID;
static mible_handle_range_t bnHdlRange;


typedef enum{
  BN_SERVDISPROCESS_IDLE,
  BN_SERVDISPROCESS_PRIMARYSERVDIS,
  BN_SERVDISPROCESS_CHARDISCBYUUID,
  BN_SERVDISPROCESS_CCCFIND,
  BN_SERVDISPROCESS_READVALUEBYUUID
}bn_servDiscovery_st;


uint8_t bnStatus = BN_SERVDISPROCESS_IDLE;
unsigned short bnRand(void ){
   return (unsigned char )RNG->VAL;


}

static mible_status_t blueSt2MiStConvert(tBleStatus sta){

  if(sta == BLE_STATUS_INVALID_HANDLE){
    return MIBLE_ERR_INVALID_CONN_HANDLE;
  }
  if(sta == BLE_STATUS_INSUFFICIENT_RESOURCES){
    return MI_ERR_BUSY;
  }
  
  if(sta == BLE_STATUS_INVALID_PARAMS){
    return MI_ERR_INVALID_PARAM;
  }
  /*test*/
   if(sta == BLE_STATUS_SUCCESS){
    return MI_SUCCESS;
  }
  return (mible_status_t)sta;
}

/**
  * @brief The LE Advertising Report event indicates that a Bluetooth device or multiple
Bluetooth devices have responded to an active scan or received some information
during a passive scan. The Controller may queue these advertising reports
and send information from multiple devices in one LE Advertising Report event.
  * @param Num_Reports Number of responses in this event.
  * Values:
  - 0x01
  * @param Advertising_Report See @ref Advertising_Report_t
  * @retval None
*/
void hci_le_advertising_report_event(uint8_t Num_Reports,
                                     Advertising_Report_t Advertising_Report[])
{
  mible_gap_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  uint8_t i;
  Advertising_Report_t *r;
  for(i=0;i<Num_Reports;i++){
    r = &Advertising_Report[i];
    param.report.addr_type = (mible_addr_type_t)r->Address_Type;
    param.report.adv_type = (r->Event_Type == SCAN_RSP)?SCAN_RSP_DATA:ADV_DATA;
    param.report.data_len = r->Length_Data;
    Osal_MemCmp(param.report.data,r->Data,r->Length_Data);
    Osal_MemCmp(param.report.peer_addr,r->Address,6);
    param.report.rssi = r->RSSI;
    mible_gap_event_callback(MIBLE_GAP_EVT_ADV_REPORT,&param);
  }
  
}	
void aci_l2cap_connection_update_req_event(uint16_t Connection_Handle,
                                           uint8_t Identifier,
                                           uint16_t L2CAP_Length,
                                           uint16_t Interval_Min,
                                           uint16_t Interval_Max,
                                           uint16_t Slave_Latency,
                                           uint16_t Timeout_Multiplier)
{
    printf("received cnn update req \r\n");  
    printf("Connection_Handle=%x\r\n",Connection_Handle);
	printf("Identifier=%x\r\n",Identifier);
	printf("L2CAP_Length=%x\r\n",L2CAP_Length);
	printf("Interval_Min=%x\r\n",Interval_Min);
	printf("Interval_Max=%x\r\n",Interval_Max);
	printf("Slave_Latency=%x\r\n",Slave_Latency);
	printf("Timeout_Multiplier=%x\r\n",Timeout_Multiplier);
	/*
	tBleStatus st = aci_l2cap_connection_parameter_update_resp( Connection_Handle,
                                                       Interval_Min,
                                                       Interval_Max,
                                                       Slave_Latency,
                                                       Timeout_Multiplier,
                                                      0xffff,
                                                      0xffff,
                                                       Identifier,
                                                       0x01);
	printf("aci_l2cap_connection_parameter_update_resp =%d \r\n",st);
	*/
}
										   
void aci_l2cap_connection_update_resp_event(uint16_t Connection_Handle,
                                            uint16_t Result)
{
   printf("receive resp\r\n");
  
                                     	
}
/**
  * @brief The LE Connection Update Complete event is used to indicate that the Controller
process to update the connection has completed.
On a slave, if no connection parameters are updated, then this event shall not be issued.
On a master, this event shall be issued if the Connection_Update command was sent.
  * @param Status Error code. See Core v4.1, Vol. 2, part D.
  * @param Connection_Handle Connection handle to be used to identify the connection with the peer device.
  * Values:
  - 0x0000 ... 0x0EFF
  * @param Conn_Interval Connection interval used on this connection.
Time = N * 1.25 msec
  * Values:
  - 0x0006 (7.50 ms)  ... 0x0C80 (4000.00 ms) 
  * @param Conn_Latency Slave latency for the connection in number of connection events.
  * Values:
  - 0x0000 ... 0x01F3
  * @param Supervision_Timeout Supervision timeout for the LE Link.
It shall be a multiple of 10 ms and larger than (1 + connSlaveLatency) * connInterval * 2.
Time = N * 10 msec.
  * Values:
  - 0x000A (100 ms)  ... 0x0C80 (32000 ms) 
  * @retval None
*/
void hci_le_connection_update_complete_event(uint8_t Status,
                                             uint16_t Connection_Handle,
                                             uint16_t Conn_Interval,
                                             uint16_t Conn_Latency,
                                             uint16_t Supervision_Timeout)
{
  mible_gap_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  param.update_conn.conn_param.conn_sup_timeout = Supervision_Timeout;
  param.update_conn.conn_param.max_conn_interval = Conn_Interval;
  param.update_conn.conn_param.min_conn_interval = Conn_Interval;
  
  param.update_conn.conn_param.slave_latency = Conn_Latency;
    
  mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED,&param);
}

/*******************************************************************************
 * Function Name  : hci_le_connection_complete_event.
 * Description    : This event indicates that a new connection has been created.
 * Input          : See file bluenrg1_events.h
 * Output         : See file bluenrg1_events.h
 * Return         : See file bluenrg1_events.h
 *******************************************************************************/
void hci_le_connection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Role,
                                      uint8_t Peer_Address_Type,
                                      uint8_t Peer_Address[6],
                                      uint16_t Conn_Interval,
                                      uint16_t Conn_Latency,
                                      uint16_t Supervision_Timeout,
                                      uint8_t Master_Clock_Accuracy)

{ 
  mible_gap_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  param.connect.role = (Role == 0)?MIBLE_GAP_CENTRAL:MIBLE_GAP_PERIPHERAL;
  
  param.connect.type = (mible_addr_type_t)Peer_Address_Type;
  Osal_MemCpy(param.connect.peer_addr,Peer_Address,6);
  
  param.connect.conn_param.conn_sup_timeout = Supervision_Timeout;
  param.connect.conn_param.max_conn_interval = Conn_Interval;
  
  param.connect.conn_param.min_conn_interval =Conn_Interval;
  
  param.connect.conn_param.slave_latency = Conn_Latency;
  
  mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED,&param);
  
}/* end hci_le_connection_complete_event() */

/*******************************************************************************
 * Function Name  : hci_disconnection_complete_event.
 * Description    : This event occurs when a connection is terminated.
 * Input          : See file bluenrg1_events.h
 * Output         : See file bluenrg1_events.h
 * Return         : See file bluenrg1_events.h
 *******************************************************************************/
void hci_disconnection_complete_event(uint8_t Status,
                                      uint16_t Connection_Handle,
                                      uint8_t Reason)
{
  mible_gap_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  switch(Reason){
    case BLE_ERROR_CONNECTION_TIMEOUT:{
      param.disconnect.reason = CONNECTION_TIMEOUT;
    }
    break;
    case BLE_ERROR_TERMINATED_REMOTE_USER:{
      param.disconnect.reason = REMOTE_USER_TERMINATED;
    }
    break;
    case BLE_ERROR_TERMINATED_LOCAL_HOST:{
      param.disconnect.reason = LOCAL_HOST_TERMINATED;
    }
    break;
    default:{
     param.disconnect.reason = (mible_gap_disconnect_reason_t)0xff;
    }
    break;
  }
  
  mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNET,&param);
  
}/* end hci_disconnection_complete_event() */


/**
  * @brief This event is given to the application when a write request, write command or signed write
command is received by the server from the client. This event will be given to the application
only if the event bit for this event generation is set when the characteristic was added.
When this event is received, the application has to check whether the value being requested
for write can be allowed to be written and respond with the command @ref aci_gatt_write_resp.
The details of the parameters of the command can be found. Based on the response from
the application, the attribute value will be modified by the stack. If the write is rejected by the
application, then the value of the attribute will not be modified. In case of a write REQ, an
error response will be sent to the client, with the error code as specified by the application.
In case of write/signed write commands, no response is sent to the client but the attribute is
not modified.
  * @param Connection_Handle Handle of the connection on which there was the request to write the attribute
  * @param Attribute_Handle The handle of the attribute
  * @param Data_Length Length of Data field
  * @param Data The data that the client has requested to write
  * @retval None
*/
void aci_gatt_write_permit_req_event(uint16_t Connection_Handle,
                                     uint16_t Attribute_Handle,
                                     uint8_t Data_Length,
                                     uint8_t Data[])
{
  printf("aci_gatt_write_permit_req_event\r\n");
  mible_gatts_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  param.write.len = Data_Length;
  param.write.data = Data;
  param.write.value_handle = Attribute_Handle-1;
  mible_gatts_event_callback(MIBLE_GATTS_EVT_WRITE_PERMIT_REQ, &param);
}
 void aci_gatt_prepare_write_permit_req_event(uint16_t Connection_Handle,
                                             uint16_t Attribute_Handle,
                                             uint16_t Offset,
                                             uint8_t Data_Length,
                                             uint8_t Data[])
{
   printf("aci_gatt_prepare_write_permit_req_event\r\n");
}
											 
/**
  * @brief This event is given to the application when a read request or read blob request is received
by the server from the client. This event will be given to the application only if the event bit
for this event generation is set when the characteristic was added.
On receiving this event, the application can update the value of the handle if it desires and
when done, it has to send the @ref aci_gatt_allow_read command to indicate to the stack that it
can send the response to the client.
  * @param Connection_Handle Connection handle related to the response
  * @param Attribute_Handle The handle of the attribute
  * @param Offset Contains the offset from which the read has been requested
  * @retval None
*/
void aci_gatt_read_permit_req_event(uint16_t Connection_Handle,
                                    uint16_t Attribute_Handle,
                                    uint16_t Offset)
{
  printf("aci_gatt_read_permit_req_event\r\n");
  mible_gatts_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  param.read.value_handle = Attribute_Handle-1;
  //param.read.conn_handle = Connection_Handle;
  mible_gatts_event_callback(MIBLE_GATTS_EVT_READ_PERMIT_REQ, &param);
}
void aci_gatt_proc_complete_event(uint16_t Connection_Handle,
																	  uint8_t Error_Code)
{
  printf("aci_gatt_proc_complete_event\r\n");

}

/**
  * @brief This event is generated to the application by the GATT server when a client modifies any
attribute on the server, as consequence of one of the following GATT procedures:
- write without response
- signed write without response
- write characteristic value
- write long characteristic value
- reliable write.
  * @param Connection_Handle The connection handle which modified the attribute.
  * @param Attr_Handle Handle of the attribute that was modified.
  * @param Offset Offset from which the write has been performed by the peer device.
  * @param Attr_Data_Length Length of Attr_Data in octets
  * @param Attr_Data The modified value
  * @retval None
*/
void aci_gatt_attribute_modified_event(uint16_t Connection_Handle,
                                       uint16_t Attr_Handle,
                                       uint16_t Offset,
                                       uint16_t Attr_Data_Length,
                                       uint8_t Attr_Data[])
{

  printf("aci_gatt_attribute_modified_event\r\n");
  mible_gatts_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  param.write.value_handle = Attr_Handle-1;
  param.write.len = Attr_Data_Length;
  param.write.data = Attr_Data;
  param.write.offset = Offset;
  mible_gatts_event_callback(MIBLE_GATTS_EVT_WRITE, &param);
}

#if 0
/**
  * @brief This event is generated when an indication is received from the server.
  * @param Connection_Handle Connection handle related to the response
  * @param Attribute_Handle The handle of the attribute
  * @param Attribute_Value_Length Length of Attribute_Value in octets
  * @param Attribute_Value The current value of the attribute
  * @retval None
*/
void aci_gatt_indication_event(uint16_t Connection_Handle,
                               uint16_t Attribute_Handle,
                               uint8_t Attribute_Value_Length,
                               uint8_t Attribute_Value[])
{ 
#if CLIENT
  uint16_t attr_handle;
 
  attr_handle = Attribute_Handle;
    if(attr_handle == tx_handle+1)
    {
      for(int i = 0; i < Attribute_Value_Length; i++) 
          printf("%c", Attribute_Value[i]);
    }
#endif
}
/**
  * @brief This event is generated when a notification is received from the server.
  * @param Connection_Handle Connection handle related to the response
  * @param Attribute_Handle The handle of the attribute
  * @param Attribute_Value_Length Length of Attribute_Value in octets
  * @param Attribute_Value The current value of the attribute
  * @retval None
*/
void aci_gatt_notification_event(uint16_t Connection_Handle,
                                 uint16_t Attribute_Handle,
                                 uint8_t Attribute_Value_Length,
                                 uint8_t Attribute_Value[])
{ 
#if CLIENT
  uint16_t attr_handle;
 
  attr_handle = Attribute_Handle;
    if(attr_handle == tx_handle+1)
    {
      for(int i = 0; i < Attribute_Value_Length; i++) 
          printf("%c", Attribute_Value[i]);
    }
#endif
}
#endif

 
mible_status_t bnReadMacAddr(uint8_t *addr){
  return blueSt2MiStConvert(hci_read_bd_addr(addr)); 
}
    
/**
  * @brief dosen't support variable timeout, timeout must be 1
*/

mible_status_t bnsetScanParam(mible_gap_scan_type_t scanType,mible_gap_scan_param_t *sp){
  
  /*
  if(sp->timeout != 1){
    return MI_ERR_INVALID_PARAM;
  }
  */
  
  //tBleStatus st = hci_le_set_scan_parameters(scanType,sp->scan_interval,sp->scan_window,
    //                                  1,1);
 //tBleStatus st = aci_gap_start_general_connection_establish_proc(scanType,sp->scan_interval,sp->scan_window,
   //                                   1,1);
  //return blueSt2MiStConvert(st);  
   return   MI_ERR_INVALID_PARAM;
}


mible_status_t bnEnDisScan(uint8_t enScan){
  return blueSt2MiStConvert(hci_le_set_scan_enable(enScan,0));
}


mible_status_t bnsetAdvParam(mible_gap_adv_param_t *ap){

  uint8_t at;
  switch(ap->adv_type){
  case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:{
    at = ADV_IND;
  }
  break;
  case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:{
    at = ADV_SCAN_IND;
    
  }
  break;
  case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:{
    at = ADV_NONCONN_IND;
  }
  default:
  break;  
  }
  uint8_t directAddr[6] = {0xff,0xff,0xff,0xff,0xff,0xff};
  uint8_t cmap = *((uint8_t *)&ap->ch_mask);
  cmap = ~cmap;
  printf("Enable ADV channel mask %d\r\n",cmap);
  tBleStatus st = hci_le_set_advertising_parameters(ap->adv_interval_min,ap->adv_interval_max,
                                      at,0,0,directAddr,cmap,0);
  /*
  st = hci_le_set_advertising_data(ap->adv_len,ap->adv_data);
  if(st != ERR_CMD_SUCCESS){
    //return blueSt2MiStConvert(st);
  }
  st = hci_le_set_scan_response_data(ap->scan_rsp_len,ap->scan_rsp_data);
  if(st != ERR_CMD_SUCCESS){
    //return blueSt2MiStConvert(st);
  }
  */
  return blueSt2MiStConvert(st);

}


mible_status_t bnGapAdvDataSet(uint8_t const * p_data,
                              uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen){

        uint8_t * p_data_temp = (uint8_t *)p_data;
		uint8_t * p_sr_data_temp = (uint8_t *)p_sr_data;
		
	           hci_le_set_scan_response_data(srdlen,p_sr_data_temp);
		return blueSt2MiStConvert(hci_le_set_advertising_data(dlen,p_data_temp));
		

}

mible_status_t bnEnDisAdv(uint8_t enAdv){
  return blueSt2MiStConvert(hci_le_set_advertise_enable(enAdv));
}


mible_status_t bnCreateConnection(mible_gap_scan_param_t *sp, mible_gap_connect_t *cp){

uint8_t peerAddrType = (cp->type == MIBLE_ADDRESS_TYPE_PUBLIC)?0:1;
uint8_t ownAddrType = 0;//public address type

tBleStatus st = aci_gap_create_connection(sp->scan_interval,sp->scan_window,
                                           peerAddrType,cp->peer_addr, ownAddrType,
                                           cp->conn_param.min_conn_interval,
                                           cp->conn_param.max_conn_interval,
                                           cp->conn_param.slave_latency,
                                           cp->conn_param.conn_sup_timeout,0xffff,0xffff);

/*
  uint8_t peerAddrType = (cp->type == MIBLE_ADDRESS_TYPE_PUBLIC)?0:1;
  uint8_t ownAddrType = 0;//public address type
  tBleStatus st = hci_le_create_connection(sp->scan_interval,sp->scan_window,
                                           0,peerAddrType,cp->peer_addr, ownAddrType,
                                           cp->conn_param.min_conn_interval,
                                           cp->conn_param.max_conn_interval,
                                           cp->conn_param.slave_latency,
                                           cp->conn_param.conn_sup_timeout,0xffff,0xffff);
                                           */
  
   return blueSt2MiStConvert(st);
  
}



mible_status_t bnDisconnect(uint16_t handle){
  return blueSt2MiStConvert(hci_disconnect(handle,0x13));//remote user disconnect
}

mible_status_t bnUpdateConnParam(uint16_t conHandle,uint16_t intervalMin, 
                 uint16_t intervalMax, uint16_t latency, uint16_t timeout)
{

  return blueSt2MiStConvert(aci_l2cap_connection_parameter_update_req(conHandle,
                                                                      intervalMin,
                                                                      intervalMax,
                                                                      latency,
                                                                        timeout));
}


mible_status_t bnAddService(mible_gatts_srv_db_t *sdb){  
  //Service
  //Character Declaration
  //Character Value
  uint8_t servUUIDtype = (sdb->srv_uuid.type)?UUID_TYPE_128:UUID_TYPE_16;
  uint8_t servType = (sdb->srv_type == MIBLE_PRIMARY_SERVICE)?PRIMARY_SERVICE:SECONDARY_SERVICE;
  uint8_t ret = aci_gatt_add_service(servUUIDtype, (Service_UUID_t *)sdb->srv_uuid.uuid128, servType, 30, &sdb->srv_handle); 
  if (ret != BLE_STATUS_SUCCESS){
    printf("Service add Error, error code 0x%x\n",ret);
    goto fail;    
  }
  printf("Service add success with service handle 0x%x\n",sdb->srv_handle);
  uint8_t i;
  mible_gatts_char_db_t *cdb;    
  for(i=0;i<sdb->char_num;i++){
    cdb = sdb->p_char_db + i;
    uint8_t charUUIDtype = (cdb->char_uuid.type)?UUID_TYPE_128:UUID_TYPE_16;
    uint8_t sp = ATTR_PERMISSION_NONE;
	uint8_t gatEventMask = 0;
    if(cdb->rd_author){
      sp |= ATTR_PERMISSION_AUTHOR_READ;
      gatEventMask |= GATT_NOTIFY_READ_REQ_AND_WAIT_FOR_APPL_RESP;
    }
    if(cdb->wr_author){
      sp |= ATTR_PERMISSION_AUTHOR_WRITE;
      gatEventMask |= GATT_NOTIFY_WRITE_REQ_AND_WAIT_FOR_APPL_RESP;
    }

    if(cdb->char_property&MIBLE_WRITE || cdb->char_property&MIBLE_WRITE_WITHOUT_RESP){
      gatEventMask |= GATT_NOTIFY_ATTRIBUTE_WRITE;
    }
	
    ret =  aci_gatt_add_char(sdb->srv_handle, charUUIDtype, (Char_UUID_t *)cdb->char_uuid.uuid128, cdb->char_value_len, 
                             (uint8_t )cdb->char_property, sp, gatEventMask, 16, cdb->is_variable_len, 
                             &cdb->char_value_handle);
    if (ret != BLE_STATUS_SUCCESS){
      printf("Attr add Error, error code 0x%x\n",ret);
      goto fail;    
    }
  }
  printf("Chat Service added.\n");
  return MI_SUCCESS; 
  fail:
  printf("Error while adding Chat service.\n");
  return MIBLE_ERR_UNKNOWN ;
}

mible_status_t bnReadLocalGattValue(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len)
{
  uint16_t realLen;
  uint16_t readLen;
  uint16_t attr_handle = value_handle+1;
  tBleStatus st = aci_gatt_read_handle_value(attr_handle,0,255,
                                             &realLen, &readLen,p_value);
  printf("the realLen =%d\r\n",realLen);
  printf("the readLen =%d\r\n",readLen);
  *p_len = 0;
  if(st == BLE_STATUS_SUCCESS){
    *p_len = (uint8_t )readLen;
  }
  
  return blueSt2MiStConvert(st);
}


mible_status_t bnUpdateCharacterValue(uint16_t conn_handle, uint16_t serv_handle, uint16_t char_value_handle, 
                                      uint8_t offset, uint8_t* p_value,
                                      uint8_t len, uint8_t type)
{

  return blueSt2MiStConvert(aci_gatt_update_char_value_ext(conn_handle,serv_handle,char_value_handle,
                                                           type,len,offset,len,p_value));
  
}


/**********************Event Dispatch**************************/

/**
  * @brief This event is generated in response to a @ref aci_att_find_by_type_value_req
  * @param Connection_Handle Connection handle related to the response
  * @param Num_of_Handle_Pair Number of attribute, group handle pairs
  * @param Attribute_Group_Handle_Pair See @ref Attribute_Group_Handle_Pair_t
  * @retval None
*/
void aci_att_find_by_type_value_resp_event(uint16_t Connection_Handle,
                                           uint8_t Num_of_Handle_Pair,
                                           Attribute_Group_Handle_Pair_t Attribute_Group_Handle_Pair[])
{
  mible_gattc_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  param.srv_disc_rsp.primary_srv_range.begin_handle = bnHdlRange.begin_handle;
  param.srv_disc_rsp.primary_srv_range.end_handle = bnHdlRange.end_handle;
  Osal_MemCpy((uint8_t *)&param.srv_disc_rsp.srv_uuid,(uint8_t *)&bnUUID,sizeof(bnUUID));
  param.srv_disc_rsp.succ = (Num_of_Handle_Pair)?1:0;
  mible_gattc_event_callback(MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP,&param);
}


mible_status_t bnPrimServDiscByUUID(uint16_t conn_handle, mible_handle_range_t handle_range, 
                                    mible_uuid_t *p_char_uuid)
{
  bnStatus =  BN_SERVDISPROCESS_PRIMARYSERVDIS;
  Osal_MemCpy((uint8_t *)&bnUUID,(uint8_t *)p_char_uuid,sizeof(*p_char_uuid));
  Osal_MemCpy((uint8_t *)&bnHdlRange,(uint8_t *)&handle_range,sizeof(handle_range));
  return blueSt2MiStConvert(aci_gatt_disc_primary_service_by_uuid(conn_handle,
                                                     ((uint8_t )p_char_uuid->type),
                                                     ((UUID_t *)(p_char_uuid->uuid128))));
}





/**
  * @brief This event can be generated during a "Discover Characteristics By UUID" procedure or a
"Read using Characteristic UUID" procedure.
The attribute value will be a service declaration as defined in Bluetooth Core v4.1spec
(vol.3, Part G, ch. 3.3.1), when a "Discover Characteristics By UUID" has been started. It will
be the value of the Characteristic if a* "Read using Characteristic UUID" has been
performed.
  * @param Connection_Handle Connection handle related to the response
  * @param Attribute_Handle The handle of the attribute
  * @param Attribute_Value_Length Length of Attribute_Value in octets
  * @param Attribute_Value The attribute value will be a service declaration as defined in Bluetooth Core v4.0 spec
 (vol.3, Part G, ch. 3.3.1), when a "Discover Characteristics By UUID" has been started.
 It will be the value of the Characteristic if a "Read using Characteristic UUID" has been performed.
  * @retval None
*/

void aci_gatt_disc_read_char_by_uuid_resp_event(uint16_t Connection_Handle,
                                                uint16_t Attribute_Handle,
                                                uint8_t Attribute_Value_Length,
                                                uint8_t Attribute_Value[])
{
  mible_gattc_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  
  mible_gattc_evt_t evt;
  switch(bnStatus){
  case BN_SERVDISPROCESS_CCCFIND:{
    param.clt_cfg_desc_disc_rsp.desc_handle = Attribute_Handle;
    param.clt_cfg_desc_disc_rsp.succ = 1;
    evt = MIBLE_GATTC_EVT_CCCD_DISCOVER_RESP;
  }
  break;
  case BN_SERVDISPROCESS_CHARDISCBYUUID:{
   Osal_MemCpy((uint8_t *)&param.char_disc_rsp.uuid_type,(uint8_t *)&bnUUID,sizeof(bnUUID));
   param.char_disc_rsp.succ = 1;
   evt = MIBLE_GATTC_EVT_CHR_DISCOVER_BY_UUID_RESP;
  }
  break;
  case BN_SERVDISPROCESS_READVALUEBYUUID:{
    param.read_char_value_by_uuid_rsp.char_value_handle = Attribute_Handle;
    param.read_char_value_by_uuid_rsp.len = Attribute_Value_Length;
    param.read_char_value_by_uuid_rsp.data = Attribute_Value;
    param.read_char_value_by_uuid_rsp.succ = 1;
    evt = MIBLE_GATTC_EVT_READ_CHAR_VALUE_BY_UUID_RESP;
  }
  break;
  default:
    
    break;
    
  }
  
  mible_gattc_event_callback(evt,&param);
}


mible_status_t bnCharacterDiscByUUID(uint16_t conn_handle, mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid){
  
  if((p_char_uuid->type == 0) && (p_char_uuid->uuid16 == 0x2092)){
    bnStatus =  BN_SERVDISPROCESS_CCCFIND;
  }else{
    bnStatus =  BN_SERVDISPROCESS_CHARDISCBYUUID;    
  }
  return blueSt2MiStConvert(aci_gatt_disc_char_by_uuid(conn_handle,
                                                     handle_range.begin_handle,
                                                     handle_range.end_handle,
                                                     ((uint8_t )p_char_uuid->type),
                                                     ((UUID_t *)(p_char_uuid->uuid128))));
}






mible_status_t bnReadValueByUUID(uint16_t conn_handle, mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid){
  bnStatus =  BN_SERVDISPROCESS_READVALUEBYUUID;        
  return blueSt2MiStConvert(aci_gatt_read_using_char_uuid(conn_handle,
                                                     handle_range.begin_handle,
                                                     handle_range.end_handle,
                                                     ((uint8_t )p_char_uuid->type),
                                                     ((UUID_t *)(p_char_uuid->uuid128))));
}



/**
  * @brief This event is generated in response to an Execute Write Request.
  * @param Connection_Handle Connection handle related to the response
  * @retval None
*/
void aci_att_exec_write_resp_event(uint16_t Connection_Handle){
  mible_gattc_evt_param_t param;
  Osal_MemSet(&param,0,sizeof(param));
  param.conn_handle = Connection_Handle;
  param.write_rsp.succ = 1;
  mible_gattc_event_callback(MIBLE_GATTC_EVT_WRITE_RESP,&param);
}


mible_status_t bnGattWrite(unsigned char enRsp,uint16_t conn_handle, uint16_t att_handle, uint8_t* p_value, uint8_t len){
  if(!enRsp){
    return blueSt2MiStConvert(aci_gatt_write_without_resp(conn_handle,att_handle,len,p_value));
  }else{
    return blueSt2MiStConvert(aci_gatt_write_char_reliable(conn_handle,att_handle,0,len,p_value));
  }
  
}
mible_status_t bnGattREAuthorReply(uint16_t conn_handle,
        uint8_t status, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type)
{
	if(type==1){//read resp
		   if(status==1){//permit
			   return blueSt2MiStConvert(aci_gatt_allow_read(conn_handle));
		   }
		   else if(status==2){
			   printf("reject read\r\n");
			   return MI_ERR_TIMEOUT;
		   }
	   }
	   else if(type == 2){
		   if(status==1){
		   	 uint16_t Attr_Handle = char_value_handle + 1;
			   return blueSt2MiStConvert(aci_gatt_write_resp( conn_handle,
                               Attr_Handle,
                               0,
                               0x00,
                               len,
                               p_value));
		   }
		   else if(status==2){
                         uint16_t Attr_Handle = char_value_handle + 1;  
		   	return blueSt2MiStConvert(aci_gatt_write_resp( conn_handle,
                               Attr_Handle,
                               1,
                               0x00,
                               len,
                               p_value));
			   
		   }
	   }
        return MI_ERR_INVALID_PARAM;
  
}


unsigned char bnTimerStart(void *t){
  struct timer *info = t;
  Timer_Set(info,info->interval);
  return 0;
}

unsigned char bnTimerExpired(void *t){
  struct timer *info = t;
  return Timer_Expired(info);
  
}

unsigned char bnAesBlock(unsigned char* key,unsigned char *text, unsigned char *cipher){
  
  return hci_le_encrypt(key,text,cipher);
}


void st_hex_dump(uint8_t *base_addr,uint8_t bytes )
{
    int i;
	for(i=0;i<bytes;i++){
		if(i==0)
    printf("0x%02x",base_addr[i]);	
		else
	printf("%02x",base_addr[i]);			
		}
	//SdkEvalComIOSendData(base_addr[i]);


}


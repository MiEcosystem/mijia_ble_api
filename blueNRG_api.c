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
#include "BlueNRG1_flash.h"
#include "blueNRG_port.h"
#include <string.h>

#define        APP_TASKQUEUENUM                 4 //must be 2^n
#define        APP_TIMERQUEUENUM                 4 
#define _MEMORY_FLASH_BEGIN_  0x10040000
#define MI_FLASH_ADDR (_MEMORY_FLASH_BEGIN_+0x20000)//0x19000
#define FLASH_RECORD_ID_MANAGE_ADDR 0x10060000
#define FLASH_RECORD_DATA_ADDR (0x10060000+2048)
#define FLASH_RECORD_BACKUP_ADDR (0x10060000+2048+2048)
#define FLASH_RECORD_BACKUP_WRITE_OFFSET_ADDR  0x100613c0

#define FLASH_RECORD_ID_MANAGE_SIZE 96
#define FLASH_RECORD_WRITE_OFFSET_ADDR  0x100603C0


 uint32_t record_write_offset = 0xffff0000;
 uint32_t record_read_offset = 0;
 uint16_t record_write_offset_location = 0;
 uint16_t record_read_offset_location = 0;
uint16_t delete_id;
void record_data_reload(void *arg);
void record_id_manage_reload(void *arg);
uint8_t record_id_manage_backup(void);
uint8_t record_data_backup(void);
void record_id_delete_and_reload(void *arg);
void record_id_manage_reload_from_backup(void *arg);
void record_data_reload_from_backup(void *arg);


typedef struct{
  mible_handler_t taskHandle;
  void            *arg;
  
}appTask_t;

static unsigned char rePtrTask = 0;
static unsigned char wrPtrTask = 0;
appTask_t appTask[APP_TASKQUEUENUM] = {0};






typedef struct{
  //timer struct
  unsigned int             startTimeInternal;
  unsigned int             interval;

  mible_timer_handler      timerHandle;
  void                     *arg;
  unsigned short           mode;
  unsigned char           used;
  unsigned char           run;
}appTimer_t;

uint8_t timerRunning = 0;

appTimer_t appTimer[APP_TIMERQUEUENUM];


static unsigned char taskQueueFull(void ){
  return !((wrPtrTask - rePtrTask) < APP_TASKQUEUENUM);
}

appTimer_t *appGetFreeTimer(void ){
  
  for(uint8_t i = 0;i<APP_TIMERQUEUENUM;i++){
    if(!appTimer[i].used){
      appTimer[i].used = 1;
      return &appTimer[i];
    }
  }
  return NULL;
}



/**
 *        GAP APIs
 */

/**
 * @brief   Get BLE mac address.
 * @param   [out] mac: pointer to data
 * @return  MI_SUCCESS          The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note: 	You should copy gap mac to mac[6]  
 * */
mible_status_t mible_gap_address_get(mible_addr_t mac) 
{ 
   return bnReadMacAddr(mac); 
}

/**
 * @brief   Start scanning
 * @param   [in] scan_type: passive or active scaning
 *          [in] scan_param: scan parameters including interval, windows
 * and timeout
 * @return  MI_SUCCESS             Successfully initiated scanning procedure.
 *          MI_ERR_INVALID_STATE   Has initiated scanning procedure.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending
 * events and retry.
 * @note    Other default scanning parameters : public address, no
 * whitelist.
 *          The scan response is given through
 * MIBLE_GAP_EVT_ADV_REPORT event
 */
mible_status_t mible_gap_scan_start(mible_gap_scan_type_t scan_type,
    mible_gap_scan_param_t scan_param)
{

  mible_status_t st = bnsetScanParam(scan_type,&scan_param);
  return st;
/*
  if(st != MI_SUCCESS){
    return st;
  }
  return bnEnDisScan(1); 
*/
}

/**
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void) 
{ 
  return bnEnDisScan(0); 
}

/**
 * @brief   Start advertising
 * @param   [in] p_adv_param : pointer to advertising parameters, see
 * mible_gap_adv_param_t for details
 * @return  MI_SUCCESS             Successfully initiated advertising procedure.
 *          MI_ERR_INVALID_STATE   Initiated connectable advertising procedure
 * when connected.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 * retry.
 *          MI_ERR_RESOURCES       Stop one or more currently active roles
 * (Central, Peripheral or Observer) and try again.
 * @note    Other default advertising parameters: local public address , no
 * filter policy
 * */
 mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_param)
{
  if(!p_param){
    return MI_ERR_INVALID_PARAM;
  }
  mible_status_t st = bnsetAdvParam(p_param);
  if(st != MI_SUCCESS){
    return st;
  }
  return bnEnDisAdv(1); 
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
mible_status_t mible_gap_adv_data_set(uint8_t const * p_data,
        uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
      return  bnGapAdvDataSet(p_data,dlen,p_sr_data,srdlen);
}

/**
 * @brief   Stop advertising
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void) 
{
  return bnEnDisAdv(0); 
}

/**
 * @brief   Create a Direct connection
 * @param   [in] scan_param : scanning parameters, see TYPE
 * mible_gap_scan_param_t for details.
 *          [in] conn_param : connection parameters, see TYPE
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
  return bnCreateConnection(&scan_param,&conn_param);
}

/**
 * @brief   Disconnect from peer
 * @param   [in] conn_handle: the connection handle
 * @return  MI_SUCCESS             Successfully disconnected.
 *          MI_ERR_INVALID_STATE   Not in connnection.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note    This function can be used by both central role and periphral
 * role.
 * */
mible_status_t mible_gap_disconnect(uint16_t conn_handle) 
{ 
   return bnDisconnect(conn_handle); 
}

/**
 * @brief   Update the connection parameters.
 * @param   [in] conn_handle: the connection handle.
 *          [in] conn_params: the connection parameters.
 * @return  MI_SUCCESS             The Connection Update procedure has been
 *started successfully.
 *          MI_ERR_INVALID_STATE   Initiated this procedure in disconnected
 *state.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_BUSY            The stack is busy, process pending events and
 *retry.
 *          MIBLE_ERR_INVALID_CONN_HANDLE
 * @note    This function can be used by both central role and peripheral
 *role.
 * */
mible_status_t mible_gap_update_conn_params(uint16_t conn_handle,
    mible_gap_conn_param_t conn_params)
{
    return bnUpdateConnParam(conn_handle,conn_params.min_conn_interval,
                             conn_params.max_conn_interval,conn_params.slave_latency,conn_params.conn_sup_timeout);
}

/**
 *        GATT Server APIs
 */

/**
 * @brief   Add a Service to a GATT server
 * @param   [in|out] p_server_db: pointer to mible service data type 
 * of mible_gatts_db_t, see TYPE mible_gatts_db_t for details. 
 * @return  MI_SUCCESS             Successfully added a service declaration.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 *          MI_ERR_NO_MEM          Not enough memory to complete operation.
 * @note    This function can be implemented asynchronous. When service inition complete, call mible_arch_event_callback function and pass in MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP event and result.
 * */
 mible_status_t mible_gatts_service_init(mible_gatts_db_t *p_server_db)
{
  if(!p_server_db||!p_server_db->p_srv_db||
     !p_server_db->p_srv_db->p_char_db){
    return MI_ERR_INVALID_ADDR;
  }
  
  uint8_t i;
  mible_status_t st;
  for(i=0;i<p_server_db->srv_num;i++){
    st = bnAddService(p_server_db->p_srv_db);
    if(st!=MI_SUCCESS){
      break;
    }
  }
  //recall the call back function
  mible_arch_evt_param_t param;
  memset((uint8_t *)&param, 0, sizeof(param));
  param.srv_init_cmp.status = st;
  param.srv_init_cmp.p_gatts_db = p_server_db;
  mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
  return st;
}

/**
 * @brief   Set characteristic value
 * @param   [in] srv_handle: service handle
 *          [in] value_handle: characteristic value handle
 *          [in] offset: the offset from which the attribute value has
 *to be updated
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully retrieved the value of the
 *attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE  Attributes are not modifiable by
 *the application.
 * */
mible_status_t mible_gatts_value_set(uint16_t srv_handle, 
uint16_t value_handle, uint8_t offset, uint8_t* p_value, uint8_t len)
{
  if(!p_value){
    return MI_ERR_INVALID_ADDR;
  }
  if(!len){
    return MI_ERR_INVALID_LENGTH;
  }
  
  return bnUpdateCharacterValue(0,srv_handle,value_handle,offset,p_value,len,0);
}

/**
 * @brief   Get charicteristic value as a GATTS.
 * @param   [in] srv_handle: service handle
 *          [in] value_handle: characteristic value handle
 *          [out] p_value: pointer to data which stores characteristic value
 *          [out] p_len: pointer to data length.
 * @return  MI_SUCCESS             Successfully get the value of the attribute.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter (offset) supplied.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MIBLE_ERR_ATT_INVALID_HANDLE     Attribute not found.
 **/
mible_status_t mible_gatts_value_get(uint16_t srv_handle, uint16_t value_handle,
    uint8_t* p_value, uint8_t *p_len)
{
  if(!p_value||!p_len){
    return MI_ERR_INVALID_ADDR;
  }
  return bnReadLocalGattValue(srv_handle,value_handle,p_value,p_len);
}

/**
 * @brief   Set characteristic value and notify it to client.
 * @param   [in] conn_handle: conn handle
 *          [in] srv_handle: service handle
 *          [in] char_value_handle: characteristic  value handle
 *          [in] offset: the offset from which the attribute value has to
 * be updated
 *          [in] p_value: pointer to data
 *          [in] len: data length
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
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE   //Attributes are not modifiable by
 * the application.
 * @note    This function checks for the relevant Client Characteristic
 * Configuration descriptor value to verify that the relevant operation (notification or
 * indication) has been enabled by the client.
 * */
mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle, uint16_t srv_handle,
    uint16_t char_value_handle, uint8_t offset, uint8_t* p_value,
    uint8_t len, uint8_t type)
{
  if(!p_value){
    return MI_ERR_INVALID_ADDR;
  }
  if((type != 1) &&(type!=2)){
    
    return MI_ERR_INVALID_PARAM;
  }
  return bnUpdateCharacterValue(conn_handle,srv_handle,char_value_handle,offset,p_value,len,type);
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
        uint8_t status, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type)
{
   return  bnGattREAuthorReply( conn_handle,
         status,  char_value_handle,  offset,
          p_value, len, type);
}

/**
 *        GATT Client APIs
 */

/**
 * @brief   Discover primary service by service UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for primary sevice
 *discovery procedure
 *          [in] p_srv_uuid: pointer to service uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Primary
 *Service Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    The response is given through
 *MIBLE_GATTC_EVT_PRIMARY_SERVICE_DISCOVER_RESP event
 * */
mible_status_t mible_gattc_primary_service_discover_by_uuid(uint16_t conn_handle,
    mible_handle_range_t handle_range,
    mible_uuid_t* p_srv_uuid)
{
  if(!p_srv_uuid){
    return MI_ERR_INVALID_ADDR;
  }
  return bnPrimServDiscByUUID(conn_handle,handle_range,p_srv_uuid);
}

/**
 * @brief   Discover characteristic by characteristic UUID.
 * @param   [in] conn_handle: connect handle
 *          [in] handle_range: search range for characteristic discovery
 * procedure
 *          [in] p_char_uuid: pointer to characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the
 * Characteristic Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through
 * MIBLE_GATTC_CHR_DISCOVER_BY_UUID_RESP event
 * */
mible_status_t mible_gattc_char_discover_by_uuid(uint16_t conn_handle,
                                                 mible_handle_range_t handle_range,
                                                 mible_uuid_t* p_char_uuid)
{
    return bnCharacterDiscByUUID(conn_handle,handle_range,p_char_uuid);
}

/**
 * @brief   Discover characteristic client configuration descriptor
 * @param   [in] conn_handle: connection handle
 *          [in] handle_range: search range
 * @return  MI_SUCCESS             Successfully started Clien Config Descriptor
 * Discovery procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    Maybe run the charicteristic descriptor discover procedure firstly,
 * then pick up the client configuration descriptor which att type is 0x2092
 *          The response is given through MIBLE_GATTC_CCCD_DISCOVER_RESP
 * event
 *          Only return the first cccd handle within the specified
 * range.
 * */
mible_status_t mible_gattc_clt_cfg_descriptor_discover(uint16_t conn_handle, mible_handle_range_t handle_range)
{
  mible_uuid_t mUuid;
  mUuid.type = 0;//UUID 16
  mUuid.uuid16 = 0x2092;
  

  return bnCharacterDiscByUUID(conn_handle,handle_range,&mUuid);
}

/**
 * @brief   Read characteristic value by UUID
 * @param   [in] conn_handle: connnection handle
 *          [in] handle_range: search range
 *          [in] p_char_uuid: pointer to characteristic uuid
 * @return  MI_SUCCESS             Successfully started or resumed the Read
 * using Characteristic UUID procedure.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through
 * MIBLE_GATTC_EVT_READ_CHR_VALUE_BY_UUID_RESP event
 * */
mible_status_t mible_gattc_read_char_value_by_uuid(uint16_t conn_handle, mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid)
{
  if(!p_char_uuid){
    return MI_ERR_INVALID_ADDR;
  }
  
  return bnReadValueByUUID(conn_handle,handle_range,p_char_uuid);
}
/*
 * @brief	Write value by handle with response
 * @param 	[in] conn_handle: connection handle
 * 			[in] handle: handle to the attribute to be written.
 * 			[in] p_value: pointer to data
 * 			[in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write with response
 * procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE   Invaild connection handle.
 * @note    The response is given through MIBLE_GATTC_EVT_WRITE_RESP event
 *
 * */
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle, uint16_t handle,
    uint8_t* p_value, uint8_t len)
{
  if(!p_value){
    return MI_ERR_INVALID_ADDR;
  }

  if(!len){
    return MI_ERR_INVALID_LENGTH;
  }
  
  return bnGattWrite(1,conn_handle,handle,p_value,len);
}

/**
 * @brief   Write value by handle without response
 * @param   [in] conn_handle: connection handle
 *          [in] att_handle: handle to the attribute to be written.
 *          [in] p_value: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS             Successfully started the Write Cmd procedure.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE   Invalid Connection State.
 *          MI_ERR_INVALID_LENGTH   Invalid length supplied.
 *          MI_ERR_BUSY            Procedure already in progress.
 *          MIBLE_ERR_INVALID_CONN_HANDLE  Invaild connection handle.
 * @note    no response
 * */
 mible_status_t mible_gattc_write_cmd(uint16_t conn_handle,
        uint16_t att_handle, uint8_t* p_value, uint8_t len)
{
  if(!p_value){
    return MI_ERR_INVALID_ADDR;
  }
  
  if(!len){
    return MI_ERR_INVALID_LENGTH;
  }
  return bnGattWrite(0,conn_handle,att_handle,p_value,len);
  
}

/**
 * @brief   Create a timer.
 * @param   [out] p_timer_id: a pointer to timer id address which can uniquely identify the timer.
 *          [in] timeout_handler: a pointer to a function which can be
 * called when the timer expires.
 *          [in] mode: repeated or single shot.
 * @return  MI_SUCCESS             If the timer was successfully created.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   timer module has not been initialized or the
 * timer is running.
 *          MI_ERR_NO_MEM          timer pool is full.
 *
 * */
mible_status_t mible_timer_create(void** p_timer_id,
    mible_timer_handler timeout_handler, mible_timer_mode mode)
{
  appTimer_t *tinfo = appGetFreeTimer();
  if(!tinfo){
    return MI_ERR_NO_MEM;
  }
  tinfo->mode = mode;
  tinfo->timerHandle = timeout_handler;
  
  *p_timer_id = tinfo;
  
  return MI_SUCCESS;
}

/**
 * @brief   Delete a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
mible_status_t mible_timer_delete(void* timer_id) 
{ 
  appTimer_t *tinfo = timer_id;
  if((tinfo < (&appTimer[0]))||(tinfo > (&appTimer[APP_TIMERQUEUENUM - 1]))){
    return MI_ERR_INVALID_PARAM;
  }
  
  if(tinfo->run && timerRunning){
    timerRunning--;
  }
  
  memset(tinfo,0,sizeof(*tinfo));
  return MI_SUCCESS; 
}

/**
 * @brief   Start a timer.
 * @param   [in] timer_id: timer id
 *          [in] timeout_value: Number of milliseconds to time-out event
 * (minimum 10 ms).
 *          [in] p_context: parameters that can be passed to
 * timeout_handler
 *
 * @return  MI_SUCCESS             If the timer was successfully started.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *          MI_ERR_INVALID_STATE   If the application timer module has not been
 * initialized or the timer has not been created.
 *          MI_ERR_NO_MEM          If the timer operations queue was full.
 * @note    If the timer has already started, it will start counting again.
 * */
mible_status_t mible_timer_start(void* timer_id, uint32_t timeout_value,
    void* p_context)
{
  appTimer_t *tinfo = timer_id;
  if((tinfo < (&appTimer[0]))||(tinfo > (&appTimer[APP_TIMERQUEUENUM - 1]))||tinfo->run){
    return MI_ERR_INVALID_PARAM;
  }
  
  tinfo->interval = timeout_value;
  tinfo->arg = p_context;
  bnTimerStart(timer_id);
  tinfo->run = 1;
  timerRunning++;
  
  return MI_SUCCESS;
}

/**
 * @brief   Stop a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully stopped.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied.
 *
 * */
mible_status_t mible_timer_stop(void* timer_id) 
{ 
  appTimer_t *tinfo = timer_id;
  if((tinfo < (&appTimer[0]))||(tinfo > (&appTimer[APP_TIMERQUEUENUM - 1]))){
    return MI_ERR_INVALID_PARAM;
  }
  
  if(tinfo->run && timerRunning){
    timerRunning--;
  }
  
  tinfo->run = 0;
  return MI_SUCCESS; 
}
uint8_t check_record_security_flag(void)
{
    if(
    (0xaa!=FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+1))||
	(0xaa!=FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+3))||
	(0xaa!=FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+1))||
	(0xaa!=FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+3))
	)
	return 1;
	else
	return 0;	
}
void set_id_manage_security_flag(void)
{
 FLASH_ProgramWord(FLASH_RECORD_ID_MANAGE_ADDR,0xaaffaaff);


}
void set_data_security_flag(void)
{
	FLASH_ProgramWord(FLASH_RECORD_DATA_ADDR,0xaaffaaff);
}



void check_flash_record(void)
{


   uint16_t i,j;
   uint8_t check_data;
   for(i=0;i<2000;i++)
   	{
   	check_data = FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i);
	if(check_data != 0xff)
		{
		 printf("have id\r\n");
		 break;
		}
   	}
      for(j=0;j<2000;j++)
   	{
   	check_data = FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+j);
	if(check_data != 0xff)
		{
		 printf("have data\r\n");

		 break;

		}
   	}
   if(i>=2000&&j>=2000)
   	{
   	 printf("have no id&data\r\n");
	 set_id_manage_security_flag();
	 set_data_security_flag();
	 FLASH_ProgramWord(FLASH_RECORD_WRITE_OFFSET_ADDR,0xffff0000);
   	}
   else
   	{
   	     if(0xaa != FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+1)||0xaa != FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+3))
		 mible_task_post(record_id_manage_reload_from_backup,NULL);	
		 if(0xaa != FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+1)||0xaa != FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+3))
		 mible_task_post(record_data_reload_from_backup,NULL);	
   	}
  
}

void  record_write_offset_search(uint8_t isbackup)
{
 
     uint16_t i;
     uint16_t offset_flag = 0;
     uint8_t write_offset_lb = 0;
     uint8_t write_offset_hb = 0;
	   i=0;
if(isbackup)
{
	do{
		 write_offset_lb = FLASH_ReadByte((FLASH_RECORD_BACKUP_WRITE_OFFSET_ADDR+i));
		 write_offset_hb = FLASH_ReadByte((FLASH_RECORD_BACKUP_WRITE_OFFSET_ADDR+i+1));
		 offset_flag = (uint16_t)(write_offset_hb<<8) | (uint16_t)write_offset_lb ;
		 i = i+2;
	   }
	 while(offset_flag != 0xffff);
	 offset_flag = 0;
	 record_write_offset_location = i-2;
	 //printf("record_write_offset_location = 0x%x\r\n",record_write_offset_location);
	 i = i-4;
	 write_offset_lb = FLASH_ReadByte((FLASH_RECORD_BACKUP_WRITE_OFFSET_ADDR+i));
	 write_offset_hb = FLASH_ReadByte((FLASH_RECORD_BACKUP_WRITE_OFFSET_ADDR+i+1));
	 record_write_offset = (uint16_t)(write_offset_hb<<8) | (uint16_t)write_offset_lb ;
	 //printf("record_write_offset = 0x%x\r\n",record_write_offset);
	 

}
else
{
  do{
  	  write_offset_lb = FLASH_ReadByte((FLASH_RECORD_WRITE_OFFSET_ADDR+i));
      write_offset_hb = FLASH_ReadByte((FLASH_RECORD_WRITE_OFFSET_ADDR+i+1));
      offset_flag = (uint16_t)(write_offset_hb<<8) | (uint16_t)write_offset_lb ;
	  i = i+2;
  	}
  while(offset_flag != 0xffff);
  offset_flag = 0;
  record_write_offset_location = i-2;
  //printf("record_write_offset_location = 0x%x\r\n",record_write_offset_location);
  i = i-4;
  write_offset_lb = FLASH_ReadByte((FLASH_RECORD_WRITE_OFFSET_ADDR+i));
  write_offset_hb = FLASH_ReadByte((FLASH_RECORD_WRITE_OFFSET_ADDR+i+1));
  record_write_offset = (uint16_t)(write_offset_hb<<8) | (uint16_t)write_offset_lb ;
  //printf("record_write_offset = 0x%x\r\n",record_write_offset);
}
}
void  record_read_offset_search(uint16_t record_id,uint8_t isbackup)
{
  uint8_t read_id;
    static uint16_t i;
  uint16_t offset_flag = 0;
  uint8_t read_offset_lb = 0;
  uint8_t read_offset_hb = 0;
  if(isbackup)
 {
  	
  read_id = FLASH_ReadByte((FLASH_RECORD_BACKUP_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE));
  if(read_id!=(uint8_t)record_id)
  	{
  	     printf("id err\r\n");
		 return;
  	}

  	   i=0;
  do{
      read_offset_lb  = FLASH_ReadByte((FLASH_RECORD_BACKUP_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i));
      read_offset_hb  = FLASH_ReadByte((FLASH_RECORD_BACKUP_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i+1));
      offset_flag = (uint16_t)(read_offset_hb<<8) | (uint16_t)read_offset_lb ;
	  i = i+2;
  	}
  while(offset_flag != 0xffff);
  offset_flag = 0;
  record_read_offset_location = i-2;
  //printf("record_read_offset_location = 0x%x\r\n",record_read_offset_location);
  if(i>=4){
  i = i-4;
  read_offset_lb  = FLASH_ReadByte((FLASH_RECORD_BACKUP_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i));
  read_offset_hb  = FLASH_ReadByte((FLASH_RECORD_BACKUP_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i+1));
  record_read_offset = (uint16_t)(read_offset_hb<<8) | (uint16_t)read_offset_lb ;
  //printf("record_read_offset = 0x%x\r\n",record_read_offset);
  	}
  else{
   record_read_offset = 0xffff;
   printf("no record store for the id = 0x%x\r\n",record_read_offset);
  	}
  	
 }
  else
 {
  read_id = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE));
  if(read_id!=(uint8_t)record_id)
	  {
		   printf("id err\r\n");
		   return;
	  }

  	   i=0;
  do{
      read_offset_lb  = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i));
      read_offset_hb  = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i+1));
      offset_flag = (uint16_t)(read_offset_hb<<8) | (uint16_t)read_offset_lb ;
	  i = i+2;
  	}
  while(offset_flag != 0xffff);
  offset_flag = 0;
  record_read_offset_location = i-2;
  //printf("record_read_offset_location = 0x%x\r\n",record_read_offset_location);
  if(i>=4){
  i = i-4;
  read_offset_lb  = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i));
  read_offset_hb  = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+i+1));
  record_read_offset = (uint16_t)(read_offset_hb<<8) | (uint16_t)read_offset_lb ;
  //printf("record_read_offset = 0x%x\r\n",record_read_offset);
  	}
  else{
	  record_read_offset = 0xffff;
	  printf("no record store for the id = 0x%x\r\n",record_read_offset);

  	}
 }

}
/**
 *        NVM APIs
 */

/**
 * @brief   Create a record in flash 
 * @param   [in] record_id: identify a record in flash 
 *          [in] len: record length
 * @return  MI_SUCCESS              Create successfully.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_NO_MEM,          Not enough flash memory to be assigned 
 *              
 * */
 mible_status_t mible_record_create(uint16_t record_id, uint8_t len)
{

  if(record_id>9)
  	return MI_ERR_INVALID_LENGTH;
  uint8_t read_id;
  read_id = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE));
  if(read_id!=0xff)
  //	return MI_ERR_INVALID_ADDR;
    	if(len%4 != 0){
		len = (len/4 + 1)*4;
		}
		if(len>200)
		return MI_ERR_INVALID_ADDR;	
  uint32_t word = ((uint32_t)(len<<16|0xff000000))|((uint32_t)(record_id|0xff00));
  FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE),word);
  printf("create a record\r\n");
  printf("read_id = %x\r\n",record_id);
  return MI_SUCCESS;
}

/**
 * @brief   Delete a record in flash
 * @param   [in] record_id: identify a record in flash  
 * @return  MI_SUCCESS              Delete successfully. 
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 * */
 mible_status_t mible_record_delete(uint16_t record_id)
{
    	
    if(record_id>9)
  	return MI_ERR_INVALID_LENGTH;

	delete_id = record_id;
	mible_task_post(record_id_delete_and_reload,NULL);

    return MI_SUCCESS;
}

/**
 * @brief   Restore data to flash
 * @param   [in] record_id: identify an area in flash
 *          [out] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 *allowed size.
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 * */
 mible_status_t mible_record_read(uint16_t record_id, uint8_t* p_data,
        uint8_t len)
{

  if(record_id>9)
  	return MI_ERR_INVALID_LENGTH;
    uint8_t i;
    uint8_t read_id;
	uint8_t read_len;
	
	if(check_record_security_flag())
		{
		printf("check fail\r\n");
		return MI_ERR_INTERNAL;
		}
		
  read_id = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE));
  if(read_id!=(uint8_t)record_id)
  	return MI_ERR_INVALID_ADDR;
  read_len = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+2));
  if(len>read_len)
  	return MI_ERR_INVALID_ADDR;
  record_read_offset_search(record_id,0);
  if(record_read_offset == 0xffff)
  	{
  	printf("no record for the id\r\n");
  	return MI_ERR_NOT_FOUND;
  	}
  for(i=0;i<len;i++){
    p_data[i] = FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+4+i+record_read_offset);
  }
    return MI_SUCCESS;
}

/**
 * @brief   Store data to flash
 * @param   [in] record_id: identify an area in flash
 *          [in] p_data: pointer to data
 *          [in] len: data length
 * @return  MI_SUCCESS              The command was accepted.
 *          MI_ERR_INVALID_LENGTH   Size was 0, or higher than the maximum
 * allowed size.
 *          MI_ERR_INVALID_PARAMS   p_data is not aligned to a 4 byte boundary.
 * @note    Should use asynchronous mode to implement this function.
 *          The data to be written to flash has to be kept in memory until the
 * operation has terminated, i.e., an event is received.
 *          When record writing complete , call mible_arch_event_callback function and pass MIBLE_ARCH_EVT_RECORD_WRITE_CMP event and result. 
 * */
 mible_status_t mible_record_write(uint16_t record_id, uint8_t* p_data,
        uint8_t len)
{

  if(record_id>9)
  	return MI_ERR_INVALID_LENGTH;
  uint8_t read_id;
  uint8_t read_len;
  read_id = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE));
  if(read_id!=(uint8_t)record_id)
  	return MI_ERR_INVALID_ADDR;

  read_len = FLASH_ReadByte((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+2));
  if(len>read_len)
  	return MI_ERR_INVALID_ADDR;
  uint8_t len4bytes = len/4 + 1;
  uint8_t i;
  uint32_t word;
  uint8_t index;
  uint8_t copyLen = 4;
  record_write_offset_search(0);
  record_read_offset_search(record_id,0);
  	if((record_write_offset+len)>2000||record_write_offset_location>1000||record_read_offset_location>100)
  		{
  		    printf("write fail,no memery\r\n");
  			return MI_ERR_DATA_SIZE;
  		}
	
  	if(record_write_offset>1600){
	mible_task_post(record_data_reload,NULL);
	}
	else if(record_write_offset_location>800||record_read_offset_location>80){
	mible_task_post(record_id_manage_reload,NULL);	
	}
	
  for(i=0;i<len4bytes;i++){
    index = i*4;
    if(index + 4 > len){
      memset((uint8_t *)&word,0xff,4);
      copyLen = len - index;
    }
    memcpy((uint8_t *)&word,p_data+index,copyLen);
    FLASH_ProgramWord((FLASH_RECORD_DATA_ADDR+4+index+record_write_offset),word);
  }
	  	if(len%4 != 0){
		len = (len/4 + 1)*4;
		}
    record_read_offset = record_write_offset;
    record_write_offset = record_write_offset+len;
	
		if(record_write_offset_location%4 != 0){
		record_write_offset = (record_write_offset<<16)|0x0000ffff;
		//printf("record_write_offset = %x\r\n",record_write_offset);
		}
		else
		{
		record_write_offset = record_write_offset|0xffff0000;
		//printf("record_write_offset = %x\r\n",record_write_offset);
		}
		
	FLASH_ProgramWord((FLASH_RECORD_WRITE_OFFSET_ADDR+record_write_offset_location),record_write_offset);
       
		if(record_read_offset_location%4 != 0){
		record_read_offset = (record_read_offset<<16)|0x0000ffff;
		//printf("record_read_offset = %x\r\n",record_read_offset);
		}
		else
		{
		record_read_offset = record_read_offset|0xffff0000;
		//printf("record_read_offset = %x\r\n",record_read_offset);
		}
		
	FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+record_id*FLASH_RECORD_ID_MANAGE_SIZE+4+record_read_offset_location),record_read_offset);
	
  record_write_offset = 0;
  record_read_offset = 0;
  record_read_offset_location = 0;
  record_write_offset_location = 0;
  
    mible_arch_evt_param_t param;
    param.record.id = record_id;
    param.record.status = MI_SUCCESS;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE,&param);
    return MI_SUCCESS;
}

void record_data_reload(void *arg){

  printf("data reload\r\n");
  if(record_data_backup())
  	{
  	 printf("data backup fail \r\n");
	 return;
  	}
  FLASH_ErasePage(65);
  int i,j;
  uint8_t read_id;
  uint8_t len;

  uint32_t write_offset_new = 0;
  uint8_t lenfourbyte;
  uint32_t word;
  for(i=0;i<10;i++)
  	{
  	read_id = FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE);
	if(read_id == i)
	{
	  len = FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE+2);
	  uint8_t record_data_temp[len];
	  record_read_offset_search(read_id,0);
	if(record_read_offset!=0xffff)
	{
      for(j=0;j<len;j++)
	  {
       record_data_temp[j] = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+4+j+record_read_offset);
	   //printf("record_data_temp[%d]=%d\r\n",j,record_data_temp[j]);
       }

	  lenfourbyte = len/4;
      for(j=0;j<lenfourbyte;j++)
	  	 {
         memcpy((uint8_t *)&word,(record_data_temp+j*4),4);
		 printf("word = %x\r\n",word);
         FLASH_ProgramWord((FLASH_RECORD_DATA_ADDR+4+j*4+write_offset_new),word);
         } 
	  	  //store the read_offset == write_offset_new
	  	  record_read_offset = write_offset_new;
	  	  if(record_read_offset_location%4 != 0){
		record_read_offset = (record_read_offset<<16)|0x0000ffff;
		}
		else
		{
		record_read_offset = record_read_offset|0xffff0000;
		}
		
	FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+read_id*FLASH_RECORD_ID_MANAGE_SIZE+4+record_read_offset_location),record_read_offset);
	  write_offset_new = write_offset_new + len;
	  printf("write_offset_new = %d\r\n",write_offset_new);
  	}
   }
  }
  record_write_offset_search(0);
  record_write_offset = write_offset_new;
  		if(record_write_offset_location%4 != 0){
		record_write_offset = (record_write_offset<<16)|0x0000ffff;
		//printf("record_write_offset = %x\r\n",record_write_offset);
		}
		else
		{
		record_write_offset = record_write_offset|0xffff0000;
		//printf("record_write_offset = %x\r\n",record_write_offset);
		}
  FLASH_ProgramWord(FLASH_RECORD_WRITE_OFFSET_ADDR+record_write_offset_location,record_write_offset);
   set_data_security_flag();	

  record_write_offset = 0;
  record_read_offset = 0;
  record_read_offset_location = 0;
  record_write_offset_location = 0;
 
}

void record_data_reload_from_backup(void *arg){
		printf("data reload from the backup\r\n");

	  FLASH_ErasePage(65);
	  int i,j;
	  uint8_t read_id;
	  uint8_t len;
	
	  uint32_t write_offset_new = 0;
	  uint8_t lenfourbyte;
	  uint32_t word;
	  for(i=0;i<10;i++)
		{
		read_id = FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE);
		if(read_id == i)
		{
		  len = FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE+2);
		  uint8_t record_data_temp[len];
		  record_read_offset_search(read_id,0);
		 if(record_read_offset!=0xffff)
		 {
		  for(j=0;j<len;j++)
		  {
		   record_data_temp[j] = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+4+j+record_read_offset);
		   //printf("record_data_temp[%d]=%d\r\n",j,record_data_temp[j]);
		   }
	
		  lenfourbyte = len/4;
		  for(j=0;j<lenfourbyte;j++)
			 {
			 memcpy((uint8_t *)&word,(record_data_temp+j*4),4);
			 //printf("word = %x\r\n",word);
			 FLASH_ProgramWord((FLASH_RECORD_DATA_ADDR+4+j*4+write_offset_new),word);
			 } 
			  //store the read_offset == write_offset_new
			  record_read_offset = write_offset_new;
			  if(record_read_offset_location%4 != 0){
			record_read_offset = (record_read_offset<<16)|0x0000ffff;
			}
			else
			{
			record_read_offset = record_read_offset|0xffff0000;
			}
			
		FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+read_id*FLASH_RECORD_ID_MANAGE_SIZE+4+record_read_offset_location),record_read_offset);
		  write_offset_new = write_offset_new + len;
		  printf("write_offset_new = %d\r\n",write_offset_new);
		}
	   }
	  }
	  record_write_offset_search(0);
	  record_write_offset = write_offset_new;
			if(record_write_offset_location%4 != 0){
			record_write_offset = (record_write_offset<<16)|0x0000ffff;
			//printf("record_write_offset = %x\r\n",record_write_offset);
			}
			else
			{
			//record_write_offset = record_write_offset|0xffff0000;
			printf("record_write_offset = %x\r\n",record_write_offset);
			}
	  FLASH_ProgramWord(FLASH_RECORD_WRITE_OFFSET_ADDR+record_write_offset_location,record_write_offset);
	  set_data_security_flag();
	  record_write_offset = 0;
	  record_read_offset = 0;
	  record_read_offset_location = 0;
	  record_write_offset_location = 0;
	
 
}

void record_id_delete_and_reload(void *arg)
{
  printf("delete and reload\r\n");
  if(record_id_manage_backup())
  	{
  	printf("delete backup fail \r\n");
	return;
  	}
  FLASH_ErasePage(64);
  int i;
  uint8_t read_id;
  uint8_t len;
  for(i=0;i<10;i++)
  	{
  	read_id = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE);
	if(read_id == i){
		if(read_id != delete_id)
		{
		//printf("read_id = %d\r\n",read_id);
	  len = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+2+i*FLASH_RECORD_ID_MANAGE_SIZE);
	  uint32_t word = ((uint32_t)(len<<16|0xff000000))|((uint32_t)(read_id|0xff00));
	  FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE),word);
	  record_read_offset_search(read_id,1);
	    if(record_read_offset!=0xffff)
	    {
	     record_read_offset = record_read_offset|0xffff0000;
	     FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+4+i*FLASH_RECORD_ID_MANAGE_SIZE),record_read_offset);
	 	}
		 }
  	   }
     }
  record_write_offset_search(1);
  record_write_offset = record_write_offset|0xffff0000;
  FLASH_ProgramWord(FLASH_RECORD_WRITE_OFFSET_ADDR,record_write_offset);
  set_id_manage_security_flag();
  record_write_offset = 0;
  record_read_offset = 0;
  record_read_offset_location = 0;
  record_write_offset_location = 0;

  mible_arch_evt_param_t param;
  param.record.id = delete_id;
  param.record.status = MI_SUCCESS;
  mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE,&param);

}

void record_id_manage_reload(void *arg)
{

  printf("id reload\r\n");
  if(record_id_manage_backup())
  	{
  	printf("id backup fail \r\n");
	return;
  	}
  FLASH_ErasePage(64);
  int i;
  uint8_t read_id;
  uint8_t len;
  for(i=0;i<10;i++)
  	{
  	read_id = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE);
	if(read_id == i){
		//printf("read_id = %d\r\n",read_id);
	  len = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+2+i*FLASH_RECORD_ID_MANAGE_SIZE);
	  uint32_t word = ((uint32_t)(len<<16|0xff000000))|((uint32_t)(read_id|0xff00));
	  FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE),word);
	  record_read_offset_search(read_id,1);
	  if(record_read_offset!=0xffff)
	  	{
	     record_read_offset = record_read_offset|0xffff0000;
	     FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+4+i*FLASH_RECORD_ID_MANAGE_SIZE),record_read_offset);
	  	}
  	   }
     }
  	record_write_offset_search(1);
    record_write_offset = record_write_offset|0xffff0000;
	FLASH_ProgramWord(FLASH_RECORD_WRITE_OFFSET_ADDR,record_write_offset);
	set_id_manage_security_flag();
  record_write_offset = 0;
  record_read_offset = 0;
  record_read_offset_location = 0;
  record_write_offset_location = 0;
 
}


void record_id_manage_reload_from_backup(void *arg)
{
  printf("id reload from backup\r\n");
  FLASH_ErasePage(64);
  int i;
  uint8_t read_id;
  uint8_t len;
  for(i=0;i<10;i++)
  	{
  	read_id = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE);
	if(read_id == i){
		//printf("read_id = %d\r\n",read_id);
	  len = FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+2+i*FLASH_RECORD_ID_MANAGE_SIZE);
	  uint32_t word = ((uint32_t)(len<<16|0xff000000))|((uint32_t)(read_id|0xff00));
	  FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+i*FLASH_RECORD_ID_MANAGE_SIZE),word);
	  record_read_offset_search(read_id,1);
	  if(record_read_offset!=0xffff)
	  	{
	     record_read_offset = record_read_offset|0xffff0000;
	     FLASH_ProgramWord((FLASH_RECORD_ID_MANAGE_ADDR+4+i*FLASH_RECORD_ID_MANAGE_SIZE),record_read_offset);
	  	}
  	   }
     }
  	record_write_offset_search(1);
    record_write_offset = record_write_offset|0xffff0000;
	FLASH_ProgramWord(FLASH_RECORD_WRITE_OFFSET_ADDR,record_write_offset);
	set_id_manage_security_flag();
  record_write_offset = 0;
  record_read_offset = 0;
  record_read_offset_location = 0;
  record_write_offset_location = 0;
 
}

uint8_t record_data_backup (void)
{
  
  uint32_t backup_data,check_data;
  uint8_t a,b,c,d;
  uint16_t i;
  FLASH_ErasePage(66);

    for(i=0;i<2000;i=i+4){
        a =FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+i);
		b =FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+i+1);
	    c =FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+i+2);
		d =FLASH_ReadByte(FLASH_RECORD_DATA_ADDR+i+3);
		backup_data = (uint32_t)a|(uint32_t)b<<8|(uint32_t)c<<16|(uint32_t)d<<24;
		//printf("backup_data[%d]=%x\r\n",i,backup_data);
		FLASH_ProgramWord((FLASH_RECORD_BACKUP_ADDR+i),backup_data);
	    a =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i);
		b =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i+1);
	    c =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i+2);
		d =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i+3);
		check_data = (uint32_t)a|(uint32_t)b<<8|(uint32_t)c<<16|(uint32_t)d<<24;
		if(backup_data!=check_data)
			{	
					 return 1;
			}
    	}
    return 0;

}

uint8_t record_id_manage_backup(void)
{
  uint32_t backup_data,check_data;
  uint8_t a,b,c,d;
  uint16_t i;
  FLASH_ErasePage(66);

    for(i=0;i<2000;i=i+4){
        a =FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i);
		b =FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i+1);
	    c =FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i+2);
		d =FLASH_ReadByte(FLASH_RECORD_ID_MANAGE_ADDR+i+3);
		backup_data = (uint32_t)a|(uint32_t)b<<8|(uint32_t)c<<16|(uint32_t)d<<24;
		//printf("backup_data[%d]=%x\r\n",i,backup_data);
		FLASH_ProgramWord((FLASH_RECORD_BACKUP_ADDR+i),backup_data);
	    a =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i);
		b =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i+1);
	    c =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i+2);
		d =FLASH_ReadByte(FLASH_RECORD_BACKUP_ADDR+i+3);
		check_data = (uint32_t)a|(uint32_t)b<<8|(uint32_t)c<<16|(uint32_t)d<<24;
		if(backup_data!=check_data)
			{
					 return 1;
			}
    	}
    return 0;
	//printf("backup_ok\r\n");
}

void mible_timer_exec(void ){
  if(timerRunning){
    uint8_t i = 0;
    do{
        if(appTimer[i].run && (bnTimerExpired((void *)&appTimer[i]))){
          appTimer[i].timerHandle(appTimer[i].arg);
          if(appTimer[i].mode == MIBLE_TIMER_REPEATED){
            bnTimerStart((void *)&appTimer[i]);
          }else{
            appTimer[i].run = 0;
            if(timerRunning){
              timerRunning--;
            }
          }
          break;
      }
      i++;
    }while(i<APP_TIMERQUEUENUM);
  }
}
/**
 *        MISC APIs
 */
/*
 * @brief 	Get ture random bytes .
 * @param 	[out] p_buf: pointer to data
 * 			[in] len: Number of bytes to take from pool and place in
 * p_buff
 * @return  MI_SUCCESS          The requested bytes were written to
 * p_buff
 *          MI_ERR_NO_MEM       No bytes were written to the buffer, because
 * there were not enough random bytes available.
 * @note    SHOULD use TRUE random num generator
 * */
mible_status_t mible_rand_num_generator(uint8_t* p_buf, uint8_t len)
{
  if(!p_buf || !len){
    return MI_ERR_NO_MEM;
  }
  while(len){
    *(p_buf++ )= (unsigned char )bnRand();
    len--;
  }
  return MI_SUCCESS;
}

/**
 * @brief   Encrypts a block according to the specified parameters. 128-bit
 * AES encryption. (zero padding)
 * @param   [in] key: encryption key
 *          [in] plaintext: pointer to plain text
 *          [in] plen: plain text length
 *          [out] ciphertext: pointer to cipher text
 * @return  MI_SUCCESS              The encryption operation completed.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE    Encryption module is not initialized.
 *          MI_ERR_INVALID_LENGTH   Length bigger than 16.
 *          MI_ERR_BUSY             Encryption module already in progress.
 * @note    SHOULD use synchronous mode to implement this function
 * */
  mible_status_t mible_aes128_encrypt(const uint8_t* key,
        const uint8_t* plaintext, uint8_t plen, uint8_t* ciphertext){
  if(!plaintext || !key)
    return MI_ERR_INVALID_ADDR;
  else if(plen > 16 || !plen)
    return MI_ERR_INVALID_LENGTH;
  
  unsigned char key16[16] = {0};
  memcpy(key16,key,16);
  
  unsigned char text[16] = {0};
  memcpy(text,plaintext,plen);
  
  if(!bnAesBlock(key16,text,ciphertext)){
    return MI_SUCCESS;
  	}
  return MI_ERR_INTERNAL;
}

/**
 * @brief   Post a task to a task quene, which can be executed in a right place 
 * (maybe a task in RTOS or while(1) in the main function).
 * @param   [in] handler: a pointer to function 
 *          [in] param: function parameters 
 * @return  MI_SUCCESS              Successfully put the handler to quene.
 *          MI_ERR_NO_MEM           The task quene is full. 
 *          MI_ERR_INVALID_PARAM    Handler is NULL
 * */
mible_status_t mible_task_post(mible_handler_t handler, void *arg){

  if(!handler){
    return MI_ERR_INVALID_PARAM;
  }
  if(taskQueueFull()){
    return MI_ERR_NO_MEM;
  }
  appTask_t *appt = &appTask[wrPtrTask++ & (APP_TASKQUEUENUM - 1)];
  
  appt->taskHandle = handler;
  appt->arg = arg;
  return MI_SUCCESS;
}
void mible_tasks_exec(void){
  if(rePtrTask != wrPtrTask){
    appTask_t *appt = &appTask[rePtrTask++ & (APP_TASKQUEUENUM - 1)];
    if(appt->taskHandle){
      appt->taskHandle(appt->arg);
    }
  }
}


void mible_loop(void ){
  mible_timer_exec();
  mible_tasks_exec();
}




/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_gatt.c
* @brief     xiaomi ble gatt api
* @details   Gatt data types and functions.
* @author    hector_huang
* @date      2018-12-26
* @version   v1.0
* *********************************************************************************************************
*/
#include <string.h>
#include "gap.h"
#include "gatt.h"
#include "gatt_uuid.h"
#include "gattservapp.h"
#include "mible_api.h"
#include "mible_port.h"
#include "mible_type.h"
#define MI_LOG_MODULE_NAME "PPLUS_GATT"
#include "mible_log.h"
#include "platform_types.h"

#define MAX_GATT_DB_NUM                      4 //To support multi services
#define GATT_TABLE_VALUE_ALLOC               1



typedef struct
{
    bool used;
    uint16_t*     char_value_lens;
    gattService_t service;
} service_db_t;

static service_db_t srv_dbs[MAX_GATT_DB_NUM];

static bool is_fixed_uuid(uint16_t uuid)
{
    if ((GATT_PRIMARY_SERVICE_UUID == uuid) ||
        (GATT_SECONDARY_SERVICE_UUID == uuid) ||
        (GATT_INCLUDE_UUID == uuid) ||
        (GATT_CHARACTER_UUID == uuid) ||
        (GATT_CHAR_EXT_PROPS_UUID == uuid) ||
        (GATT_CHAR_USER_DESC_UUID == uuid) ||
        (GATT_CLIENT_CHAR_CFG_UUID == uuid) ||
        (GATT_SERV_CHAR_CFG_UUID == uuid) ||
        (GATT_CHAR_FORMAT_UUID == uuid) ||
        (GATT_CHAR_AGG_FORMAT_UUID == uuid) ||
        (GATT_VALID_RANGE_UUID == uuid) ||
        (GATT_EXT_REPORT_REF_UUID == uuid) ||
        (GATT_REPORT_REF_UUID == uuid) ||
        (GATT_CHAR_DESCRIPTOR_NUM_OF_DIGITALS_UUID == uuid) ||
        (GATT_CHAR_DESCRIPTOR_VALUE_TRIGGER_SETTING_UUID == uuid) ||
        (GATT_CHAR_SENSING_CONFIGURATION_UUID == uuid) ||
        (GATT_CHAR_SENSING_MEASUREMENT_UUID == uuid) ||
        (GATT_CHAR_SENSING_TRIGGER_SETTING_UUID == uuid) ||
        (GATT_CHAR_DESCRIPTOR_TIME_TRIGGER_SETTING_UUID == uuid))
    {
        return TRUE;
    }

    return FALSE;
}

static service_db_t *find_srv_db(uint8_t attr_handle)
{
    for (uint8_t i = 0; i < MAX_GATT_DB_NUM; ++i)
    {
        if (srv_dbs[i].used && 
                  ((srv_dbs[i].service.attrs[0].handle <= attr_handle) &&
                  (((srv_dbs[i].service.attrs[0].handle + srv_dbs[i].service.numAttrs) > attr_handle))))
        {
            return &srv_dbs[i];
        }
    }
    return NULL;
}



/*********************************************************************
    @fn          mi_gatt_read_cb

    @brief       attribute read callback.

    @param       connHandle - connection message was received on
    @param       pAttr - pointer to attribute
    @param       pValue - pointer to data to be read
    @param       pLen - length of data to be read
    @param       offset - offset of the first octet to be read
    @param       maxLen - maximum length of data to be read
    @param       method - type of read message

    @return      SUCCESS, blePending or Failure
*/
static uint8 mi_gatt_read_cb( uint16 connHandle, gattAttribute_t* pAttr,
                             uint8* pValue, uint16* pLen, uint16 offset, uint8 maxLen )
{
    bStatus_t status = SUCCESS;

    mible_gatts_evt_t event;
    if (pAttr->permissions & GATT_PERMIT_AUTHOR_READ)
    {
        event = MIBLE_GATTS_EVT_READ_PERMIT_REQ;
        mible_gatts_evt_param_t param;
        memset(&param, 0, sizeof(param));
        param.conn_handle = connHandle;
        param.read.value_handle = pAttr->handle;
        mible_gatts_event_callback(event, &param);
    }


    return ( status );
}

/*********************************************************************
    @fn      mi_gatt_write_cb

    @brief   Validate attribute data prior to a write operation

    @param   connHandle - connection message was received on
    @param   pAttr - pointer to attribute
    @param   pValue - pointer to data to be written
    @param   len - length of data
    @param   offset - offset of the first octet to be written

    @return  Success or Failure
*/
static bStatus_t mi_gatt_write_cb( uint16 connHandle, gattAttribute_t* pAttr,
                                  uint8* pValue, uint16 len, uint16 offset )
{
  bStatus_t status = SUCCESS;
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
  mible_gatts_evt_t event = MIBLE_GATTS_EVT_BASE;
  if((pAttr->type.len == 2) && 
      (uuid == GATT_CLIENT_CHAR_CFG_UUID || uuid == GATT_SERV_CHAR_CFG_UUID ))
  {
    LOG("CCCD or SCCD\n");

  }
  else
  {

    mible_gatts_evt_param_t param;
    event = MIBLE_GATTS_EVT_WRITE;
    if(pAttr->permissions & GATT_PERMIT_AUTHOR_WRITE)
      event = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
    memset(&param, 0, sizeof(param));
    param.conn_handle = connHandle;
    param.write.value_handle = pAttr->handle;
    param.write.offset = 0;
    param.write.data = pValue;
    param.write.len = len;
    mible_gatts_event_callback(event, &param);
  }
  return ( status );
}

uint8_t mi_gatt_authorize_attr_cb( uint16_t conn_id, gattAttribute_t* p_attr, uint8 opcode )
{
  return SUCCESS;

}


const gattServiceCBs_t mi_gatt_server_cbs = 
{
  mi_gatt_read_cb,       //!< Read callback function pointer
  mi_gatt_write_cb,     //!< Write callback function pointer
  mi_gatt_authorize_attr_cb //!< Authorization callback function pointer
};






uint32_t add_value_to_attr_db(gattAttribute_t* p_attr, uint32_t len, uint8_t* value, uint8_t* p_value_db)
{
  if(((uint32_t)p_value_db) & 0xffff0000){
    *((uint8_t**)&(p_attr->pValue)) = p_value_db;
    memset(p_value_db, 0, len);
    if(value)
      memcpy(p_attr->pValue, value, len);
  }
  return ((len + 3)&0xfffffffc); /*word aligned*/
}

uint32_t add_uuid_to_attr_db(gattAttribute_t* p_attr, mible_uuid_t* p_uuid_src, uint8_t* p_uuid_db)
{
  uint32_t len = (p_uuid_src->type == 0) ? 2 :16;
  if(((uint32_t)p_uuid_db) & 0xffff0000){
    p_attr->type.uuid = p_uuid_db;
    if(p_uuid_src->type == 0)//case 16bit uuid
    {
      p_attr->type.len = 2;
      p_uuid_db[0] = LO_UINT16(p_uuid_src->uuid16);
      p_uuid_db[1] = HI_UINT16(p_uuid_src->uuid16);
    }
    else if(p_uuid_src->type == 1)//case 128bit uuid
    {
      p_attr->type.len = 16;
      memcpy(p_uuid_db, p_uuid_src->uuid128, 16);
    }
  }
  return (len+3)&0xfffffffc;/*word aligned*/
}

uint32_t add_uuid_to_attr_type(gattAttrType_t* p_attr_type, mible_uuid_t* p_uuid_src, uint8_t* p_uuid_db)
{
  uint32_t len = (p_uuid_src->type == 0) ? 2 :16;

  if(((uint32_t)p_uuid_db) & 0xffff0000){
    p_attr_type->uuid = p_uuid_db;
    if(p_uuid_src->type == 0)//case 16bit uuid
    {
      p_attr_type->len = 2;
      p_uuid_db[0] = LO_UINT16(p_uuid_src->uuid16);
      p_uuid_db[1] = HI_UINT16(p_uuid_src->uuid16);
    }
    else if(p_uuid_src->type == 1)//case 128bit uuid
    {
      p_attr_type->len = 16;
      memcpy(p_uuid_db, p_uuid_src->uuid128, 16);
    }
  }
  return (len+3)&0xfffffffc;/*word aligned*/
}


static uint32_t calculate_service_db(mible_gatts_srv_db_t *pservice, 
                            service_db_t* pservice_db, 
                            uint32_t* vlen_offset, 
                            uint32_t* uuid_offset, 
                            uint32_t* value_offset)
{
    uint32_t size = 0, attr_db_size = 0;
    mible_gatts_char_db_t *pcharacter = NULL;
    mible_gatts_char_desc_db_t *pdescriptor = NULL;
    gattAttribute_t* p_current_attr = NULL;
    uint8_t* p_uuid = NULL;
    uint8_t* p_value = NULL;
    gattCharCfg_t CCD = {0,0};

    if(pservice_db){
      uint8_t* phead = (uint8_t*)pservice_db->service.attrs;
      p_current_attr = pservice_db->service.attrs;
      pservice_db->char_value_lens = (uint16_t*)( phead + *vlen_offset);
      p_uuid = phead + *uuid_offset;
      p_value = phead + *value_offset; 
    }

    /* increase primary or secondary service */
    size ++;
    {
      mible_uuid_t srv_uuid = {
        .type = 0,
        .uuid16 = 0,
      };
      gattAttrType_t service_uuid;
      srv_uuid.uuid16 = (pservice->srv_type == MIBLE_PRIMARY_SERVICE) ? 
                              GATT_PRIMARY_SERVICE_UUID : GATT_SECONDARY_SERVICE_UUID;

      p_uuid += add_uuid_to_attr_db(p_current_attr, &(srv_uuid), p_uuid);
      p_uuid += add_uuid_to_attr_type(&service_uuid, &(pservice->srv_uuid), p_uuid);
      p_value += add_value_to_attr_db(p_current_attr, sizeof(gattAttrType_t), (uint8_t*)(&service_uuid), p_value);
      if(p_current_attr){
        p_current_attr->permissions = GATT_PERMIT_READ;
        p_current_attr ++;
      }
    }

    /* increase characteristics */
    for (uint8_t char_idx = 0; char_idx < pservice->char_num; ++char_idx)
    {
        pcharacter = pservice->p_char_db + char_idx;
        uint8_t char_value = 0;
        /* increase characterstic */
        size ++;
        {
          mible_uuid_t char_uuid = {
            .type = 0,
            .uuid16 = GATT_CHARACTER_UUID,
          };
          p_uuid += add_uuid_to_attr_db(p_current_attr, &char_uuid, p_uuid);
          p_value += add_value_to_attr_db(p_current_attr, 1, &(pcharacter->char_property), p_value);
          if(p_current_attr){
            p_current_attr->permissions = GATT_PERMIT_READ;
            p_current_attr ++;
          }
        }
        /* increase characterstic value */
        size ++;
        {
          p_uuid += add_uuid_to_attr_db(p_current_attr, &(pcharacter->char_uuid), p_uuid);
          p_value += add_value_to_attr_db(p_current_attr, pcharacter->char_value_len , &char_value, p_value);
          if(p_current_attr){
            if (pcharacter->rd_author)
            {
                p_current_attr->permissions |= GATT_PERMIT_AUTHOR_READ;
            }
            else
            {
                p_current_attr->permissions |= GATT_PERMIT_READ;
            }

            if (pcharacter->wr_author)
            {
                p_current_attr->permissions |= GATT_PERMIT_AUTHOR_WRITE;
            }
            else
            {
                p_current_attr->permissions |= GATT_PERMIT_WRITE;
            }
            p_current_attr ++;
          }
        }

        /* increase characteristic descriptor */
        if (0 != (pcharacter->char_property & MIBLE_NOTIFY))
        {
            /* increase cccd */
            size ++;
            {
              mible_uuid_t char_uuid = {
                .type = 0,
                .uuid16 = GATT_CLIENT_CHAR_CFG_UUID,
              };
              p_uuid += add_uuid_to_attr_db(p_current_attr, &char_uuid, p_uuid);
              p_value += add_value_to_attr_db(p_current_attr, sizeof(CCD), (uint8_t*)(&CCD), p_value);
              if(p_current_attr){
                p_current_attr->permissions = (GATT_PERMIT_READ | GATT_PERMIT_WRITE);
                p_current_attr ++;
              }
            }
        }

        if (0 != (pcharacter->char_property & MIBLE_BROADCAST))
        {
            /* increase sccd */
            size ++;
            {
              mible_uuid_t char_uuid = {
                .type = 0,
                .uuid16 = GATT_SERV_CHAR_CFG_UUID,
              };
              p_uuid += add_uuid_to_attr_db(p_current_attr, &char_uuid, p_uuid);
              p_value += add_value_to_attr_db(p_current_attr, 1, (uint8_t*)(&CCD), p_value);
              if(p_current_attr){
                p_current_attr->permissions = (GATT_PERMIT_READ | GATT_PERMIT_WRITE);
                p_current_attr ++;
              }
            }
        }

        pdescriptor = &(pcharacter->char_desc_db);
        if (NULL != pdescriptor->extend_prop)
        {
            /* increase characteristic extended properties */
            size ++;
            {
              mible_uuid_t char_uuid = {
                .type = 0,
                .uuid16 = GATT_CHAR_EXT_PROPS_UUID,
              };
              p_uuid += add_uuid_to_attr_db(p_current_attr, &char_uuid, p_uuid);
              p_value += add_value_to_attr_db(p_current_attr, sizeof(mible_gatts_char_desc_ext_prop_t),
                                                  (uint8_t*)(pdescriptor->extend_prop), p_value);
              if(p_current_attr){
                p_current_attr->permissions = GATT_PERMIT_READ;
                p_current_attr ++;
              }
            }
        }

        if (NULL != pdescriptor->char_format)
        {
            /* increase characteristic format */
            size ++;
            {
              mible_uuid_t char_uuid = {
                .type = 0,
                .uuid16 = GATT_CHAR_FORMAT_UUID,
              };
              gattCharFormat_t char_fmt;
              char_fmt.format    = pdescriptor->char_format->format;
              char_fmt.exponent  = pdescriptor->char_format->exponent;
              char_fmt.unit      = pdescriptor->char_format->unit;
              char_fmt.nameSpace = pdescriptor->char_format->name_space;
              char_fmt.desc      = pdescriptor->char_format->desc;
              p_uuid += add_uuid_to_attr_db(p_current_attr, &char_uuid, p_uuid);
              p_value += add_value_to_attr_db(p_current_attr, sizeof(char_fmt),
                                                 (uint8_t*)(&char_fmt), p_value);
              if(p_current_attr){
                p_current_attr->permissions = GATT_PERMIT_READ;
                p_current_attr ++;
              }
            }
        }

        if (NULL != pdescriptor->user_desc)
        {
            /* increase characteristic user description */
            size ++;
            {
              mible_uuid_t char_uuid = {
                .type = 0,
                .uuid16 = GATT_CHAR_USER_DESC_UUID,
              };
              p_uuid += add_uuid_to_attr_db(p_current_attr, &char_uuid, p_uuid);
              p_value += add_value_to_attr_db(p_current_attr, pdescriptor->user_desc->len,
                                      (uint8_t*)(pdescriptor->user_desc->string), p_value);
              if(p_current_attr){
                p_current_attr->permissions = GATT_PERMIT_READ|GATT_PERMIT_WRITE;
                p_current_attr ++;
              }
            }
        }
    }

    MI_LOG_DEBUG("service table count = %d", size);
    if(pservice_db == NULL){
      attr_db_size = size * sizeof(gattAttribute_t) + size * 2 + (uint32_t)p_uuid + (uint32_t)p_value;
      *vlen_offset = size * sizeof(gattAttribute_t);
      *uuid_offset = size * sizeof(gattAttribute_t) + size * 2;
      *value_offset = size * sizeof(gattAttribute_t) + size * 2 + (uint32_t)p_uuid;
      return attr_db_size;
    }
    return size;
}


static service_db_t *request_service_db(void)
{
    for (uint8_t i = 0; i < MAX_GATT_DB_NUM; ++i)
    {
        if (!srv_dbs[i].used)
        {
            return &srv_dbs[i];
        }
    }

    return NULL;
}

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
    mible_gatts_srv_db_t *pservice = NULL;
    mible_gatts_char_db_t *pcharacter = NULL;
    mible_gatts_char_desc_db_t *pdescriptor = NULL;
    uint32_t service_table_size = 0;
    service_db_t *psrv_db = NULL;
    /* validation database space */
    uint8_t unused_count = 0;
    for (uint8_t i = 0; i < MAX_GATT_DB_NUM; ++i)
    {
        if (!srv_dbs[i].used)
        {
            unused_count ++;
        }
    }
    if (unused_count < p_server_db->srv_num)
    {
        return MI_ERR_NO_MEM;
    }

    /* add service databases */
    for (uint8_t srv_idx = 0; srv_idx < p_server_db->srv_num; ++srv_idx)
    {
        uint32_t vlen_offset = 0, uuid_offset = 0, value_offset = 0;
        pservice = p_server_db->p_srv_db + srv_idx;

        /* find unused space */
        psrv_db = request_service_db();
        if (NULL == psrv_db)
        {
            return MI_ERR_NO_MEM;
        }

        /* allocate service table */
        service_table_size = calculate_service_db(pservice, NULL, 
                                                  &vlen_offset,  &uuid_offset, &uuid_offset);
        psrv_db->service.attrs = (gattAttribute_t *)osal_bm_alloc(service_table_size);
        if (NULL == psrv_db->service.attrs)
        {
            return MI_ERR_NO_MEM;
        }
        /* zero service table */
        memset(psrv_db->service.attrs, 0, service_table_size);
        psrv_db->used = TRUE;

        psrv_db->service.numAttrs = calculate_service_db(pservice,
                                                         psrv_db,
                                                         &vlen_offset,
                                                         &uuid_offset,
                                                         &uuid_offset);
        
        {
          uint8_t status = GATT_RegisterService( &(psrv_db->service));
          if ( status == SUCCESS )
          {
            // Register the service CBs with GATT Server Application
            status = gattServApp_RegisterServiceCBs( GATT_SERVICE_HANDLE(psrv_db->service.attrs),
                                                       mi_gatt_server_cbs );
          }
          else
          {
            return (err_code_convert(status));
          }
          
        }
    }

    mible_arch_evt_param_t param;
    memset(&param, 0, sizeof(param));
    param.srv_init_cmp.status = MI_SUCCESS;
    param.srv_init_cmp.p_gatts_db = p_server_db;
    mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);

    //dump_attribute();

    return MI_SUCCESS;
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
                                     uint16_t value_handle, uint8_t offset, uint8_t *p_value, uint8_t len)
{
    if (NULL == p_value)
    {
        MI_LOG_ERROR("gatt server set failed: invalue address");
        return MI_ERR_INVALID_ADDR;
    }

    service_db_t *psrv_db = find_srv_db(value_handle);
    if (NULL == psrv_db)
    {
        return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
    }

    uint8_t attr_idx = value_handle - psrv_db->service.attrs[0].handle;
    gattAttribute_t* pattr = &(psrv_db->service.attrs[attr_idx]);
    /* get uuid */
    uint8_t uuid_len = pattr->type.len;
    uint16_t uuid = 0;
    if (uuid_len == 2)
    {
      uuid = pattr->type.uuid[1];
      uuid = (uuid << 8) | pattr->type.uuid[0];
      if(is_fixed_uuid(uuid)){
        MI_LOG_ERROR("gatt server set failed: fixed uuid(%d)", uuid);
        return MIBLE_ERR_GATT_INVALID_ATT_TYPE;
      }
    }

    MI_LOG_DEBUG("gatt server set uuid(%d) value", uuid);

    {
      MI_LOG_DEBUG("gatt server set void value");
      uint16_t max_len = psrv_db->char_value_lens[attr_idx];
      
      uint16_t write_len = (max_len < (offset + len)) ? (max_len - offset) : len;
      memcpy(pattr->pValue + offset, p_value, write_len);
    }
    return MI_SUCCESS;
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
mible_status_t mible_gatts_value_get(uint16_t srv_handle,
                                     uint16_t value_handle, uint8_t *p_value, uint8_t *p_len)
{
    if (NULL == p_value)
    {
        return MI_ERR_INVALID_ADDR;
    }

    if (NULL == p_len)
    {
        return MI_ERR_INVALID_LENGTH;
    }

    service_db_t *psrv_db = find_srv_db(value_handle);
    if (NULL == psrv_db)
    {
        return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
    }

    uint8_t attr_idx = value_handle - psrv_db->service.attrs[0].handle;
    gattAttribute_t* pattr = &(psrv_db->service.attrs[attr_idx]);


    {
      MI_LOG_DEBUG("gatt server set void value");
      uint16_t max_len = psrv_db->char_value_lens[attr_idx];
      
      *p_len = (uint8_t)(max_len & 0xff);
      memcpy(p_value, pattr->pValue, *p_len);
    }


    return MI_SUCCESS;
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
mible_status_t mible_gatts_notify_or_indicate(uint16_t conn_handle,
                                              uint16_t srv_handle, uint16_t char_value_handle, uint8_t offset,
                                              uint8_t *p_value, uint8_t len, uint8_t type)
{
  bStatus_t ret = SUCCESS;
  if (NULL == p_value)
  {
      return MI_ERR_INVALID_ADDR;
  }


  service_db_t *psrv_db = find_srv_db(srv_handle);
  if (NULL == psrv_db)
  {
      return MIBLE_ERR_ATT_INVALID_ATT_HANDLE;
  }

  uint8_t attr_idx = char_value_handle - psrv_db->service.attrs[0].handle;
  gattAttribute_t* pattr = &(psrv_db->service.attrs[attr_idx]);

  /* get uuid */
  uint8_t uuid_len = pattr->type.len;
  uint16_t uuid = 0;
  if (uuid_len == 2)
  {
    uuid = pattr->type.uuid[1];
    uuid = (uuid << 8) | pattr->type.uuid[0];
    if(is_fixed_uuid(uuid)){
      MI_LOG_ERROR("gatt server set failed: fixed uuid(%d)", uuid);
      return MIBLE_ERR_GATT_INVALID_ATT_TYPE;
    }
  }

  gattCharCfg_t* pcccd = (gattCharCfg_t*)(pattr[1].pValue);
  uint16_t cccd_value = 0; 
  cccd_value = GATTServApp_ReadCharCfg(conn_handle, pcccd);
  if(cccd_value != (uint16_t)type){
    return MIBLE_ERR_GATT_INVALID_ATT_TYPE;
  }

  if (type == GATT_CLIENT_CFG_NOTIFY)
  {
    attHandleValueNoti_t* pnotify = (attHandleValueNoti_t*)osal_bm_alloc(sizeof(attHandleValueNoti_t));
    if(pnotify == NULL)
      return MI_ERR_NO_MEM;
    osal_memset(pnotify, 0, sizeof(attHandleValueNoti_t));
    pnotify->handle = char_value_handle;
    pnotify->len = offset + len;
    memcpy(pattr->pValue + offset, p_value, len);
    memcpy(pnotify->value, pattr->pValue, pnotify->len);
    ret = GATT_Notification(conn_handle, pnotify, FALSE);
    osal_bm_free(pnotify);
  }
  else if(type == GATT_CLIENT_CFG_INDICATE)
  {
    attHandleValueInd_t* pindicate = (attHandleValueInd_t*)osal_bm_alloc(sizeof(attHandleValueInd_t));
    if(pindicate == NULL)
      return MI_ERR_NO_MEM;
    osal_memset(pindicate, 0, sizeof(attHandleValueInd_t));
    pindicate->handle = char_value_handle;
    pindicate->len = offset + len;
    memcpy(pattr->pValue + offset, p_value, len);
    memcpy(pindicate->value, pattr->pValue, pindicate->len);
    ret = GATT_Indication(conn_handle, pindicate, FALSE, get_taskID());
    
    osal_bm_free(pindicate);

  }

  return err_code_convert(ret);
  ;
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
                                         uint8_t *p_value, uint8_t len, uint8_t type)
{
    return MI_SUCCESS;
}



/******************************************************************************

 @file  cc26xx_api.c

 @brief porting layer for the MiBLE API, for the CC26XX familly devices of TI.

 Group: WCS, BTS
 Target Device: cc13x2_26x2

 ******************************************************************************
 
 Copyright (c) 2018-2019, Texas Instruments Incorporated
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 *  Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.

 *  Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 *  Neither the name of Texas Instruments Incorporated nor the names of
    its contributors may be used to endorse or promote products derived
    from this software without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ******************************************************************************
 
 
 *****************************************************************************/

/*********************************************************************
 * INCLUDES
 */
#include "mible_type.h"
#include "bcomdef.h"
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>
#include <ti/sysbios/knl/Clock.h>

#include "mible_log.h"
#include "mible_api.h"

#include "icall_ble_api.h"

#include <portable/cc26xx_api.h>
#include <common/cc26xx/util.h>
#ifdef CC26X2
#include <services/src/nv/cc26xx/nvoctp.h>
#include <ti/drivers/AESECB.h>
#include <ti/drivers/cryptoutils/cryptokey/CryptoKeyPlaintext.h>
#else  // CC26x2
#include <services/src/nv/cc26xx/nvocop.h>
#include <ti/drivers/crypto/CryptoCC26XX.h>
#endif  // CC26x2

#include "TRNGCC26XX.h"

/*********************************************************************
 * MACROS
 */

#define GATT_PRIMARY_SERVICE(data)  \
{                                   \
  .type =                           \
  {                                 \
    .len  = ATT_BT_UUID_SIZE,       \
    .uuid = primaryServiceUUID      \
  },                                \
  .permissions = GATT_PERMIT_READ,  \
  .handle = 0,                      \
  .pValue = data                    \
}

#define GATT_CHAR_ATTR(data)        \
{                                   \
  .type =                           \
  {                                 \
    .len  = ATT_BT_UUID_SIZE,       \
    .uuid = characterUUID           \
  },                                \
  .permissions = GATT_PERMIT_READ,  \
  .handle = 0,                      \
  .pValue = data                    \
}


#define GATT_CCC_ATTRIB(data)       \
{                                   \
  .type =                           \
  {                                 \
    .len  = ATT_BT_UUID_SIZE,       \
    .uuid = clientCharCfgUUID       \
  },                                \
  .permissions = GATT_PERMIT_READ | GATT_PERMIT_WRITE, \
  .handle = 0,                      \
  .pValue = (uint8 *) data                    \
}

#define GATT_CHAR_FORMAT_ATTRIB(data)       \
{                                   \
  .type =                           \
  {                                 \
    .len  = ATT_BT_UUID_SIZE,       \
    .uuid = charFormatUUID          \
  },                                \
  .permissions = GATT_PERMIT_READ,  \
  .handle = 0,                      \
  .pValue = (uint8 *) data          \
}

#define GATT_CHAR_EXT_PROP_ATTRIB(data)       \
{                                   \
  .type =                           \
  {                                 \
    .len  = ATT_BT_UUID_SIZE,       \
    .uuid = charExtPropsUUID        \
  },                                \
  .permissions = GATT_PERMIT_READ,  \
  .handle = 0,                      \
  .pValue = (uint8 *) data          \
}

#define GATT_CHAR_USER_DESC_ATTRIB(data)       \
{                                   \
  .type =                           \
  {                                 \
    .len  = ATT_BT_UUID_SIZE,       \
    .uuid = charUserDescUUID        \
  },                                \
  .permissions = GATT_PERMIT_READ,  \
  .handle = 0,                      \
  .pValue = (uint8 *) data          \
}

#define GATT_CHAR_VALUE_ATTRIB(lenght, char_uuid, permission, data)        \
{                                   \
  .type =                           \
  {                                 \
    .len  = lenght,                 \
    .uuid = char_uuid               \
  },                                \
  .permissions = permission,        \
  .handle = 0,                      \
  .pValue = data                    \
}

#define STATUS_CHECK(status) {                      \
  if (status != SUCCESS)                            \
  {                                                 \
    MIBLE_PORT_ASSERT(!status);                     \
    MIBLE_PORT_LOG_WAR1("Failure...%d", status);    \
    return(status);                                 \
  }                                                 \
}

const static uint8_t advertDataDefault[] =
{
    0x02, // length of this data
    GAP_ADTYPE_FLAGS,
    GAP_ADTYPE_FLAGS_GENERAL | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED
};

#define NOT_MANAGE_AT_THIS_TIME 0
#define YOU_NEED_TO_ENABLE_AUTHORIZATION_FEATURE 0
/*********************************************************************
 * CONSTANTS
 */

#define MIBLE_UUID_16           0
#define MIBLE_UUID_128          1

#define MIBLE_TASK_Q_EVENT      0xFFFF /* dummy event */

#define MIBLE_SYSTEM            (NVINTF_SYSID_APP+1)

#define MIBLE_NV_RECORD_MAX_SIZE 64
/*********************************************************************
 * TYPEDEFS
 */

//! \brief structure to store the state of an ATT operation.
// per specification, only one ATT req can be active at a time (except for
// ATT WRITE CMD and NOTIFICATION
typedef struct _ATToperation_s
{
  struct _flag_s {
    uint8_t validity : 1;           //!< indicate an ongoing ATT operation.
    uint8_t authorNeeded : 1;       //!< indicate if an ongoing ATT operation requires authorization.
    uint8_t safe_to_dealloc: 1;
  } flag;
  uint8_t          method;          //!< method of the ongoing ATT operation.
  uint16_t         maxLen;          //!< max length of data for this ATT operation.
  uint16_t         maxCharValueLen; //!< Max size of Characteristic Value
  uint16_t         connHandle;      //!< handle of the connection the ATT operation is done for
  gattAttribute_t  *pAttr;          //!< pointer to the attribute the ATT operation is perform on
  uint16_t         offset;          //!< offset of the value to perform the read/write operation
  uint8_t          *pValue;         //!< pointer to the data for the ATT operation
}ATToperation_t;


//! \brief structure to store all characterisitc value parameters for a specific service.
typedef struct _qServiceRec_s
{
  Queue_Elem      _elem;          //!< element information
  gattAttribute_t *pAttrSrvTbl;   //!< pointer to the attribute table for this service
  Queue_Struct    charValueQ;     //!< Structure of the characteristic value queue for this service
  Queue_Handle    charValueQHdl;  //!< Handle of the characteristic value queue for this service
} qServiceRec_t;

//! \brief structure for storing parameters of a specific characteristic.
typedef struct _qCharValueRec_s
{
  Queue_Elem _elem;               //!< element information
  struct _flag2_s {
    uint8_t is_variable_len : 1;  //!< indicate this characteristic value has a variable length
    uint8_t rd_author : 1;        //!< indicate this characteristic value requires read authorization
    uint8_t wr_author : 1;        //!< indicate this characteristic value requires write authorization
  } flag;
  uint8_t         len;            //!< actual length of the characteristic value
  uint8_t         maxLen;         //!< maximum length of the characteristic value (if variable length characteristic value)
  uint16_t        handle;         //!< handle of the characteristic value
  gattAttribute_t *pAttr;         //!< attribute of the charaxteristic value
  gattCharCfg_t   *pCCCValue;     //!< Characteristic client configuration for this characteristic value.
} qCharValueRec_t;

//! \brief structure containing all parameters of the porting layer.
struct port_object_s
{
  uint8_t                 advHandle;     //!< Advertising set handle
  uint8_t                 *advPayload;   //!< Advertising set payload pointer
  uint8_t                 *advSrPayload; //!< Advertising set scan response payload pointer
  ATToperation_t          ATTop;         //!< structure for ATT operation
  mible_port_cb_t         callbacks;     //!< structure for porting layer callback
  Queue_Struct            taskQ;         //!< Queue structure for task
  Queue_Handle            taskQHdl;      //!< Queue handle for task
  Queue_Struct            serviceQ;      //!< Queue structure for Service
  Queue_Handle            serviceQHdl;   //!< Queue handle for Service
  Event_Handle            appSync;       //!< Sync Object to wake up the application thread
  uint8_t                 taskId;        //!< iCall taskid (used for indication confirmation)
  NVINTF_nvFuncts_t       NVpfn;         //!< Structure for NV driver function pointer
};

//! \brief structure for Timer objects
typedef struct
{
  Clock_Struct clock;       //!< Clock object for timer
  mible_timer_handler cb;   //!< callback that will be called when clock expired.
  void *arg;                //!< argument pass to the callback.
}timer_t;


//! \brief structure for GATT event.
typedef struct
{
    mible_gatts_evt_t evt;                //!< event triggered
    mible_gatts_evt_param_t gattsParams;  //!< parameter of the GATT event
}gatt_event_t;

//! \brief structure for task.
typedef struct _qTaskRec_s
{
  Queue_Elem      _elem;          //!< element  (private)
  mible_handler_t handler;        //!< handler, will be call when the task execute
  void            *arg;           //!< will be pass as an argument to the handler
} qTaskRec_t;

/*********************************************************************
 * GLOBAL VARIABLES
 */

/*********************************************************************
 * LOCAL FUNCTIONS
 */
static bStatus_t miapi_port_ReadAttrCB(uint16_t connHandle,
                                         gattAttribute_t *pAttr,
                                         uint8_t *pValue, uint16_t *pLen,
                                         uint16_t offset, uint16_t maxLen,
                                         uint8_t method);
static bStatus_t miapi_port_WriteAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t len,
                                          uint16_t offset, uint8_t method);
static void miapi_port_init (Event_Handle sync, uint8_t taskId);
static void miapi_port_gapEvent (gapEventHdr_t * data);
static void miapi_port_gattEvent (gattMsgEvent_t * data);
static void miapi_port_stackEvent (uint8_t stackEventCat , void * data);
static void miapi_port_advReport( void *pData );

// Note: When an operation on a characteristic requires authorization and
// pfnAuthorizeAttrCB is not defined for that characteristic's service, the
// Stack will report a status of ATT_ERR_UNLIKELY to the client.  When an
// operation on a characteristic requires authorization the Stack will call
// pfnAuthorizeAttrCB to check a client's authorization prior to calling
// pfnReadAttrCB or pfnWriteAttrCB, so no checks for authorization need to be
// made within these functions.
CONST gattServiceCBs_t MibleProfileCBs = {
  miapi_port_ReadAttrCB,  // Read callback function pointer
  miapi_port_WriteAttrCB, // Write callback function pointer
  NULL                    // Authorization callback function pointer
};

/*********************************************************************
 * LOCAL VARIABLES
 */
static struct port_object_s port_object =
{
  .advHandle = 0,
  .advPayload = NULL,
  .advSrPayload = NULL,
  .ATTop = {0},
  .callbacks =
  {
    .init = miapi_port_init,
    .stackEvent = miapi_port_stackEvent,
    .taskExec = mible_tasks_exec,
  },
  .taskQ = NULL,
  .taskQHdl = NULL,
  .serviceQ = NULL,
  .serviceQHdl = NULL,
  .appSync = NULL,
  .taskId = INVALID_TASK_ID,
  .NVpfn = NULL,
};

#ifndef CC26X2
static uint16_t NV_bitfield = 0x0000;   // each bit indicate a customer NV record whether assigned
#endif // ! CC26X2

/*********************************************************************
 * FUNCTIONS
 */

void miapi_port_timer_cb(void *arg)
{
  timer_t* timer = (timer_t*) arg;
  mible_task_post(timer->cb, timer->arg);
}

mible_status_t miapi_port_translateError(uint8_t status)
{
    mible_status_t res = MI_SUCCESS;
    if (status != SUCCESS)                    
    {                                         
      if (status == bleMemAllocError)         
      {                                       
          res = MI_ERR_RESOURCES;
      }                                       
      else if (status == bleInternalError)    
      {                                       
          res = MI_ERR_INVALID_STATE;
      }                                       
      else if (status == INVALIDPARAMETER)    
      {                                       
          res = MI_ERR_INVALID_PARAM;
      }                                       
      else if (status == bleIncorrectMode)    
      {                                       
          res = MI_ERR_INVALID_STATE;
      }                                       
      else if (status == bleNotReady)         
      {                                       
          res = MI_ERR_BUSY;
      }                                       
      else if (status != SUCCESS)             
      {                                       
          res = MI_ERR_BUSY;
      }                                       
      MIBLE_PORT_LOG_WAR1("Status Error  : 0x%x)", status);
    }
    return(res);
} 

static qCharValueRec_t* miapi_port_GattTblFindCharByPtrs(uint8_t *pValue)
{
  qServiceRec_t *elemSer;

  for (elemSer = Queue_head(port_object.serviceQHdl);
       elemSer != (qServiceRec_t *)port_object.serviceQHdl;
       elemSer = Queue_next(&elemSer->_elem))
  {
      qCharValueRec_t *elemChar;

      for (elemChar = Queue_head(elemSer->charValueQHdl);
           elemChar != (qCharValueRec_t *)elemSer->charValueQHdl;
           elemChar = Queue_next(&elemChar->_elem))
      {
         if( elemChar->pAttr->pValue == pValue )
         {
             return(elemChar);
         }
      }
  }
  return(NULL);
}

static bStatus_t miapi_port_SendNotiInd(uint16 connHandle, uint8 cccValue, gattAttribute_t *pAttr, pfnGATTReadAttrCB_t pfnReadAttrCB)
{
  attHandleValueNoti_t noti;
  uint16 len;
  bStatus_t status;

  // If the attribute value is longer than (ATT_MTU - 3) octets, then
  // only the first (ATT_MTU - 3) octets of this attributes value can
  // be sent in a notification.
  noti.pValue = (uint8 *)GATT_bm_alloc(connHandle, ATT_HANDLE_VALUE_NOTI,
                                        GATT_MAX_MTU, &len);
  if (noti.pValue != NULL)
  {
    noti.handle = pAttr->handle;

    status = (*pfnReadAttrCB)( connHandle, pAttr, noti.pValue, &noti.len,
                               0, len, GATT_LOCAL_READ );

    if (cccValue & GATT_CLIENT_CFG_NOTIFY)
    {
      status = GATT_Notification(connHandle, &noti, FALSE);
    }
    else // GATT_CLIENT_CFG_INDICATE
    {
      status = GATT_Indication(connHandle, (attHandleValueInd_t *)&noti,
                               FALSE, port_object.taskId);
    }

    if (status != SUCCESS)
    {
      GATT_bm_free((gattMsg_t *)&noti, ATT_HANDLE_VALUE_NOTI);
    }
  }
  else
  {
    status = bleNoResources;
  }

  return (status);
}

static bStatus_t miapi_port_ReadAttrValue(gattAttribute_t *pAttr,
                                            uint8_t *pValue, uint16_t *pLen,
                                            uint16_t offset, uint16_t maxLen)
{
  // find attribute in the database...
  // GATT server will take care of reading the USER_DESC, CHAR Format UUID, CCC, extended properties, server cfg, ...
  // Other characteristic attribute should not exist based on the service definition limitation of the Xiaomi API.
  // Therefore, only Characteristic value attribute are considered bellow.
  qCharValueRec_t* charValue = miapi_port_GattTblFindCharByPtrs((uint8_t*) pAttr->pValue);
  MIBLE_PORT_ASSERT(charValue);
  if (offset > charValue->len)
  {
    return(ATT_ERR_INVALID_OFFSET);
  }

  uint16_t dataSize = ((charValue->len - offset) > maxLen)? maxLen:(charValue->len - offset);

  VOID memcpy(pValue, pAttr->pValue + offset, dataSize);
  *pLen = dataSize;
  MIBLE_PORT_DBG1("Len Returned: %d", dataSize);
  return(SUCCESS);
}

static void miapi_port_gattTask(void * param)
{
  gatt_event_t *author = (gatt_event_t*)param;
  mible_gatts_event_callback(author->evt, &author->gattsParams);

  // If this is a answer to a write request, the initial buffer that stores 
  // the write request value needs to be freed.
  if (port_object.ATTop.flag.safe_to_dealloc)
  {
    if(port_object.ATTop.pValue)
    {
      BM_free(port_object.ATTop.pValue);
    }
    port_object.ATTop.flag.safe_to_dealloc = 0;
  }
  
  ICall_free(author);
}

/*********************************************************************
 * @fn          miapi_port_ReadAttrCB
 *
 * @brief       Read an attribute.
 *
 * @param       connHandle - connection message was received on
 * @param       pAttr - pointer to attribute
 * @param       pValue - pointer to data to be read
 * @param       pLen - length of data to be read
 * @param       offset - offset of the first octet to be read
 * @param       maxLen - maximum length of data to be read
 * @param       method - type of read message
 *
 * @return      SUCCESS, blePending or Failure
 */
static bStatus_t miapi_port_ReadAttrCB(uint16_t connHandle,
                                       gattAttribute_t *pAttr,
                                       uint8_t *pValue, uint16_t *pLen,
                                       uint16_t offset, uint16_t maxLen,
                                       uint8_t method)
{
    qCharValueRec_t* charValue = miapi_port_GattTblFindCharByPtrs((uint8_t*) pAttr->pValue);
    MIBLE_PORT_ASSERT(charValue);
    MIBLE_PORT_DBG3(": Handle: %d , method: %d, offset: %d, ", pAttr->handle, method, offset);
#ifdef MIAPI_PORT_AUTHORIZATION_FTR
    // Check is it requires authorization//
    if (charValue->flag.rd_author)
    {
        // if yes, return ble_epending and signal it.
        // Store all needed information about the ATT operation...
        // only one ATT operation at teh same time, some method are not supported yet.
        switch(method)
        {
            case ATT_READ_REQ:
            case ATT_READ_BLOB_REQ:
            case ATT_READ_BY_TYPE_REQ:
                MIBLE_PORT_ASSERT(!port_object.ATTop.flag.validity);
                if (port_object.ATTop.flag.validity)
                {
                    return(bleInternalError);
                }
                port_object.ATTop.flag.validity = 1;
                port_object.ATTop.flag.authorNeeded = 1;
                port_object.ATTop.flag.safe_to_dealloc = 0;                
                port_object.ATTop.method = method;
                port_object.ATTop.connHandle = connHandle;
                port_object.ATTop.pAttr = pAttr;
                port_object.ATTop.maxLen = maxLen;
                port_object.ATTop.offset = offset;
                // stack a tasklet to call the callback in a different context...
                {
                  gatt_event_t* author = ICall_mallocLimited(sizeof(gatt_event_t));
                  MIBLE_PORT_ASSERT(author);
                  if(!author) return(bleNoResources);
                  author->evt = MIBLE_GATTS_EVT_READ_PERMIT_REQ;
                  author->gattsParams.conn_handle = connHandle;
                  author->gattsParams.read.value_handle = pAttr->handle;
                  mible_status_t status = mible_task_post(miapi_port_gattTask, author);
                  MIBLE_PORT_ASSERT(status==MI_SUCCESS);
                  if(MI_SUCCESS != status)
                  {
                    ICall_free(author);
                    return(bleNoResources);
                  }
                }
                return(blePending);
            case ATT_READ_BY_GRP_TYPE_REQ:
            default:
                MIBLE_PORT_LOG_WAR1("this method is not supported with authorization: %d", method);
                return(bleNoResources);
        }
    }
    else
#endif // #ifndef MIAPI_PORT_AUTHORIZATION_FTR
    {
        // no Authorization callback has been done.
        // reply with the read.
        return(miapi_port_ReadAttrValue(pAttr, pValue, pLen, offset, maxLen));
    }
}

static bStatus_t miapi_port_WriteAttrValue(uint8_t connHandle, gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset)
{  
  bStatus_t status = SUCCESS;
    
  if ((pAttr->type.len == ATT_BT_UUID_SIZE) &&
      (GATT_CLIENT_CHAR_CFG_UUID == BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1])))
  {
    // 16-bit UUID
    status = GATTServApp_ProcessCCCWriteReq(connHandle, pAttr, pValue, len,
                                              offset, GATT_CLIENT_CFG_NOTIFY | GATT_CLIENT_CFG_INDICATE);
  }
  else
  {
    // Find Attribute to write on
    // At this point, this should only be a Characteristic Value, since all other attribute are READ ONLY.
    qCharValueRec_t* charValue = miapi_port_GattTblFindCharByPtrs(pAttr->pValue);
    MIBLE_PORT_ASSERT(charValue);
    if ((offset > charValue->maxLen) || ((offset + len) > charValue->maxLen))
    {
      return(ATT_ERR_INVALID_OFFSET);
    }
    else if (len > charValue->maxLen)
    {
      return(ATT_ERR_INVALID_VALUE_SIZE);
    }

    if(charValue->flag.is_variable_len)
    {
      charValue->len = len + offset;
    }
    uint8 *pCurValue = (uint8 *)pAttr->pValue;
    VOID memcpy(pCurValue + offset, pValue, len);
  }

  if (status == SUCCESS)
  {
    // start a task to call the callback in a different context...
    {
      gatt_event_t* author = ICall_mallocLimited(sizeof(gatt_event_t));
      MIBLE_PORT_ASSERT(author);
      if(!author) return(bleNoResources);
      author->evt = MIBLE_GATTS_EVT_WRITE;
      author->gattsParams.conn_handle = connHandle;
      author->gattsParams.write.value_handle = pAttr->handle;
      if ((pAttr->type.len == ATT_BT_UUID_SIZE) &&
          (GATT_CLIENT_CHAR_CFG_UUID == BUILD_UINT16(pAttr->type.uuid[0], pAttr->type.uuid[1])))
      {
          gattCharCfg_t * chartbl = GATT_CCC_TBL(pAttr->pValue);
          for ( uint8_t i = 0; i < linkDBNumConns; i++ )
          {
            if ( chartbl[i].connHandle == connHandle )
            {
              // Entry found
                author->gattsParams.write.data =  (uint8_t*) ( &(chartbl[i].value) );
                break;
            }
          }
      }
      else
      {
          author->gattsParams.write.data = (pAttr->pValue) + offset;
      }
      author->gattsParams.write.len = len;
      author->gattsParams.write.offset = offset;
      mible_status_t status = mible_task_post(miapi_port_gattTask, author);
      MIBLE_PORT_ASSERT(status==MI_SUCCESS);
      if(MI_SUCCESS != status)
      {
        return(bleNoResources);
      }
    }
  }
  return(status);
}

/*********************************************************************
 * @fn      miapi_port_WriteAttrCB
 *
 * @brief   Validate attribute data prior to a write operation
 *
 * @param   connHandle - connection message was received on
 * @param   pAttr - pointer to attribute
 * @param   pValue - pointer to data to be written
 * @param   len - length of data
 * @param   offset - offset of the first octet to be written
 * @param   method - type of write message
 *
 * @return  SUCCESS, blePending or Failure
 */
static bStatus_t miapi_port_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method)
{
  MIBLE_PORT_DBG2(": Handle: %d , method: %d", pAttr->handle, method);
#ifdef MIAPI_PORT_AUTHORIZATION_FTR
  qCharValueRec_t* charValue = miapi_port_GattTblFindCharByPtrs((uint8_t*) pAttr->pValue);
  // The attribute being written could be a CCC attribute.
  // if so, charValue will be null... and no authorization is requiered.
  // Check is it requires authorization
  if ((charValue) && (charValue->flag.wr_author))
  {
    // if yes, return ble_epending and signal it.
    // Store all needed information about the ATT operation...
    // only one ATT operation at teh same time, some method are not supported yet.
    switch(method)
    {
       case ATT_WRITE_REQ:
         MIBLE_PORT_ASSERT(!port_object.ATTop.flag.validity);
         if (port_object.ATTop.flag.validity)
         {
           return(bleInternalError);
         }
         port_object.ATTop.flag.validity = 1;
         port_object.ATTop.flag.authorNeeded = 1;
         port_object.ATTop.flag.safe_to_dealloc = 1;
         port_object.ATTop.connHandle = connHandle;
         port_object.ATTop.maxLen = len;
         port_object.ATTop.pAttr  = pAttr;
         port_object.ATTop.offset = offset;
         port_object.ATTop.pValue = pValue;
         port_object.ATTop.method = method;
         // start a tasklet to call the callback in a different context...
         {
           gatt_event_t* author = ICall_mallocLimited(sizeof(gatt_event_t));
           MIBLE_PORT_ASSERT(author);
           if(!author) return(bleNoResources);
           author->evt = MIBLE_GATTS_EVT_WRITE_PERMIT_REQ;
           author->gattsParams.conn_handle = connHandle;
           author->gattsParams.write.value_handle = pAttr->handle;
           author->gattsParams.write.data = pValue;
           author->gattsParams.write.len = len;
           author->gattsParams.write.offset = offset;
           mible_status_t status = mible_task_post(miapi_port_gattTask, author);
           MIBLE_PORT_ASSERT(status==MI_SUCCESS);
           if(MI_SUCCESS != status)
           {
             ICall_free(author);
             return(bleNoResources);
           }
         }
         return(blePending);
       case ATT_EXECUTE_WRITE_REQ:
       case ATT_PREPARE_WRITE_REQ:
       case ATT_WRITE_CMD:
       case ATT_SIGNED_WRITE_CMD:
       default:
         MIBLE_PORT_LOG_WAR1("this method is not supported with authorization: %d", method);
         return(bleNoResources);
    }
  }
  else
#endif // MIAPI_PORT_AUTHORIZATION_FTR
  {
    // no Authorization callback has been done.
    // reply with the read.
    return(miapi_port_WriteAttrValue(connHandle, pAttr, pValue, len, offset));
  }
}

/*********************************************************************
 * @fn      miapi_port_advCallback
 *
 * @brief   GapAdv module callback
 *
 * @param   pMsg - message to process
 */
static void miapi_port_advCallback(uint32_t event, void *pBuf, uintptr_t arg)
{
}

/*********************************************************************
 * @fn      miapi_port_scanCb
 *
 * @brief   Callback called by GapScan module
 *
 * @param   evt - event
 * @param   msg - message coming with the event
 * @param   arg - user argument
 *
 * @return  none
 */
void miapi_port_scanCb(uint32_t evt, void* pMsg, uintptr_t arg)
{
  if (evt & GAP_EVT_ADV_REPORT)
  {
    // The report will come back as an event...
    mible_task_post(miapi_port_advReport, pMsg);
  }
  else
  {
    return;
  }
}

static void miapi_port_advReport( void *pData )
{
  mible_gap_evt_t evt;
  mible_gap_evt_param_t gap_params = {0};
  GapScan_Evt_AdvRpt_t* pAdvRpt = (GapScan_Evt_AdvRpt_t*) (pData);
  
  gap_params.conn_handle = 0;
  gap_params.report.addr_type = (pAdvRpt->addrType == ADDRTYPE_PUBLIC || pAdvRpt->addrType == ADDRTYPE_PUBLIC_ID) ? MIBLE_ADDRESS_TYPE_PUBLIC : MIBLE_ADDRESS_TYPE_RANDOM;
  memcpy(gap_params.report.peer_addr, pAdvRpt->addr, 6);
  gap_params.report.adv_type = (pAdvRpt->evtType == ADV_RPT_EVT_TYPE_SCAN_RSP) ? SCAN_RSP_DATA : ADV_DATA;
  gap_params.report.rssi = pAdvRpt->rssi;
  memcpy(gap_params.report.data, pAdvRpt->pData, pAdvRpt->dataLen);;
  gap_params.report.data_len = pAdvRpt->dataLen;  
  
  MI_LOG_INFO("addr_type:%d, peer:%02x:%02x:%02x:%02x:%02x:%02x, event:%d, rssi:%d, len:%d",
              gap_params.report.addr_type, gap_params.report.peer_addr[0], gap_params.report.peer_addr[1], gap_params.report.peer_addr[2],
              gap_params.report.peer_addr[3], gap_params.report.peer_addr[4], gap_params.report.peer_addr[5], gap_params.report.peer_addr,
              gap_params.report.rssi, gap_params.report.data_len);

  evt = MIBLE_GAP_EVT_ADV_REPORT;
  mible_gap_event_callback(evt, &gap_params);
}

/**
 *        GATT Server APIs
 */
static uint16_t miapi_port_getServiceSize(mible_gatts_srv_db_t *pSrv)
{
  uint8_t mibleAttrTblNum = 0;
  // To get attribute number first.
  // First attribute in service define is always Service Declaration
  mibleAttrTblNum++;

  // Inquire whole char list to calculate number of attributes.
  for (uint16_t charIdx = 0; charIdx < pSrv->char_num; charIdx++)
  {
    mible_gatts_char_db_t *pChar = pSrv->p_char_db + charIdx;
    
    // Each char at least has a declaration & value attribute. mibleAttrTblNum+=2
    mibleAttrTblNum = mibleAttrTblNum + 2; 
    // If the char has notify property, it should have a CCC attribute
    if (pChar->char_property & ( MIBLE_NOTIFY | MIBLE_INDICATE)) mibleAttrTblNum++;
    if (pChar->char_desc_db.extend_prop) mibleAttrTblNum++;
    if (pChar->char_desc_db.char_format) mibleAttrTblNum++;
    if (pChar->char_desc_db.user_desc) mibleAttrTblNum++;
  }
  return(mibleAttrTblNum);
}

static mible_status_t miapi_port_GattTblConvert(mible_gatts_srv_db_t *pSrv)
{
  bStatus_t ret = SUCCESS;
  uint8_t charIdx = 0;
  uint16_t mibleAttrTblIdx = 0;
  gattAttribute_t *pMibleAttrTbl = NULL;
  uint16_t mibleAttrTblNum = miapi_port_getServiceSize(pSrv);
  uint16_t offsetHandle = GATT_GetNextHandle();
  
  MIBLE_PORT_LOG_HEAP_START();

  pMibleAttrTbl = (gattAttribute_t*)ICall_mallocLimited(sizeof(gattAttribute_t) * mibleAttrTblNum);
  if (pMibleAttrTbl == NULL)
  {
    MIBLE_PORT_ASSERT(pMibleAttrTbl);
    return(MI_ERR_NO_MEM);
  }

  qServiceRec_t *pMibleAttrTblTmp = (qServiceRec_t*)ICall_mallocLimited(sizeof(qServiceRec_t));
  memset(pMibleAttrTblTmp, 0, sizeof(qServiceRec_t));
  if (pMibleAttrTblTmp == NULL)
  {
    // all previously allocated memory should be free, but there is no point to continue execution
    // if no more memory are available...
    MIBLE_PORT_ASSERT(pMibleAttrTblTmp);
    return(MI_ERR_NO_MEM);
  }

  pMibleAttrTblTmp->charValueQHdl = Util_constructQueue(&pMibleAttrTblTmp->charValueQ);

  uint8_t pSrvUUIDlen = (pSrv->srv_uuid.type == MIBLE_UUID_16) ? ATT_BT_UUID_SIZE : ATT_UUID_SIZE;
  uint8_t *pSrvUUID = (uint8_t*)ICall_mallocLimited(pSrvUUIDlen);
  if (pSrvUUID == NULL)
  {
    MIBLE_PORT_ASSERT(pMibleAttrTblTmp);
    return(MI_ERR_NO_MEM);
  }
  if (pSrv->srv_uuid.type == MIBLE_UUID_16)
  {
    (void) memcpy(pSrvUUID, &pSrv->srv_uuid.uuid16, pSrvUUIDlen);
  }
  else
  {
    (void) memcpy(pSrvUUID, pSrv->srv_uuid.uuid128, pSrvUUIDlen);
  }

  // Locate service UUID type
  gattAttrType_t *pServiceUUIDType = (gattAttrType_t*)ICall_mallocLimited(sizeof(gattAttrType_t));
  if (pServiceUUIDType == NULL)
  {
    MIBLE_PORT_ASSERT(pServiceUUIDType);
    return(MI_ERR_NO_MEM);
  }
  
  pServiceUUIDType->len = pSrvUUIDlen;
  pServiceUUIDType->uuid = pSrvUUID;
      
  // Create a temporary attribute body, due to pValue member is const pointer, need to use memcpy to copy later.
  gattAttribute_t tmpAttrSrvDec = GATT_PRIMARY_SERVICE((uint8 *)pServiceUUIDType);

  if (pSrv->srv_type != 1)
  {
      tmpAttrSrvDec.type.uuid = (const uint8 *) secondaryServiceUUID;
  }

  pSrv->srv_handle = mibleAttrTblIdx + offsetHandle;
  VOID memcpy(pMibleAttrTbl + mibleAttrTblIdx++, &tmpAttrSrvDec, sizeof(gattAttribute_t));

  // Now copy all the char attributes' information to the table.
  for (charIdx = 0; charIdx < pSrv->char_num; charIdx++)
  {
    mible_gatts_char_db_t *pChar = &pSrv->p_char_db[charIdx];
    // Copy char declaration.
    uint8_t *pCharProp = (uint8_t*)ICall_mallocLimited(sizeof(uint8_t));
    if (pCharProp == NULL)
    {
      // all previously allocated memory should be free, but there is no point to continue execution
      // if no more memory are available...
      MIBLE_PORT_ASSERT(pCharProp);
      return(MI_ERR_NO_MEM);
    }
    
    *pCharProp = pChar->char_property;
    
    gattAttribute_t tmpAttrCharDec = GATT_CHAR_ATTR(pCharProp);
    
    VOID memcpy(pMibleAttrTbl + mibleAttrTblIdx++, &tmpAttrCharDec, sizeof(gattAttribute_t));

    // Copy char value.
    // UUID
    uint8_t pCharUUIDlen = (pChar->char_uuid.type == MIBLE_UUID_16) ? ATT_BT_UUID_SIZE : ATT_UUID_SIZE;
    uint8_t *pCharUUID = (uint8_t*)ICall_mallocLimited(pCharUUIDlen);
  
    if (pCharUUID == NULL)
    {
      // all previously allocated memory should be free, but there is no point to continue execution
      // if no more memory are available...
      MIBLE_PORT_ASSERT(pCharUUID);
      return(MI_ERR_NO_MEM);
    }
    if (pChar->char_uuid.type == MIBLE_UUID_16)
    {
      (void) memcpy(pCharUUID, &pChar->char_uuid.uuid16, pCharUUIDlen);
    }
    else
    {
      (void) memcpy(pCharUUID, pChar->char_uuid.uuid128, pCharUUIDlen);
    }

    // Value permissions.
    uint8_t vPermission = 0;
    if (pChar->char_property & MIBLE_READ)
    {
      vPermission |= GATT_PERMIT_READ;
    }
    if ((pChar->char_property & MIBLE_WRITE) || (pChar->char_property & MIBLE_WRITE_WITHOUT_RESP))
    {
      vPermission |= GATT_PERMIT_WRITE;
    }
    if (pChar->rd_author)
    {
      vPermission |= GATT_PERMIT_READ;
#ifndef MIAPI_PORT_AUTHORIZATION_FTR
      MIBLE_PORT_ASSERT(YOU_NEED_TO_ENABLE_AUTHORIZATION_FEATURE);
#endif // MIAPI_PORT_AUTHORIZATION_FTR
    }
    if (pChar->wr_author)
    {
      vPermission |= GATT_PERMIT_WRITE;
#ifndef MIAPI_PORT_AUTHORIZATION_FTR
      MIBLE_PORT_ASSERT(YOU_NEED_TO_ENABLE_AUTHORIZATION_FEATURE);
#endif // MIAPI_PORT_AUTHORIZATION_FTR
    }
    
    // Data, Value
    // if characteristic is variable length, the len is considered the maximum length of the characteristic?
    uint8_t *pCharValue = (uint8_t*)ICall_mallocLimited(pChar->char_value_len);
    if (pCharValue == NULL)
    {
      // all previously allocated memory should be free, but there is no point to continue execution
      // if no more memory are available...
      MIBLE_PORT_ASSERT(pCharValue);
      return(MI_ERR_NO_MEM);
    }
     
    VOID memcpy(pCharValue, pChar->p_value, pChar->char_value_len);
    
    gattAttribute_t tmpAttrCharValue= GATT_CHAR_VALUE_ATTRIB(pCharUUIDlen, pCharUUID, vPermission, pCharValue);

    qCharValueRec_t *pCharValueTemp = (qCharValueRec_t*)ICall_mallocLimited(sizeof(qCharValueRec_t));
    if (pCharValueTemp == NULL)
    {
      // all previously allocated memory should be free, but there is no point to continue execution
      // if no more memory are available...
      MIBLE_PORT_ASSERT(pCharValueTemp);
      return(MI_ERR_NO_MEM);
    }
    memset(pCharValueTemp, 0, sizeof(qCharValueRec_t));
    pCharValueTemp->flag.is_variable_len = pChar->is_variable_len;
#ifdef MIAPI_PORT_AUTHORIZATION_FTR
    pCharValueTemp->flag.rd_author = pChar->rd_author;
    pCharValueTemp->flag.wr_author = pChar->wr_author;
#endif //MIAPI_PORT_AUTHORIZATION_FTR
    pCharValueTemp->handle = 0;
    //pCharValueTemp->pCharValue = pCharValue;
    pCharValueTemp->pCCCValue = 0;
    pCharValueTemp->pAttr = pMibleAttrTbl + mibleAttrTblIdx;
    pCharValueTemp->len = pChar->char_value_len;
    pCharValueTemp->maxLen = pChar->char_value_len;

    pChar->char_value_handle = pCharValueTemp->handle = mibleAttrTblIdx + offsetHandle;
    VOID memcpy(pMibleAttrTbl + mibleAttrTblIdx++, &tmpAttrCharValue, sizeof(gattAttribute_t));
    
    if (pChar->char_property & (MIBLE_NOTIFY | MIBLE_INDICATE) )
    {
      gattCharCfg_t *pCharCCC = (gattCharCfg_t*)ICall_mallocLimited(sizeof(gattCharCfg_t*) + sizeof(gattCharCfg_t) * MAX_NUM_BLE_CONNS);
      if (pCharCCC == NULL)
      {
        // all previously allocated memory should be free, but there is no point to continue execution
        // if no more memory are available...
        MIBLE_PORT_ASSERT(pCharCCC);
        return(MI_ERR_NO_MEM);
      }
      
      //Assign the value
      *((uint8_t**)(pCharCCC)) = ((uint8_t*)(pCharCCC)) + sizeof(gattCharCfg_t*);

      gattAttribute_t tmpAttrCharCCC = GATT_CCC_ATTRIB(pCharCCC);
      
      VOID memcpy(pMibleAttrTbl + mibleAttrTblIdx++, &tmpAttrCharCCC, sizeof(gattAttribute_t));
      pCharValueTemp->pCCCValue = pCharCCC;
       
      GATTServApp_InitCharCfg(LINKDB_CONNHANDLE_INVALID, GATT_CCC_TBL(pCharCCC));
    }

    if (pChar->char_desc_db.extend_prop)
    {
      uint16_t *pCharExtProp = (uint16_t*)ICall_mallocLimited(sizeof(uint16_t));
      *pCharExtProp = 0;
      if (pCharExtProp == NULL)
      {
        // all previously allocated memory should be free, but there is no point to continue execution
        // if no more memory are available...
        MIBLE_PORT_ASSERT(pCharExtProp);
        return(MI_ERR_NO_MEM);
      }

      // This copy works fine since structure gattCharFormat_t and mible_gatts_char_desc_user_desc_t are identical.
      *pCharExtProp = (pChar->char_desc_db.extend_prop->reliable_write |  pChar->char_desc_db.extend_prop->writeable);

      gattAttribute_t tmpExtProp = GATT_CHAR_EXT_PROP_ATTRIB(pCharExtProp);
      VOID memcpy(pMibleAttrTbl + mibleAttrTblIdx++, &tmpExtProp, sizeof(gattAttribute_t));
    }

    if (pChar->char_desc_db.char_format)
    {
        gattCharFormat_t *pCharFormat = (gattCharFormat_t*)ICall_mallocLimited(sizeof(gattCharFormat_t));
        if (pCharFormat == NULL)
        {
          // all previously allocated memory should be free, but there is no point to continue execution
          // if no more memory are available...
          MIBLE_PORT_ASSERT(pCharFormat);
          return(MI_ERR_NO_MEM);
        }

        // This copy works fine since structure gattCharFormat_t and mible_gatts_char_desc_user_desc_t are identical.
        MIBLE_PORT_ASSERT(sizeof(mible_gatts_char_desc_user_desc_t) == sizeof(gattCharFormat_t));
        VOID memcpy(pCharFormat, pChar->char_desc_db.char_format, sizeof(gattCharFormat_t));

        gattAttribute_t tmpAttrFormat = GATT_CHAR_FORMAT_ATTRIB(pCharFormat);
        VOID memcpy(pMibleAttrTbl + mibleAttrTblIdx++, &tmpAttrFormat, sizeof(gattAttribute_t));
    }

    if (pChar->char_desc_db.user_desc)
    {
      uint8_t *pCharUserDesc = (uint8_t*)ICall_mallocLimited(pChar->char_desc_db.user_desc->len);
      if (pCharUserDesc == NULL)
      {
        // all previously allocated memory should be free, but there is no point to continue execution
        // if no more memory are available...
        MIBLE_PORT_ASSERT(pCharUserDesc);
        return(MI_ERR_NO_MEM);
      }

      // This copy works fine since structure gattCharFormat_t and mible_gatts_char_desc_user_desc_t are identical.
      VOID memcpy(pCharUserDesc, pChar->char_desc_db.user_desc->string, pChar->char_desc_db.user_desc->len);

      gattAttribute_t tmpAttrUserDesc = GATT_CHAR_USER_DESC_ATTRIB(pCharUserDesc);
      VOID memcpy(pMibleAttrTbl + mibleAttrTblIdx++, &tmpAttrUserDesc, sizeof(gattAttribute_t));
    }
    Queue_put(pMibleAttrTblTmp->charValueQHdl, &pCharValueTemp->_elem);
  }  // end for loop
  
  
  pMibleAttrTblTmp->pAttrSrvTbl = pMibleAttrTbl;

  Queue_put(port_object.serviceQHdl, &pMibleAttrTblTmp->_elem);

  // Register GATT attribute list and CBs with GATT Server App
  ret = GATTServApp_RegisterService(pMibleAttrTbl,
                                    mibleAttrTblNum,
                                    GATT_MAX_ENCRYPT_KEY_SIZE,
                                    &MibleProfileCBs);
  MIBLE_PORT_LOG_HEAP_END()
  return(miapi_port_translateError(ret));
}

/*********************************************************************
 * @fn      miapi_port_miapi_porting_init
 *
 * @brief   Initialization function of the porting layer.
 * 
 *          This function will register the necessary API to interact
 *          with the application and the ble stack.
 *          The following callback are registered:
 *          init_cb: callback returned once the stack and application
 *          is initialized
 *          gap_event_cb: call when a GAP event is send by the stack/application
 *          miapi_task: call to execute any pending task in a low priority context.
 *
 *          
 * @param   pMsg - message to process
 */
void miapi_port_initialization(mible_port_cb_t **cb)
{
    MIBLE_PORT_ASSERT(cb);
    *cb = &port_object.callbacks;
    MIBLE_PORT_DBG1("callback registered: %p", (uintptr_t)*cb);
}

/*********************************************************************
 * LOCAL FUNCTIONS
 */

static void miapi_port_init (Event_Handle sync, uint8_t taskId)
{
  uint8_t status;

  // Initialize the porting Layer
  // Create an RTOS queue for message from profile to be sent to app
  MIBLE_PORT_DBG1("Sync: %p , enqueue: %p", (uintptr_t) sync);
  port_object.taskQHdl = Util_constructQueue(&port_object.taskQ);
  port_object.appSync = sync;
  port_object.taskId = taskId;
  
  port_object.serviceQHdl = Util_constructQueue(&port_object.serviceQ);

  // Create Legacy Advertisement Set that will be used by MiBLE
  GapAdv_params_t advParam = GAPADV_PARAMS_LEGACY_SCANN_CONN;

  // Create Advertisement set #1 and assign handle
  status = GapAdv_create((pfnGapCB_t *) &miapi_port_advCallback, &advParam, &port_object.advHandle);
  MIBLE_PORT_ASSERT(status==SUCCESS);

  port_object.advPayload = (uint8_t*) ICall_mallocLimited(sizeof(advertDataDefault));
  memcpy(port_object.advPayload, advertDataDefault, sizeof(advertDataDefault));
  MIBLE_PORT_ASSERT(port_object.advPayload);
  status = GapAdv_loadByHandle(port_object.advHandle, GAP_ADV_DATA_TYPE_ADV,
                               sizeof(advertDataDefault), port_object.advPayload);
  MIBLE_PORT_ASSERT(status==SUCCESS);
  
  // Get NVS driver API.
  // the stack is initializing the NV area.
#ifdef CC26X2
  NVOCTP_loadApiPtrs(&port_object.NVpfn);
#else
  port_object.NVpfn.compactNV = NVOCOP_compactNV;
  port_object.NVpfn.readItem = NVOCOP_readItem;
  port_object.NVpfn.writeItem = NVOCOP_writeItem;
#endif // CC26X2
  MIBLE_PORT_DBG0("Done");
}

/* This function handle All Stack Event received.
 * No Memory are Freed at this time, this needs to be done in the main application thread.
 */
static void miapi_port_stackEvent (uint8_t stackEventCat, void * data)
{
  if(port_object.appSync)
  {
    switch(stackEventCat)
    {
      case GAP_MSG_EVENT:
      {
        gapEventHdr_t* pMsg = (gapEventHdr_t*) data;
        miapi_port_gapEvent(pMsg);
        break;
      }
      case GATT_MSG_EVENT:
      {
        gattMsgEvent_t *pMsg = (gattMsgEvent_t*) data;
        miapi_port_gattEvent(pMsg);
        break;
      }
      default:
          break;
    }
  }
}

/* This function handle GATT event received from the Stack
 * No Memory are freed at this time, this needs to be done in the main application thread.
 */
static void miapi_port_gattEvent(gattMsgEvent_t *pMsg)
{
  if (pMsg->method == ATT_HANDLE_VALUE_CFM)
  {
      mible_gatts_event_callback(MIBLE_GATTS_EVT_IND_CONFIRM, NULL);
  }
}

/* This function handle GAP event received from the Stack
 * No Memory are freed at this time, this needs to be done in the main application thread.
 */
static void miapi_port_gapEvent (gapEventHdr_t * data)
{
  if(port_object.appSync)
  {
      uint8_t event = data->opcode;
      switch (event)
      {
        case MI_APP_PORT_EVT_ADV_REPORT:
        {
          // not used yet... miapi_port_advReport(data);
          break;
        }
        case GAP_LINK_ESTABLISHED_EVENT:
        {
          gapEstLinkReqEvent_t *pPkt = (gapEstLinkReqEvent_t *)data;
          mible_gap_evt_param_t param;
          param.conn_handle = pPkt->connectionHandle;
          MIBLE_PORT_DBG0("Connection Established");
          if ( (pPkt->devAddrType == ADDRTYPE_PUBLIC) || (pPkt->devAddrType == ADDRTYPE_PUBLIC_ID))
          {
              param.connect.type = MIBLE_ADDRESS_TYPE_PUBLIC;
          }
          else
          {
              param.connect.type = MIBLE_ADDRESS_TYPE_RANDOM;
          }

          param.connect.role = (pPkt->connRole == GAP_PROFILE_CENTRAL?MIBLE_GAP_CENTRAL:MIBLE_GAP_PERIPHERAL);
          memcpy(param.connect.peer_addr, pPkt->devAddr, B_ADDR_LEN);
          param.connect.conn_param.min_conn_interval =  pPkt->connInterval;
          param.connect.conn_param.max_conn_interval = pPkt->connInterval;
          param.connect.conn_param.slave_latency = pPkt->connLatency;
          param.connect.conn_param.conn_sup_timeout = pPkt->connTimeout;
          mible_gap_event_callback(MIBLE_GAP_EVT_CONNECTED, &param);
          break;
        }
        case GAP_LINK_TERMINATED_EVENT:
        {
          gapTerminateLinkEvent_t *pPkt = (gapTerminateLinkEvent_t *)data;
          mible_gap_evt_param_t param; 
          param.conn_handle = pPkt->connectionHandle;
          MIBLE_PORT_DBG1("Connection Terminated, reason: %d", pPkt->reason);
          if (pPkt->reason == LL_STATUS_ERROR_PEER_TERM)
          {
              param.disconnect.reason = REMOTE_USER_TERMINATED;
          }
          else if (pPkt->reason == LL_STATUS_ERROR_CONNECTION_TIMEOUT)
          {
              param.disconnect.reason = CONNECTION_TIMEOUT;
          }
          else if(pPkt->reason == LL_STATUS_ERROR_HOST_TERM)
          {
              param.disconnect.reason = LOCAL_HOST_TERMINATED;
          }
          else
          {
              // Other reason
              param.disconnect.reason = (mible_gap_disconnect_reason_t)0xFF;
          }
          mible_gap_event_callback(MIBLE_GAP_EVT_DISCONNET, &param);
          break;
        }
        case GAP_LINK_PARAM_UPDATE_EVENT:
        {
          gapLinkUpdateEvent_t *pPkt = (gapLinkUpdateEvent_t *)data;
          if (pPkt->status == SUCCESS)
          {
            mible_gap_evt_param_t param;
            MIBLE_PORT_DBG0("Connection parameter updated");
            param.update_conn.conn_param.min_conn_interval =  pPkt->connInterval;
            param.update_conn.conn_param.max_conn_interval = pPkt->connInterval;
            param.update_conn.conn_param.slave_latency = pPkt->connLatency;
            param.update_conn.conn_param.conn_sup_timeout = pPkt->connTimeout;
            mible_gap_event_callback(MIBLE_GAP_EVT_CONN_PARAM_UPDATED, &param);
          }
          else
          {
            MIBLE_PORT_LOG_ERR0("Connection Parameter update failed");
          }
          break;
        }
        default:
          break;
      }
  }
}

static uint16_t miapi_port_GattTblFindServiceHdleByCharHandle(uint16_t char_handle)
{
  qServiceRec_t *elem;

  for ( elem = Queue_head(port_object.serviceQHdl);
        elem != (qServiceRec_t *)port_object.serviceQHdl;
        elem = Queue_next(&elem->_elem))
  {
     if( elem->pAttrSrvTbl->handle > char_handle )
     {
         // The characteristic is in the previous service...
         return(((qServiceRec_t*)Queue_prev(&elem->_elem))->pAttrSrvTbl->handle);
     }
  }
  if( elem->pAttrSrvTbl->handle < char_handle )
  {
      // The characteristic is in the last service...
      return(elem->pAttrSrvTbl->handle);
  }
  return(NULL);
}

static qServiceRec_t* miapi_port_GattTblFindServiceByHandle(uint16_t srv_handle)
{
  qServiceRec_t *elem;

  for ( elem = Queue_head(port_object.serviceQHdl);
        elem != (qServiceRec_t *)port_object.serviceQHdl;
        elem = Queue_next(&elem->_elem))
  {
     if( elem->pAttrSrvTbl->handle == srv_handle )
     {
         return(elem);
     }
  }
  return(NULL);
}

static qCharValueRec_t* miapi_port_GattTblFindCharValueByHandle(qServiceRec_t *srv_table, uint16_t value_handle)
{
  qCharValueRec_t *elem;

  for ( elem = Queue_head(srv_table->charValueQHdl);
        elem != (qCharValueRec_t *)srv_table->charValueQHdl;
        elem = Queue_next(&elem->_elem))
  {
     if( elem->handle == value_handle )
     {
         return(elem);
     }
  }
  return(NULL);
}

/*
 * @brief   Get BLE mac address.
 * @param   [out] mac: pointer to data
 * @return  MI_SUCCESS          The requested mac address were written to mac
 *          MI_ERR_INTERNAL     No mac address found.
 * @note:   You should copy gap mac to mac[6]  
 * */
mible_status_t mible_gap_address_get(mible_addr_t mac)
{
  uint8_t *macAddr;

  macAddr = GAP_GetDevAddress(TRUE) ;

  VOID memcpy(mac, macAddr, B_ADDR_LEN);
  MIBLE_PORT_DBG3("Identity Address: [%x:%x:%x:...]", macAddr[5], macAddr[4], macAddr[3]);
  MIBLE_PORT_DBG3("Identity Address: [...:%x:%x:%x]", macAddr[2], macAddr[1], macAddr[0]);
  
  return(MI_SUCCESS);
}

/*
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
  bStatus_t status = MI_SUCCESS;
  uint8_t tempPhy = SCAN_PRIM_PHY_1M;
  uint16_t tempField = (SCAN_ADVRPT_FLD_ADDRTYPE | SCAN_ADVRPT_FLD_ADDRESS);
  uint16_t typeFlt = SCAN_FLT_PDU_LEGACY_ONLY;
  uint8_t scanType = (scan_type == MIBLE_SCAN_TYPE_PASSIVE)? LL_SCAN_PASSIVE:SCAN_TYPE_ACTIVE;
  
  MIBLE_PORT_DBG4("Type: %d, interval: %d msec, window: %d msec, timeout: %d sec", scan_type, (uint32_t) (scan_param.scan_interval*0.625), (uint32_t)(scan_param.scan_window*0.625), (uint32_t)(scan_param.timeout*0.625));

  status = GapScan_registerCb(miapi_port_scanCb, NULL);

  GapScan_setEventMask(GAP_EVT_ADV_REPORT);


  status = GapScan_setPhyParams(SCAN_PRIM_PHY_1M, scanType,
                       scan_param.scan_interval, scan_param.scan_window);

  status = GapScan_setParam(SCAN_PARAM_RPT_FIELDS, &tempField);
  status = GapScan_setParam(SCAN_PARAM_PRIM_PHYS, &tempPhy);  
  status = GapScan_setParam(SCAN_PARAM_FLT_PDU_TYPE, &typeFlt);

  // Set LL Duplicate Filter
  //temp8 = SCAN_FLT_DUP_ENABLE;
  //status = GapScan_setParam(SCAN_PARAM_FLT_DUP, &temp8);
  
  status = GapScan_enable( 0, scan_param.timeout, 0 );
  MIBLE_PORT_ASSERT(status != SUCCESS);
  MIBLE_PORT_DBG1("Started, status: %d", status);
  return(miapi_port_translateError(status));
}


/*
 * @brief   Stop scanning
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped scanning procedure.
 *          MI_ERR_INVALID_STATE   Not in scanning state.
 * */
mible_status_t mible_gap_scan_stop(void)
{
  uint8_t status;

  status = GapScan_disable();
  MIBLE_PORT_ASSERT(status != SUCCESS);
  MIBLE_PORT_DBG0("disabled");
  return(miapi_port_translateError(status));
}

/*
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
mible_status_t mible_gap_adv_start(mible_gap_adv_param_t *p_adv_param)
{
  uint8_t status = SUCCESS;
  uint16_t type;
  uint8_t map = 0;
  MIBLE_PORT_DBG3("adv min: %d, adv max: %d, type: %d",
                  (uintptr_t) p_adv_param->adv_interval_min,
                  (uintptr_t) p_adv_param->adv_interval_max,
                  (uintptr_t) p_adv_param->adv_type);
  MIBLE_PORT_ASSERT(p_adv_param);

  switch(p_adv_param->adv_type)
  {
    case MIBLE_ADV_TYPE_CONNECTABLE_UNDIRECTED:
      type = GAP_ADV_PROP_SCANNABLE | GAP_ADV_PROP_CONNECTABLE | GAP_ADV_PROP_LEGACY;
      break;
    case MIBLE_ADV_TYPE_SCANNABLE_UNDIRECTED:
      type = GAP_ADV_PROP_SCANNABLE | GAP_ADV_PROP_LEGACY;
      break;
    case MIBLE_ADV_TYPE_NON_CONNECTABLE_UNDIRECTED:
      type = GAP_ADV_PROP_LEGACY;
      break;
    default:
      MIBLE_PORT_LOG_WAR0("invalid param! ");
      return(MI_ERR_INVALID_PARAM);
  }

  if(p_adv_param->ch_mask.ch_37_off == 0)
    map |= GAP_ADV_CHAN_37;

  if(p_adv_param->ch_mask.ch_38_off == 0)
    map |= GAP_ADV_CHAN_38;

  if(p_adv_param->ch_mask.ch_39_off == 0)
    map |= GAP_ADV_CHAN_39;

  status = GapAdv_setParam(port_object.advHandle, GAP_ADV_PARAM_PROPS, &type);
  STATUS_CHECK(miapi_port_translateError(status))
  {
    uint32_t interval = (uint32_t) p_adv_param->adv_interval_min;
    status = GapAdv_setParam(port_object.advHandle, GAP_ADV_PARAM_PRIMARY_INTERVAL_MIN, &interval);
    STATUS_CHECK(miapi_port_translateError(status))
    interval = (uint32_t) p_adv_param->adv_interval_max;
    status = GapAdv_setParam(port_object.advHandle, GAP_ADV_PARAM_PRIMARY_INTERVAL_MAX, &interval);
    STATUS_CHECK(miapi_port_translateError(status))
  }
  status = GapAdv_setParam(port_object.advHandle, GAP_ADV_PARAM_PRIMARY_CHANNEL_MAP, &map);
  STATUS_CHECK(miapi_port_translateError(status))

  status = GapAdv_enable(port_object.advHandle, GAP_ADV_ENABLE_OPTIONS_USE_MAX , 0);
  MIBLE_PORT_ASSERT(status == SUCCESS);
  MIBLE_PORT_DBG1("enabled (status : %d)", status);
  return(miapi_port_translateError(status));
}

/*
 * @brief   Config advertising data
 * @param   [in] p_data : Raw data to be placed in advertising packet. If NULL, no changes are made to the current advertising packet.
 * @param   [in] dlen   : Data length for p_data. Max size: 31 octets. Should be 0 if p_data is NULL, can be 0 if p_data is not NULL.
 * @param   [in] p_sr_data : Raw data to be placed in scan response packet. If NULL, no changes are made to the current scan response packet data.
 * @param   [in] srdlen : Data length for p_sr_data. Max size: BLE_GAP_ADV_MAX_SIZE octets. Should be 0 if p_sr_data is NULL, can be 0 if p_data is not NULL.
 * @return  MI_SUCCESS             Successfully set advertising data.
 *          MI_ERR_INVALID_ADDR    Invalid pointer supplied.
 *          MI_ERR_INVALID_PARAM   Invalid parameter(s) supplied.
 * */   
mible_status_t mible_gap_adv_data_set(uint8_t const * p_data, uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen)
{
  bStatus_t status = 0;
  MIBLE_PORT_DBG2("New Adv Data, len: %d,  New Scan Rsp data len: %d", dlen, srdlen);
    
  MIBLE_PORT_ASSERT(dlen < 32);
  MIBLE_PORT_ASSERT(srdlen < 32);

  if (p_data)
  {
    MIBLE_PORT_DBG0("Updating Adv Data...");
    status = GapAdv_prepareLoadByHandle(port_object.advHandle, GAP_ADV_FREE_OPTION_ADV_DATA);
    MIBLE_PORT_ASSERT(status == SUCCESS);
    if (dlen)
    {
      if(!(port_object.advPayload = (uint8_t*) ICall_mallocLimited(dlen)))
      {
        return(MI_ERR_NO_MEM);
      }
      memcpy(port_object.advPayload, p_data, dlen);
    }
    else
    {
      MIBLE_PORT_DBG0("emptying Adv Data...");
      port_object.advPayload = NULL;
    }
    status = GapAdv_loadByHandle(port_object.advHandle, GAP_ADV_DATA_TYPE_ADV,
                                 dlen, port_object.advPayload);
    MIBLE_PORT_ASSERT(status == SUCCESS);
  }

  if (p_sr_data)
  {
    MIBLE_PORT_DBG0("Updating Scan rsp Data...");
    if (port_object.advSrPayload)
    {
        status = GapAdv_prepareLoadByHandle(port_object.advHandle, GAP_ADV_FREE_OPTION_SCAN_RESP_DATA);
        MIBLE_PORT_ASSERT(status == SUCCESS);
    }
    if (srdlen)
    {
      if(!(port_object.advSrPayload = (uint8_t*) ICall_mallocLimited(srdlen)))
      {
        return(MI_ERR_NO_MEM);
      }
      memcpy(port_object.advSrPayload, p_sr_data, srdlen);
    }
    else
    {
      MIBLE_PORT_DBG0("emptying Scan Rsp Data...");
      port_object.advSrPayload = NULL;
    }
    status = GapAdv_loadByHandle(port_object.advHandle, GAP_ADV_DATA_TYPE_SCAN_RSP,
                                 srdlen, port_object.advSrPayload);
    MIBLE_PORT_ASSERT(status == SUCCESS);
  }

  MIBLE_PORT_DBG0("Update(s) Done...");
  if (status == SUCCESS)
    return(MI_SUCCESS);
  else
    MIBLE_PORT_LOG_WAR0("invalid param! ");
    return(MI_ERR_INVALID_PARAM);
}

/*
 * @brief   Stop advertising
 * @param   void
 * @return  MI_SUCCESS             Successfully stopped advertising procedure.
 *          MI_ERR_INVALID_STATE   Not in advertising state.
 * */
mible_status_t mible_gap_adv_stop(void)
{
  bStatus_t status = SUCCESS;

  status = GapAdv_disable(port_object.advHandle);
  MIBLE_PORT_DBG1("adv stopped...status: %d", status);
  return(miapi_port_translateError(status));
}


/*
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
 * @note    Own and peer address are both public.
 *          The connection result is given by MIBLE_GAP_EVT_CONNECTED
 * event
 * */
mible_status_t mible_gap_connect(mible_gap_scan_param_t scan_param,
    mible_gap_connect_t conn_param)
{
  // TODO with scanning
  return MI_SUCCESS;
}


/*
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
  uint8_t status = GAP_TerminateLinkReq( conn_handle, HCI_DISCONNECT_REMOTE_USER_TERM );
  MIBLE_PORT_DBG2("Terminating Connection: hdl: %d , status: %d", conn_handle, status);
  if (status != SUCCESS)
  {
    return(MIBLE_ERR_INVALID_CONN_HANDLE);
  }
  return MI_SUCCESS;
}

/*
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
  gapUpdateLinkParamReq_t req;
  MIBLE_PORT_DBG4("Requesting Connection Update: hdl: %d , Timeout: %d Min/Max: %d/%d",
                  (uintptr_t) conn_handle,
                  (uintptr_t) conn_params.conn_sup_timeout,
                  (uintptr_t) conn_params.min_conn_interval,
                  (uintptr_t) conn_params.max_conn_interval);

  req.connectionHandle = conn_handle;
  req.connLatency = conn_params.slave_latency;
  req.connTimeout = conn_params.conn_sup_timeout;;
  req.intervalMin = conn_params.min_conn_interval;;
  req.intervalMax = conn_params.max_conn_interval;;

  // Send parameter update
  bStatus_t status = GAP_UpdateLinkParamReq(&req);
  if(status == bleAlreadyInRequestedMode)
  {
    MIBLE_PORT_LOG_WAR0("invalid param or already performing an equivalent request...");
    return MI_ERR_INVALID_STATE;
  }
  MIBLE_PORT_DBG0("request sent");
  return MI_SUCCESS;
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
  mible_status_t status;
  MIBLE_PORT_DBG1("Adding %d service(s)", p_server_db->srv_num);
  for (uint8_t srvIdx = 0; srvIdx < p_server_db->srv_num; srvIdx++)
  {
    status = miapi_port_GattTblConvert((p_server_db->p_srv_db) + srvIdx);
  }

  mible_arch_evt_param_t param;
  param.srv_init_cmp.status = status;
  param.srv_init_cmp.p_gatts_db = p_server_db;
  mible_arch_event_callback(MIBLE_ARCH_EVT_GATTS_SRV_INIT_CMP, &param);
  return(MI_SUCCESS);
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
 *          MIBLE_ERR_ATT_INVALID_ATT_HANDLE     Attribute not found.
 *          MIBLE_ERR_GATT_INVALID_ATT_TYPE  Attributes are not modifiable by
 *the application.
 * */
mible_status_t mible_gatts_value_set(uint16_t srv_handle,
        uint16_t value_handle, uint8_t offset, uint8_t* p_value, uint8_t len)
{
  qCharValueRec_t *pChar;
  qServiceRec_t *pService;

  MIBLE_PORT_ASSERT(p_value);

  //MIBLE_PORT_DBG3("Set Char Value, handle: %d , offset: %d, len: %d ", value_handle, offset, len);

  pService = miapi_port_GattTblFindServiceByHandle(srv_handle);
  if(!pService)
  {
    MIBLE_PORT_LOG_WAR1("service not found: %d", srv_handle);
    return(MIBLE_ERR_ATT_INVALID_ATT_HANDLE);
  }

  pChar = miapi_port_GattTblFindCharValueByHandle(pService, value_handle);
  if(!pChar)
  {
    // It is possible that an attribute other than a characteristic value is modify.
    // However, the function explicitly indicate that this function is only to set characteristic value.
    MIBLE_PORT_ASSERT(pChar);
    MIBLE_PORT_LOG_WAR0("Characteristic Value not found");
    return(MIBLE_ERR_ATT_INVALID_ATT_HANDLE);
  }

  if (len > pChar->maxLen)
  {
    MIBLE_PORT_LOG_WAR1("Wrong len, bigger than Char Value defined length: %d", pChar->maxLen);
    return(MI_ERR_INVALID_LENGTH);
  }

  if ((len+offset) > pChar->maxLen)
  {
    MIBLE_PORT_LOG_WAR1("Wrong combination of len and offset, bigger than Char Value defined length: %d", pChar->maxLen);
    return(MI_ERR_INVALID_PARAM);
  }

  if(pChar->flag.is_variable_len)
  {
    pChar->len = offset + len;
  }
  // Now set the value.
  memcpy(pChar->pAttr->pValue + offset, p_value, len);
  //MIBLE_PORT_DBG0("Char Value updated");
  return(MI_SUCCESS);
}

/**
 * @brief   Get characteristic value as a GATTS.
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
        uint16_t value_handle, uint8_t* p_value, uint8_t *p_len)
{
  qCharValueRec_t *pChar;
  qServiceRec_t *pService;

  MIBLE_PORT_ASSERT(p_len);
  //MIBLE_PORT_DBG1("Get Char Value, handle: %d", value_handle);

  pService = miapi_port_GattTblFindServiceByHandle(srv_handle);
  if(!pService)
  {
    MIBLE_PORT_LOG_WAR1("service not found: %d", srv_handle);
    return(MIBLE_ERR_ATT_INVALID_ATT_HANDLE);
  }

  pChar = miapi_port_GattTblFindCharValueByHandle(pService, value_handle);
  MIBLE_PORT_ASSERT(pChar);
  if(!pChar)
  {
    // It is possible that an attribute other than a characteristic value is modify.
    // However, the function explicitly indicate that this function is only to set characteristic value.
    MIBLE_PORT_LOG_WAR0("Characteristic Value not found");
    return(MIBLE_ERR_ATT_INVALID_ATT_HANDLE);
  }

  // The Value will be set only if p_value is not NULL.
  // If p_value is NULL, only the lenght of the characteristic will be set.
  // This allows to get the length of the characteristic value without knowing it in advance.
  if(p_value)
  {
      // Now set the value.
      memcpy(p_value, pChar->pAttr->pValue, pChar->len);
  }
  *p_len = pChar->len;
  //MIBLE_PORT_DBG0("Char Value retrieved");
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
        uint8_t* p_value, uint8_t len, uint8_t type)
{
  qCharValueRec_t *pChar;
  qServiceRec_t *pService;
  uint8_t status;
  uint16_t CCCValue;
  MIBLE_PORT_DBG3("Ind/Notif: Conn: %d, Hdl: %d, type: %d", conn_handle, char_value_handle, type);
  MIBLE_PORT_ASSERT(p_value);
  
  status = mible_gatts_value_set(srv_handle, char_value_handle, offset, p_value, len);
  MIBLE_PORT_ASSERT(status == MI_SUCCESS);

  pService = miapi_port_GattTblFindServiceByHandle(srv_handle);
  if(!pService)
  {
    MIBLE_PORT_LOG_WAR1("service not found: %d", srv_handle);
    return(MIBLE_ERR_ATT_INVALID_ATT_HANDLE);
  }
  pChar = miapi_port_GattTblFindCharValueByHandle(pService, char_value_handle);
  if(!pChar)
  {
    // It is possible that an attribute other than a characteristic value is modify.
    // However, the function explicitly indicate that this function is only to set
    // characteristic value (so NOT other attribute value).
    MIBLE_PORT_ASSERT(pChar);
    MIBLE_PORT_LOG_WAR0("Characteristic Value not found");
    return(MIBLE_ERR_ATT_INVALID_ATT_HANDLE);
  }

  CCCValue  = GATTServApp_ReadCharCfg(conn_handle, GATT_CCC_TBL(pChar->pCCCValue));

  if (CCCValue != GATT_CFG_NO_OPERATION)
  {
    //if (CharAttrib != NULL)
    {
      if ( !( (CCCValue & GATT_CLIENT_CFG_NOTIFY ) && (type == 1)) &&
           !( (CCCValue & GATT_CLIENT_CFG_INDICATE) && (type == 2)))
      {
        MIBLE_PORT_LOG_WAR1("Ind/Notif of type: %d is not enable for this characteristic value", type);
        return(MI_ERR_INVALID_STATE);
      }
      miapi_port_SendNotiInd(conn_handle, type, pChar->pAttr, miapi_port_ReadAttrCB);
    }
  }
  else
  {
    MIBLE_PORT_LOG_WAR0("neither Indication or notification enable for this characteristic value");
    return(MI_ERR_INVALID_STATE);
  }

  MIBLE_PORT_DBG1("Ind/Notif: sent with status: %d", status);
  return(miapi_port_translateError(status));
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
 *          MIBLE_ERR_ATT_INVALID_ATT_HANDLE     Attribute not found.
 * @note    This call should only be used as a response to a MIBLE_GATTS_EVT_READ/WRITE_PERMIT_REQ
 * event issued to the application.
 * */
mible_status_t mible_gatts_rw_auth_reply(uint16_t conn_handle,
        uint8_t status, uint16_t char_value_handle, uint8_t offset,
        uint8_t* p_value, uint8_t len, uint8_t type)
{
  mible_status_t res;
  MIBLE_PORT_DBG4("Conn: %d, status: %d, char hdl: %d, type: %d", conn_handle, status, char_value_handle, type);

  if((port_object.ATTop.flag.validity) && (port_object.ATTop.flag.authorNeeded))
  {
    if(status == 0)
    {
      attErrorRsp_t attRsp;
      attRsp.reqOpcode = port_object.ATTop.method;
      attRsp.handle = port_object.ATTop.pAttr->handle;
      attRsp.errCode = (type == 1)?ATT_ERR_READ_NOT_PERMITTED:ATT_ERR_WRITE_NOT_PERMITTED;
      ATT_ErrorRsp(port_object.ATTop.connHandle, &attRsp);
      port_object.ATTop.flag.validity = 0;
      return(MI_SUCCESS);
    }
    else
    {
        uint16_t srvHdl = miapi_port_GattTblFindServiceHdleByCharHandle(char_value_handle);
        MIBLE_PORT_ASSERT(srvHdl);
        if(!srvHdl) return(MIBLE_ERR_ATT_INVALID_ATT_HANDLE);
        if(!p_value) return(MI_ERR_INVALID_ADDR);

        port_object.ATTop.flag.authorNeeded = 0;
        port_object.ATTop.flag.validity = 0;
        res = mible_gatts_value_set(srvHdl, char_value_handle, offset, p_value, len); 
        if (res != MI_SUCCESS)
        {
            MIBLE_PORT_DBG1("Failed to set the new value, status: %d", res);
            return(res);
        }

        switch(port_object.ATTop.method)
        {
          case ATT_READ_REQ:
          {
            attReadRsp_t rsp;
            uint16_t datasize;
            uint8_t char_len;

            // The length of the characteristic needs to be retrieved, since the length provided correspond
            // to a modification of the characteristic (offset can be different than 0), and not necessarily to the full length of the characteristic.
            mible_gatts_value_get(srvHdl, char_value_handle, NULL, &char_len);

            datasize = (char_len > port_object.ATTop.maxLen)?port_object.ATTop.maxLen:char_len;
            rsp.pValue = (uint8_t *)GATT_bm_alloc(port_object.ATTop.connHandle,
                                                  ATT_READ_RSP,
                                                  datasize, NULL);
            if (rsp.pValue)
            {
              rsp.len = datasize;
              uint16_t charValueLen;

              bStatus_t status = miapi_port_ReadAttrValue(port_object.ATTop.pAttr, rsp.pValue, &charValueLen, port_object.ATTop.offset, port_object.ATTop.maxLen);
              MIBLE_PORT_ASSERT(status==SUCCESS);
              if (SUCCESS != status)
              {
                  MIBLE_PORT_LOG_WAR0("can't read the characteristic value");
                  GATT_bm_free((gattMsg_t *)rsp.pValue, ATT_READ_RSP);
                  return(MI_ERR_INVALID_PARAM);
              }
              if(ATT_ReadRsp(conn_handle , &rsp) != SUCCESS)
              {
                MIBLE_PORT_LOG_WAR0("not enough memory");
                GATT_bm_free((gattMsg_t *)rsp.pValue, ATT_READ_RSP);
                return(MI_ERR_NO_MEM);
              }
            }
            else
            {
              MIBLE_PORT_LOG_WAR0("not enough memory");
              return(MI_ERR_NO_MEM);
            }
            res = MI_SUCCESS;
          }
          break;
          case ATT_WRITE_REQ:
          {
            ATT_WriteRsp(conn_handle);
          }
          break;
          case ATT_READ_BLOB_REQ:
          // Read blob is reading N bytes from Offset O.
          // Offset Value needs to be taken into account!
          {
            attReadRsp_t rsp;
            uint16_t datasize;
            uint8_t char_len;

            // The length of the characteristic needs to be retrieved, since the length provided correspond
            // to a modification of the characteristic (offset can be different than 0), and not necessarily to the full length of the characteristic.
            // and Read Blob also can have an offset different than 0.
            mible_gatts_value_get(srvHdl, char_value_handle, NULL, &char_len);
            datasize = ((char_len-port_object.ATTop.offset) > port_object.ATTop.maxLen)?port_object.ATTop.maxLen:(char_len-port_object.ATTop.offset);
            rsp.pValue = (uint8_t *)GATT_bm_alloc(port_object.ATTop.connHandle,
                                                  ATT_READ_BLOB_RSP,
                                                  datasize, NULL);
            if (rsp.pValue)
            {
              rsp.len = datasize;
              uint16_t charValueLen;

              bStatus_t status = miapi_port_ReadAttrValue(port_object.ATTop.pAttr, rsp.pValue, &charValueLen, port_object.ATTop.offset, port_object.ATTop.maxLen);
              MIBLE_PORT_ASSERT(status==SUCCESS);
              if (SUCCESS != status)
              {
                  MIBLE_PORT_LOG_WAR0("can't read the characteristic value");
                  GATT_bm_free((gattMsg_t *)rsp.pValue, ATT_READ_BLOB_RSP);
                  return(MI_ERR_INVALID_PARAM);
              }
              if(ATT_ReadRsp(conn_handle , &rsp) != SUCCESS)
              {
                MIBLE_PORT_LOG_WAR0("not enough memory");
                GATT_bm_free((gattMsg_t *)rsp.pValue, ATT_READ_BLOB_RSP);
                return(MI_ERR_NO_MEM);
              }
            }
            else
            {
              MIBLE_PORT_LOG_WAR0("not enough memory");
              return(MI_ERR_NO_MEM);
            }
            res = MI_SUCCESS;
          }

          case ATT_READ_BY_TYPE_REQ:
#ifdef ATT_DELAYED_REQ
          {
            uint16_t datasize;
            uint8_t char_len;
            mible_gatts_value_get(srvHdl, char_value_handle, NULL, &char_len);
            datasize = ((char_len-port_object.ATTop.offset) > port_object.ATTop.maxLen)?port_object.ATTop.maxLen:(char_len-port_object.ATTop.offset);
            uint8_t *pValue = (uint8_t *)ICall_mallocLimited(char_len);
            if (pValue)
            {
              uint16_t charValueLen;
              bStatus_t status = miapi_port_ReadAttrValue(port_object.ATTop.pAttr, pValue, &charValueLen, port_object.ATTop.offset, port_object.ATTop.maxLen);
              MIBLE_PORT_ASSERT(status==SUCCESS);
              if (SUCCESS != status)
              {
                MIBLE_PORT_LOG_WAR0("can't read the characteristic value");
                // Pass delayed response from Application Processor to GATT
                // Server to build Read By Type Response
                ICall_free(pValue);
                return(MI_ERR_INVALID_PARAM);
              }
              GATTServApp_ReadRsp(conn_handle, datasize, datasize, port_object.ATTop.pAttr->handle);
              ICall_free(pValue);
            }
            else
            {
                MIBLE_PORT_LOG_WAR0("not enough memory");
                return(MI_ERR_NO_MEM);
            }
            res = MI_SUCCESS;
          }
          break;
#endif // ATT_DELAYED_REQ
          case ATT_EXECUTE_WRITE_REQ:
          case ATT_PREPARE_WRITE_REQ:
          case ATT_WRITE_CMD:
          case ATT_SIGNED_WRITE_CMD:
          default:
              MIBLE_PORT_LOG_WAR1("this method is not supported with authorization: %d", port_object.ATTop.method);
              res = MI_ERR_BUSY;
            break;
        }
    }
    MIBLE_PORT_DBG0("Done");
    return(res);
  }
  else
  {
    MIBLE_PORT_LOG_WAR0("Invalid State, no ATT request pending, or authorization not needed");
    return(MI_ERR_INVALID_STATE);
  }
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
mible_status_t mible_gattc_primary_service_discover_by_uuid(
        uint16_t conn_handle, mible_handle_range_t handle_range,
        mible_uuid_t* p_srv_uuid)
{
  // TODO GATT Client
  return(MI_SUCCESS);
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
        mible_handle_range_t handle_range, mible_uuid_t* p_char_uuid)
{
  // TODO GATT Client
  return(MI_SUCCESS);
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
mible_status_t mible_gattc_clt_cfg_descriptor_discover(
        uint16_t conn_handle, mible_handle_range_t handle_range)
{
  // TODO GATT Client
  return(MI_SUCCESS);
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
mible_status_t mible_gattc_read_char_value_by_uuid(uint16_t conn_handle,
        mible_handle_range_t handle_range, mible_uuid_t *p_char_uuid)
{
  // TODO GATT Client
  return(MI_SUCCESS);
}

/**
 * @brief   Write value by handle with response
 * @param   [in] conn_handle: connection handle
 *          [in] handle: handle to the attribute to be written.
 *          [in] p_value: pointer to data
 *          [in] len: data length
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
mible_status_t mible_gattc_write_with_rsp(uint16_t conn_handle,
        uint16_t att_handle, uint8_t* p_value, uint8_t len)
{
  // TODO GATT Client
  return(MI_SUCCESS);
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
  // TODO GATT Client
  return(MI_SUCCESS);
}

/**
 *        SOFT TIMER APIs
 */

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
  uint32_t period = 0;
  timer_t *timer;
  MIBLE_PORT_DBG2("mode: %d , handler: %p", mode, (uintptr_t) timeout_handler);

  if (mode == MIBLE_TIMER_REPEATED)
  {
    period = 1;        // set to a value different than 0 to indicate a periodic timer
  }
  
  MIBLE_PORT_LOG_HEAP();
  timer = (timer_t *)ICall_mallocLimited(sizeof(timer_t));
  MIBLE_PORT_LOG_HEAP();
  if (timer)
  {
    Util_constructClock(&timer->clock, (Clock_FuncPtr)miapi_port_timer_cb, 0, period, false, (uintptr_t) timer);
    *p_timer_id = timer;
    timer->cb = timeout_handler;
  }
  else
  {
    MIBLE_PORT_LOG_ERR0("create, no memory left");
    MIBLE_PORT_LOG_HEAP();
    MIBLE_PORT_ASSERT(timer);
    return(MI_ERR_NO_MEM);
  }
  MIBLE_PORT_DBG1("Created, %p",  (uintptr_t) *p_timer_id);
  return(MI_SUCCESS);
}

/**
 * @brief   Delete a timer.
 * @param   [in] timer_id: timer id
 * @return  MI_SUCCESS             If the timer was successfully deleted.
 *          MI_ERR_INVALID_PARAM   Invalid timer id supplied..
 * */
 mible_status_t mible_timer_delete(void* timer_id)
{
  timer_t * timer = (timer_t *) timer_id;
  MIBLE_PORT_DBG1("delete, %p",  (uintptr_t) timer);
  if(timer == NULL)
  {
    MIBLE_PORT_LOG_WAR1("invalid object %p",  (uintptr_t) timer);
    return(MI_ERR_INVALID_PARAM);
  }
  Clock_destruct(&timer->clock);
  MIBLE_PORT_LOG_HEAP();
  if (timer)
  {
    ICall_free(timer);
  }
  MIBLE_PORT_LOG_HEAP();
  MIBLE_PORT_DBG1("%p  deleted",  (uintptr_t) timer);
  return(MI_SUCCESS);
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
  timer_t * timer = (timer_t*) timer_id;
  Clock_Handle timerHdl = Clock_handle(&timer->clock);
  uint32_t clockTicks;
  MIBLE_PORT_DBG3("start %p, timeout: %d  ms, context: %p",  (uintptr_t) timer, timeout_value, (uintptr_t) p_context);

  if((timer == NULL) || (timeout_value < 10))
  {
      MIBLE_PORT_LOG_WAR0("invalid param! ");
      return(MI_ERR_INVALID_PARAM);
  }

  Clock_stop(timerHdl);

  // Convert timeout in milliseconds to ticks.
  clockTicks = timeout_value * (1000 / Clock_tickPeriod);
  Clock_setTimeout(timerHdl, clockTicks);

  timer->arg = p_context;

  // check period to see whether it is a period or one-shot timer, and set corresponding value
  if(Clock_getPeriod(timerHdl))
  {
    Clock_setPeriod(timerHdl, clockTicks);
  }

  Clock_start(timerHdl);
  MIBLE_PORT_DBG1("%p started",  (uintptr_t) timer);
  return(MI_SUCCESS);
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
  timer_t* timer = (timer_t*) timer_id;
  Clock_Handle timerHdl = Clock_handle(&timer->clock);
  MIBLE_PORT_DBG1("timer_id: %p  ",  (uintptr_t) timer);
  if(timer == NULL)
  {
    MIBLE_PORT_LOG_WAR1("invalid object %p  ",  (uintptr_t) timer);
    return(MI_ERR_INVALID_PARAM);
  }

  Clock_stop(timerHdl);
  MIBLE_PORT_DBG1("%p stopped",  (uintptr_t) timer);
  return(MI_SUCCESS);
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
  NVINTF_itemID_t id;
  uint8_t dummy_buf[1];
  MIBLE_PORT_DBG2("creating record id: %d, len: %d", record_id, len);
#ifndef CC26X2
  if(record_id > 0x0F)
   {
       return MI_ERR_NO_MEM;
   }


   NV_bitfield |= (1 << record_id);
#endif // !CC26X2
  if((len == 0) || (len > MIBLE_NV_RECORD_MAX_SIZE))
  {
       return MI_ERR_INVALID_LENGTH;
  }
  id.systemID = MIBLE_SYSTEM;
  id.itemID = record_id;
  id.subID = 0;
  if (MI_SUCCESS != port_object.NVpfn.writeItem(id, len, dummy_buf))
  {
    MIBLE_PORT_LOG_WAR0("invalid parameter");
    return(MI_ERR_INVALID_PARAM);
  }
  MIBLE_PORT_DBG1("record id %d created", record_id);
  return(MI_SUCCESS);
}

/**
 * @brief   Delete a record in flash
 * @param   [in] record_id: identify a record in flash  
 * @return  MI_SUCCESS              Delete successfully. 
 *          MI_ERR_INVALID_PARAMS   Invalid record id supplied.
 * */
mible_status_t mible_record_delete(uint16_t record_id)
{
#ifdef CC26X2
  MIBLE_PORT_DBG1("deleting record id %d", record_id);
  NVINTF_itemID_t id;
  id.systemID = MIBLE_SYSTEM;
  id.itemID = record_id;
  id.subID = 0;
  if (MI_SUCCESS != port_object.NVpfn.deleteItem(id))
  {
    MIBLE_PORT_LOG_WAR0("invalid parameter");
    return(MI_ERR_INVALID_PARAM);
  }
  mible_arch_evt_param_t param;
  param.record.id = record_id;
  param.record.status = MI_SUCCESS;
  mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE, &param);
  MIBLE_PORT_DBG1("record id %d deleted", record_id);
  return(MI_SUCCESS);
#else
  MIBLE_PORT_DBG1("deleting record id %d", record_id);
  // Item cannot be deleted, but they can be re-used...
  // For chameleon, only Allow 16 Entries...
  if((NV_bitfield & (1 << record_id)) == 0)
  {
      return MI_ERR_INVALID_PARAM;
  }
  else
  {
      NV_bitfield &= ~(1 << record_id);
  }

  mible_arch_evt_param_t param;
  param.record.id = record_id;
  param.record.status = MI_SUCCESS;
  mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE, &param);
  MIBLE_PORT_DBG1("record id %d deleted", record_id);
  return(MI_SUCCESS);
#endif
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
mible_status_t mible_record_read(uint16_t record_id, uint8_t *p_data,
        uint8_t len)
{
  NVINTF_itemID_t id;
  uint8_t res;
  MIBLE_PORT_DBG1("reading record id %d", record_id);
  MIBLE_PORT_ASSERT(p_data);

#ifndef CC26X2
  if(record_id > 0x0F)
  {
      return MI_ERR_INVALID_PARAM;
  }
#endif // !CC26x2
  id.systemID = MIBLE_SYSTEM;
  id.itemID = record_id;
  id.subID = 0;

  res = port_object.NVpfn.readItem(id, 0, len, p_data);
  if ((len == 0) || (res == NVINTF_BADLENGTH) || (len > MIBLE_NV_RECORD_MAX_SIZE))
  {
    MIBLE_PORT_LOG_WAR0("invalid length");
    return(MI_ERR_INVALID_LENGTH);
  }
  else if (res != MI_SUCCESS)
  {
    MIBLE_PORT_LOG_WAR2("fail to read record id %d, status: %d", record_id, res);
    return(MI_ERR_INVALID_ADDR);
  }
  MIBLE_PORT_DBG1("record id: %d  read", record_id);
  return(MI_SUCCESS);
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
  NVINTF_itemID_t id;
  uint8_t res;

  MIBLE_PORT_DBG2("Writing record id: %d, len: %d", record_id, len);
  MIBLE_PORT_ASSERT(p_data);

#ifndef CC26X2
  if(record_id > 0x0F)
  {
      return MI_ERR_INVALID_PARAM;
  }
#endif // ! CC26X2

  id.systemID = MIBLE_SYSTEM;
  id.itemID = record_id;
  id.subID = 0;
  port_object.NVpfn.compactNV(len);
  res = port_object.NVpfn.writeItem(id, len, p_data);
  if ((len == 0) || (res == NVINTF_BADLENGTH) || (len > MIBLE_NV_RECORD_MAX_SIZE))
  {
    MIBLE_PORT_LOG_WAR0("invalid length");
    return(MI_ERR_INVALID_LENGTH);
  }
  else if (res != SUCCESS)
  {
      MIBLE_PORT_LOG_WAR2("fail to write record id %d, status: %d", record_id, res);
    return(MI_ERR_INVALID_PARAM);
  }
  port_object.NVpfn.compactNV(128);
  MIBLE_PORT_DBG1("recod id %d  written", record_id);
  mible_arch_evt_param_t param;
  param.record.id = record_id;
  param.record.status = MI_SUCCESS;
  mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);
  return(MI_SUCCESS);
}

/**
 *        MISC APIs
 */

/**
 * @brief   Get ture random bytes .
 * @param   [out] p_buf: pointer to data
 *          [in] len: Number of bytes to take from pool and place in
 * p_buff
 * @return  MI_SUCCESS          The requested bytes were written to
 * p_buff
 *          MI_ERR_NO_MEM       No bytes were written to the buffer, because
 * there were not enough random bytes available.
 * @note    SHOULD use TRUE random num generator
 * */
mible_status_t mible_rand_num_generator(uint8_t* p_buf, uint8_t len)
{
    uint32_t num = 0;
    uint8_t i;
    MIBLE_PORT_DBG1("len: %d", len);
    MIBLE_PORT_ASSERT(p_buf);

    for(i = 0; i < (len>>2); i++)
    {
        num = TRNGCC26XX_getNumber(NULL, NULL, NULL);
        *((uint32_t *)p_buf) = num;
    }

    if(len & 0x11)
    {
        num = TRNGCC26XX_getNumber(NULL, NULL, NULL);
        switch(len & 0x11)
        {
            case 1:
                *(p_buf + i * 4) = (uint8_t)(num);
                break;
            case 2:
                *(p_buf + i * 4)     = (uint8_t)(num >> 8);
                *(p_buf + i * 4 + 1) = (uint8_t)(num);
                break;
            case 3:
                *(p_buf + i * 4)     = (uint8_t)(num >> 16);
                *(p_buf + i * 4 + 1) = (uint8_t)(num >> 8);
                *(p_buf + i * 4 + 2) = (uint8_t)(num);
                break;
            default:
                break;
        }
    }
    MIBLE_PORT_DBG1("%d bytes generated", len);
    MIBLE_PORT_ASSERT(p_buf);
    return(MI_SUCCESS);
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
        const uint8_t* plaintext, uint8_t plen, uint8_t* ciphertext)
{
#ifdef CC26X2

    // data needed for TI crypto driver for AESECB
    AESECB_Handle    encHandleECB;
    AESECB_Params    encParamsECB;
    AESECB_Operation operationECB;
    CryptoKey        cryptoKey;
    uint32_t resEncrypt;

    MIBLE_PORT_DBG4("Key: [%d:%d:%d:%d...], ", key[0], key[1], key[2], key[3] );
    MIBLE_PORT_DBG2("Plain: Len: %d , [%d:...], ", plen, plaintext[0] );
    MIBLE_PORT_ASSERT(ciphertext);
    /* Initialize both AESCCM objects as BLE stack is using both */
    AESECB_init();

    AESECB_Params_init( &encParamsECB );
    encParamsECB.returnBehavior = AESECB_RETURN_BEHAVIOR_POLLING;
    encHandleECB = AESECB_open(0, &encParamsECB);
    MIBLE_PORT_ASSERT(encHandleECB);

    AESECB_Operation_init(&operationECB);

    CRITICAL_SECTION_ENTER();

    // The cryptoKey is initialized already, just update the key material
    //MAP_osal_memcpy(cryptoKey.u.plaintext.keyMaterial, key, ENC_KEY_LEN);
    CryptoKeyPlaintext_initKey(&cryptoKey, (uint8_t*) key, sizeof(key));
    operationECB.key               = &cryptoKey;
    operationECB.input             = (uint8_t*) plaintext;
    operationECB.output            = ciphertext;
    operationECB.inputLength       = plen;

    if((resEncrypt = AESECB_oneStepEncrypt(encHandleECB, &operationECB) != AESECB_STATUS_SUCCESS))
    {
        MIBLE_PORT_ASSERT(resEncrypt == AESECB_STATUS_SUCCESS);
    }
    CRITICAL_SECTION_EXIT();
#else // !CC26X2
	CryptoCC26XX_Handle             handle;
	int32_t                         keyIndex;
	CryptoCC26XX_AESECB_Transaction trans;
	
  MIBLE_PORT_DBG4("Key: [%d:%d:%d:%d...], ", key[0], key[1], key[2], key[3] );
  MIBLE_PORT_DBG2("Plain: Len: %d , [%d:...], ", plen, plaintext[0] );
  MIBLE_PORT_ASSERT(ciphertext);
	CryptoCC26XX_init();
	
	handle = CryptoCC26XX_open(Board_CRYPTO0, false, NULL);
		
	keyIndex = CryptoCC26XX_allocateKey(handle, CRYPTOCC26XX_KEY_ANY, (const uint32_t *) key);
		
  CRITICAL_SECTION_ENTER();
	CryptoCC26XX_Transac_init((CryptoCC26XX_Transaction *) &trans, CRYPTOCC26XX_OP_AES_ECB_ENCRYPT);
	
	trans.keyIndex         = keyIndex;
	trans.msgIn            = (uint8_t *) plaintext;
	trans.msgOut           = (uint8_t *) ciphertext;
	
	CryptoCC26XX_transact(handle, (CryptoCC26XX_Transaction *) &trans);
  
  CRITICAL_SECTION_EXIT();
	
	CryptoCC26XX_releaseKey(handle, &keyIndex);
	
	CryptoCC26XX_close(handle);

#endif // CC26X2

  MIBLE_PORT_DBG1("ciphertext: [%d:...], ", ciphertext[0] );
  return(MI_SUCCESS);

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
mible_status_t mible_task_post(mible_handler_t handler, void *arg)
{
  qTaskRec_t *pRec;
  // Add Job to the list...
  // Allocated space for queue node.
  if ((pRec = (qTaskRec_t *)ICall_mallocLimited(sizeof(qTaskRec_t))))
  {
    pRec->handler = handler;
    pRec->arg = arg;

    Queue_put(port_object.taskQHdl, &pRec->_elem);

    // @to do, go through util function
    Event_post(port_object.appSync, Event_Id_30);

    return(MI_SUCCESS);
  }  
  return(MI_ERR_NO_MEM);
}

/*
 * @brief   Function for executing all enqueued tasks.
 *
 * @note    This function must be called from within the main loop. It will 
 * execute all events scheduled since the last time it was called.
 * */
void mible_tasks_exec(void)
{
  if (port_object.taskQHdl)
  {
      qTaskRec_t *pRec = Queue_get(port_object.taskQHdl);

      while (pRec != (qTaskRec_t *)port_object.taskQHdl)
      {
        //MIBLE_PORT_DBG1("Executing Task %p", (uintptr_t) pRec);
        if(pRec->handler)
        {
          pRec->handler(pRec->arg);
        }
        //MIBLE_PORT_LOG_HEAP();
        ICall_free(pRec);
        //MIBLE_PORT_LOG_HEAP();
        pRec = Queue_get(port_object.taskQHdl);
      }
  }
}
/**
 *        IIC APIs
 */

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
  // TODO I2C
  return(MI_SUCCESS);
}

/**
 * @brief   Function for uninitializing the IIC driver instance.
 * 
 *              
 * */
void mible_iic_uninit(void)
{
  // TODO I2C
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
  // TODO I2C
  return(MI_SUCCESS);
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
  // TODO I2C
  return(MI_SUCCESS);
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
  // TODO I2C
  return(MI_SUCCESS);
}


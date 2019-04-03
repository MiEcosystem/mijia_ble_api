#include "arch_os.h"
#include "mible_mesh_api.h"

static mible_mesh_event_cb_t mible_mesh_event_callback_handler;


/**********************************************************************//**
 * data is pointer corresponding with type.
 *      mible_gap_adv_report_t *p_adv_report;
 *      mible_mesh_unprov_beacon_t *p_unprov_beacon;
 *      mible_mesh_iv_t *p_mesh_iv;
 *      mible_mesh_config_status_t *p_config_msg;
 *      mible_mesh_access_message_rx_t *p_generic_msg;
 **********************************************************************/
static int mible_mesh_event_callback(mible_mesh_event_type_t type, void * data)
{
    if(mible_mesh_event_callback_handler != NULL){
        mible_mesh_event_callback_handler(type, (mible_mesh_event_params_t *)data);
    }
    return 0;
}

int mible_mesh_node_get_composition_data(uint16_t unicast_address, uint16_t global_netkey_index, uint8_t page)
{
    return 0;
}

int mible_mesh_node_set_appkey(uint16_t opcode, mible_mesh_appkey_params_t *param)
{
    switch(opcode){
        case MIBLE_MESH_MSG_CONFIG_APPKEY_ADD:
        {
            break;
        }
        case MIBLE_MESH_MSG_CONFIG_APPKEY_DELETE:
        {
            break;
        }
        case MIBLE_MESH_MSG_CONFIG_APPKEY_UPDATE:
        {
            break;
        }
        default:
            return -1;
    }
    return 0;
}

int mible_mesh_node_bind_appkey(uint16_t opcode, mible_mesh_model_app_params_t *param)
{
    switch(opcode){
        case MIBLE_MESH_MSG_CONFIG_MODEL_APP_BIND:
        {
            break;
        }
        case MIBLE_MESH_MSG_CONFIG_MODEL_APP_UNBIND:
        {
            break;
        }
        default:
            return -1;
    }
    return 0;
}

int mible_mesh_node_set_publication(uint16_t opcode, mible_mesh_publication_params_t * param)
{
    switch(opcode){
        case MIBLE_MESH_MSG_CONFIG_MODEL_PUBLICATION_GET:
        {
            break;
        }
        case MIBLE_MESH_MSG_CONFIG_MODEL_PUBLICATION_SET:
        {
            break;
        }
        case MIBLE_MESH_MSG_CONFIG_MODEL_PUBLICATION_VIRTUAL_ADDRESS_SET:
        {
            break;
        }
        default:
            return -1;
    }
    return 0;
}

int mible_mesh_node_set_subscription(uint16_t opcode, mible_mesh_subscription_params_t * param)
{
    switch(opcode){
        case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_ADD:
            break;
        case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE:
            break;
        case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_DELETE_ALL:
            break;
        case MIBLE_MESH_MSG_CONFIG_MODEL_SUBSCRIPTION_OVERWRITE:
            break;
        default:
            return -1;
    }

    return 0;
}

int mible_mesh_node_reset(uint16_t opcode, mible_mesh_reset_params_t *param)
{
    return 0;
}

int mible_mesh_node_set_relay_param(uint16_t opcode, mible_mesh_relay_params_t *param)
{
    return 0;
}

int mible_mesh_node_generic_control(mible_mesh_generic_params_t * param)
{
    switch(param->opcode.opcode){
        case MIBLE_MESH_MSG_GENERIC_ONOFF_GET:
        {
            break;
        }
        case MIBLE_MESH_MSG_GENERIC_ONOFF_SET:
        case MIBLE_MESH_MSG_GENERIC_ONOFF_SET_UNACKNOWLEDGED:
        {
            break;
        }
        case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_GET:
        {
            break;
        }
        case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET:
        case MIBLE_MESH_MSG_LIGHT_LIGHTNESS_SET_UNACKNOWLEDGED:
        {
            break;
        }
        case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_GET:
        {
            break;
        }
        case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_SET:
        case MIBLE_MESH_MSG_LIGHT_CTL_TEMPERATURE_RANGE_SET_UNACKNOWLEDGED:
        {
            break;
        }
        case MIBLE_MESH_MIOT_SPEC_GET:
        {
            break;
        }
        case MIBLE_MESH_MIOT_SPEC_SET:
        case MIBLE_MESH_MIOT_SPEC_SET_NOACK:
        {
            break;
        }
        default:
        {
            return -1;
        }
    }

    return 0;
}

int mible_mesh_gateway_register_event_callback(mible_mesh_event_cb_t mible_mesh_event_cb)
{
    mible_mesh_event_callback_handler = mible_mesh_event_cb;
    return 0;
}

int mible_mesh_gateway_unregister_event_callback(mible_mesh_event_cb_t mible_mesh_event_cb)
{
    if(mible_mesh_event_callback_handler == mible_mesh_event_cb){
        mible_mesh_event_callback_handler = NULL;
        return 0;
    }
    return -1;
}

int mible_mesh_gateway_init_stack(void)
{
    // init your mesh stack, thread, memory, prepare for provisioner initilization.

    return 0;
}

int mible_mesh_gateway_deinit_stack(void)
{
    // deinit your mesh stack, close thread,free memory, and release firmware bt resource.

    return 0;
}

int mible_mesh_gateway_init_provisioner(mible_mesh_gateway_info_t *info)
{
    /**********************************************************************//**
     * init provisioner, init models/opcodes.
     * load seq_num, iv_index, replay list
     **********************************************************************/
    return 0;
}

int mible_mesh_gateway_create_network(uint16_t netkey_index, uint8_t *netkey, uint16_t *stack_netkey_index)
{
    /**********************************************************************//**
     * create network, load netkey information
     **********************************************************************/
    return 0;
}

int mible_mesh_gateway_set_network_transmit_param(uint8_t count, uint8_t interval_steps)
{
    // set adv interval and adv transmit times
    return 0;
}

int mible_mesh_start_recv_unprovbeacon(void)
{
    // start recv unprov beacon
    return 0;
}
int mible_mesh_stop_recv_unprovbeacon(void)
{
    // stop recv unprov beacon
    return 0;
}

int mible_mesh_gateway_update_iv_info(uint32_t iv_index, uint8_t flags)
{
    // sync and load iv_index from application
    return 0;
}

int mible_mesh_gateway_set_netkey(mible_mesh_op_t op, uint16_t netkey_index, uint8_t *netkey,
        uint16_t *stack_netkey_index)
{
    if(op == MIBLE_MESH_OP_ADD){
        // add netkey
    }else{
        // delete netkey
    }
    return 0;
}

int mible_mesh_gateway_set_appkey(mible_mesh_op_t op, uint16_t netkey_index, uint16_t appkey_index,
                    uint8_t * appkey, uint16_t *stack_appkey_index)
{
    if(op == MIBLE_MESH_OP_ADD){
        // add app_key, bind netkey index
    }else{
        // delete netkey, unbind netkey index
    }
    return 0;
}

int mible_mesh_gateway_set_model_app(mible_mesh_op_t op, uint16_t company_id, uint16_t model_id, uint16_t appkey_index)
{
    if(op == MIBLE_MESH_OP_ADD){
        // bind model appkey_index
    }else{
        // unbind model appkey_index
    }
    return 0;
}

int mible_mesh_gateway_set_device_key(mible_mesh_op_t op, mible_mesh_node_info_t *device)
{
    if(op == MIBLE_MESH_OP_ADD){
        // load device key
    }else{
        // delete device key, clean replay protection list
    }
    return 0;
}

int mible_mesh_gateway_set_sub_address(mible_mesh_op_t op,  uint16_t company_id, uint16_t model_id,
        mible_mesh_address_t *sub_addr)
{
    if(op == MIBLE_MESH_OP_ADD){
        // add subscription
    }else{
        // delete subscription
    }
    return 0;
}



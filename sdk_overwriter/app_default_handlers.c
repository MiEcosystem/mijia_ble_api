/**
 ****************************************************************************************
 *
 * @file app_default_handlers.c
 *
 * @brief Default helper handlers implementing a primitive peripheral.
 *
 * Copyright (C) 2012-2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com>
 *
 ****************************************************************************************
 */

/*
 * INCLUDE FILES
 ****************************************************************************************
 */

#include "rwip_config.h"             // SW configuration
#include "gap.h"
#include "gapc_task.h"
#include "arch_api.h"
#include "co_bt.h"
#include "co_math.h"
#include "app_prf_types.h"
#include "app_prf_perm_types.h"
#include "app.h"
#include "app_callback.h"
#include "app_default_handlers.h"
#include "app_utils.h"

#if (BLE_APP_SEC)
#include "app_easy_security.h"
#include "app_security.h"
#include "app_bond_db.h"
#endif // (BLE_APP_SEC)

#include "user_profiles_config.h"
#include "user_callback_config.h"

#if (BLE_CUSTOM_SERVER)
#include "user_custs_config.h"
#endif

#if (BLE_MIJIA_SERVER)
#include "app_mijia.h"
#include "mible_type.h"
#include "mijia.h"
#include "mible_api.h"
#endif
/*
 * DEFINES
 ****************************************************************************************
 */

#if (BLE_APP_SEC)
/// Default Passkey that is presented to the user and is entered on the peer device
#define DEFAULT_PASSKEY_TK_VAL          (123456)

/// Default OOB data
#define DEFAULT_SECURITY_OOB_TK_VAL     {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,\
                                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f}

#endif // (BLE_APP_SEC)
/*
 * FUNCTION DEFINITIONS
 ****************************************************************************************
 */

void default_advertise_operation(void)
{
    if (user_default_hnd_conf.adv_scenario == DEF_ADV_FOREVER)
    {
        app_easy_gap_undirected_advertise_start();
    }
    else if (user_default_hnd_conf.adv_scenario == DEF_ADV_WITH_TIMEOUT)
    {
        app_easy_gap_undirected_advertise_with_timeout_start(user_default_hnd_conf.advertise_period, NULL);
    }
}

void default_app_on_init(void)
{
#if BLE_PROX_REPORTER
    app_proxr_init();
#endif

#if BLE_FINDME_TARGET
    app_findt_init();
#endif

#if BLE_FINDME_LOCATOR
    app_findl_init();
#endif

#if BLE_BATT_SERVER
    app_batt_init();
#endif

#if BLE_DIS_SERVER
    app_dis_init();
#endif

#if BLE_BMS_SERVER
    app_bmss_init();
#endif

#if BLE_BCS_SERVER
    app_bcss_init();
#endif

#if BLE_UDS_SERVER
    app_udss_init();
#endif

#if BLE_CTS_SERVER
    app_ctss_init();
#endif

#if BLE_SUOTA_RECEIVER
    app_suotar_init();
#endif

#if BLE_WSS_SERVER
    app_wsss_init();
#endif

#if (BLE_MIJIA_SERVER)
		app_mijia_init();
#endif


    // Initialize service access write permissions for all the included profiles
    prf_init_srv_perm();

    // Set sleep mode
    arch_set_sleep_mode(app_default_sleep_mode);
}

void default_app_on_connection(uint8_t conidx, struct gapc_connection_req_ind const *param)
{
    if (app_env[conidx].conidx != GAP_INVALID_CONIDX)
    {
        if (user_default_hnd_conf.adv_scenario == DEF_ADV_WITH_TIMEOUT)
        {
            app_easy_gap_advertise_with_timeout_stop();
        }

        // Enable the created profiles/services
        app_prf_enable(conidx);

        #if (BLE_APP_SEC)
        if (user_default_hnd_conf.security_request_scenario == DEF_SEC_REQ_ON_CONNECT)
        {
            app_easy_security_request(conidx);
        }
        #endif // (BLE_APP_SEC)
    }
    else
    {
       // No connection has been established, restart advertising
       CALLBACK_ARGS_0(user_default_app_operations.default_operation_adv)
    }
}

void default_app_on_disconnect( struct gapc_disconnect_ind const *param ){
    // Restart Advertising
    CALLBACK_ARGS_0(user_default_app_operations.default_operation_adv)
}

void default_app_on_set_dev_config_complete(void)
{
    // Add the first required service in the database
    if (app_db_init_start())
    {
        // No more service to add, start advertising
        CALLBACK_ARGS_0(user_default_app_operations.default_operation_adv)
    }
}

void default_app_on_db_init_complete( void )
{
    CALLBACK_ARGS_0(user_default_app_operations.default_operation_adv)

		//Lucien debug 
				extern void test_aes_128(void);
				app_easy_timer(500, test_aes_128);
}

void default_app_on_get_dev_appearance(uint16_t* appearance)
{
    *appearance = 0;
}

void default_app_on_get_dev_slv_pref_params(struct gap_slv_pref* slv_params)
{
    slv_params->con_intv_min = MS_TO_DOUBLESLOTS(10);
    slv_params->con_intv_max = MS_TO_DOUBLESLOTS(20);
    slv_params->slave_latency = 0;
    slv_params->conn_timeout = MS_TO_TIMERUNITS(1250);
}

void default_app_on_set_dev_info(uint8_t req, uint8_t* status)
{
    switch (req)
    {
        case GAPC_DEV_NAME:
        {
            *status = GAP_ERR_REJECTED;
        }
        break;
        case GAPC_DEV_APPEARANCE:
        {
            *status = GAP_ERR_REJECTED;
        }
        break;
        default: /* Do Nothing */ break;
    }
}

void default_app_update_params_request(struct gapc_param_update_req_ind const *param, struct gapc_param_update_cfm *cfm)
{
    // by default, the request is being accepted no matter what the param values are
    cfm->accept = true;
    cfm->ce_len_min = MS_TO_DOUBLESLOTS(0);
    cfm->ce_len_max = MS_TO_DOUBLESLOTS(0);
}

void default_app_generate_static_random_addr(struct bd_addr *addr)
{
    // Check if the static random address is already generated.
    // If it is already generated the two MSB are equal to '1'
    if (!(addr->addr[BD_ADDR_LEN - 1] & GAP_STATIC_ADDR))
    {
        // Generate static random address, 48-bits
        co_write32p(&addr->addr[0], co_rand_word());
        co_write16p(&addr->addr[4], co_rand_hword());

        // The two MSB shall be equal to '1'
        addr->addr[BD_ADDR_LEN - 1] |= GAP_STATIC_ADDR;
    }
}

#if (BLE_APP_SEC)

void default_app_on_pairing_request(uint8_t conidx, struct gapc_bond_req_ind const *param)
{
    app_easy_security_send_pairing_rsp(conidx, param);
}

void default_app_on_tk_exch(uint8_t conidx, struct gapc_bond_req_ind const *param)
{
    if (param->data.tk_type == GAP_TK_OOB)
    {
        // By default send hardcoded OOB data
        uint8_t oob_tk[KEY_LEN] = DEFAULT_SECURITY_OOB_TK_VAL;
        // Provide TK to the Host
        app_easy_security_tk_exch(conidx, (uint8_t*) oob_tk, KEY_LEN, true);
    }
    else if (param->data.tk_type == GAP_TK_DISPLAY)
    {
        uint32_t passkey = DEFAULT_PASSKEY_TK_VAL;
        // Provide TK to the Host
        app_easy_security_tk_exch(conidx, (uint8_t*) &passkey, sizeof(passkey), true);
    }
    else if (param->data.tk_type == GAP_TK_KEY_ENTRY)
    {
        uint32_t passkey = DEFAULT_PASSKEY_TK_VAL;
        // Provide TK to the Host
        app_easy_security_tk_exch(conidx, (uint8_t*) &passkey, sizeof(passkey), true);
    }
    else if (param->data.tk_type == GAP_TK_KEY_CONFIRM)
    {
        // Numeric Comparison - Auto-Confirm
        app_easy_security_tk_exch(conidx, (uint8_t *) &param->tk, sizeof(param->tk), true);
    }
}

void default_app_on_csrk_exch(uint8_t conidx, struct gapc_bond_req_ind const *param)
{
    // Provide the CSRK to the host
    app_easy_security_csrk_exch(conidx);
}

void default_app_on_ltk_exch(uint8_t conidx, struct gapc_bond_req_ind const *param)
{
    // generate ltk and store it to sec_env
    app_sec_gen_ltk(conidx, param->data.key_size);
    //copy the parameters in the message
    app_easy_security_set_ltk_exch_from_sec_env(conidx);
    //send the message
    app_easy_security_ltk_exch(conidx);
}

void default_app_on_encrypt_req_ind(uint8_t conidx,
                                 struct gapc_encrypt_req_ind const *param)
{
    const struct app_sec_bond_data_env_tag *pbd = NULL;
    uint8_t bdaddr_type;

    // Check if pairing is in progress
    if (app_env[conidx].pairing_in_progress == false)
    {
        // Check if we have secure connection feature enabled
        if (param->ediv == 0)
        {
            // Check if peer's BD address is public or random static
            bdaddr_type = app_get_address_type(app_env[conidx].peer_addr_type, app_env[conidx].peer_addr);
            if ((bdaddr_type == APP_PUBLIC_ADDR_TYPE) || (bdaddr_type == APP_RANDOM_STATIC_ADDR_TYPE))
            {
                // Search DB by peer's BD address
                pbd = app_easy_security_bdb_search_entry(SEARCH_BY_BDA_TYPE, (void *) app_env[conidx].peer_addr.addr, BD_ADDR_LEN);
                // If peer has been found in DB
                if(pbd)
                {
                    // Store device bond data to security environment
                    app_sec_env[conidx] = *pbd;
                    // Accept encryption
                    app_easy_security_accept_encryption(conidx);
                }
                // If peer has not been found in DB
                else
                {
                    // Reject encryption, disconnect
                    app_easy_security_reject_encryption(conidx);
                }
            }
            // Check if peer's BD address is private resolvable
            else if (bdaddr_type == APP_RANDOM_PRIVATE_RESOLV_ADDR_TYPE)
            {
                // Start BD address resolving procedure
                if(!app_easy_security_resolve_bdaddr(conidx))
                {
                    app_easy_security_reject_encryption(conidx);
                }
            }
            // Check if peer's BD address is private non-resolvable
            else
            {
                // Reject encryption, disconnect
                app_easy_security_reject_encryption(conidx);
            }
        }
        // We have legacy pairing (no secure connection)
        else
        {
            pbd = app_easy_security_bdb_search_entry(SEARCH_BY_EDIV_TYPE, (void *) &param->ediv, 2);
            // If peer has been found in DB
            if(pbd)
            {
                // Store device bond data to security environment
                app_sec_env[conidx] = *pbd;
                // Accept encryption
                app_easy_security_accept_encryption(conidx);
            }
            // If peer has not been found in DB
            else
            {
                // Reject encryption, disconnect
                app_easy_security_reject_encryption(conidx);
            }
        }
    }
    // Devices have not been bonded yet. Pairing phase 3
    else
    {
        // Accept encryption to start exchange of keys
        app_easy_security_accept_encryption(conidx);
    }
}

void default_app_on_pairing_succeeded(uint8_t conidx)
{
    if (app_sec_env[conidx].auth & GAP_AUTH_BOND)
    {
        app_easy_security_bdb_add_entry(&app_sec_env[conidx]);
    }
}

void default_app_on_addr_solved_ind(uint8_t conidx,
                                    struct gapm_addr_solved_ind const *param)
{
    const struct app_sec_bond_data_env_tag *pbd = NULL;

    // Search DB by peer's IRK
    pbd = app_easy_security_bdb_search_entry(SEARCH_BY_IRK_TYPE, (void *) &param->irk, sizeof(struct gap_sec_key));

    // If peer has been found in DB
    if(pbd)
    {
        // Store device bond data to security environment
        app_sec_env[conidx] = *pbd;
        // Accept encryption
        app_easy_security_accept_encryption(conidx);
    }
    // If peer has not been found in DB
    else
    {
        // Reject encryption, disconnect
        app_easy_security_reject_encryption(conidx);
    }
}

void default_app_on_addr_resolve_failed(uint8_t conidx)
{
    app_easy_security_reject_encryption(conidx);
}

void default_app_bdb_init(void)
{
    bond_db_init();
}

uint8_t default_app_bdb_get_size(void)
{
    return bond_db_get_size();
}

void default_app_bdb_add_entry(struct app_sec_bond_data_env_tag *data)
{
    bond_db_add_entry(data);
}

void default_app_bdb_remove_entry(enum bdb_search_by_type search_type, enum bdb_remove_type remove_type,
                                  void *search_param, uint8_t search_param_length)
{
    bond_db_remove_entry(search_type, remove_type, search_param, search_param_length);
}

const struct app_sec_bond_data_env_tag* default_app_bdb_search_entry(
                                                        enum bdb_search_by_type search_type,
                                                        void *search_param,
                                                        uint8_t search_param_length)
{
    return bond_db_search_entry(search_type, search_param, search_param_length);
}

uint8_t default_app_bdb_get_stored_irks(struct gap_sec_key *valid_irk_irray)
{
    return bond_db_get_stored_irks(valid_irk_irray);
}

#endif // (BLE_APP_SEC)

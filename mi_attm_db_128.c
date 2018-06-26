/**
 ****************************************************************************************
 *
 * @file mi_attm_db_128.c
 *
 * @brief ATTM functions that handle the ctration of service database of 128bits long UUID.
 *
 * Copyright (C) 2017 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

/*
 * INCLUDES
 ****************************************************************************************
 */
#include "attm_db_128.h"
#include "mi_attm_db_128.h"
#include "attm_db.h"
#include "ke_mem.h"


#if (BLE_MIJIA_SERVER)

/**
 ****************************************************************************************
 * @brief Handles creation of database with 128bit long UUIDs
 * @param[in] cfg_flag configuration flag
 * @param[in] max_nb_att max number of database attributes
 * @param[in] dest_id task ID that requested the creation of the database.
 * @param[in] att_db pointer to the database description table
 * @param[out] shdl base handle of the database
 * @param[out] att_tbl if not NULL, it returns the attribute handles
 * @return status.
 ****************************************************************************************
 */

uint8_t attm_svc_create_db_mijia(uint16_t *shdl, uint8_t *cfg_flag, uint8_t max_nb_att,
                               uint8_t *att_tbl, ke_task_id_t const dest_id,
                               const struct attm_desc_128 *att_db, uint8_t svc_perm)
	{
		int32_t db_cfg;
		uint8_t nb_att = 0;
		uint8_t status;
		uint8_t i;
		uint8_t att_idx = 0;
		struct gattm_svc_desc *svc_desc = NULL;
		const uint16_t att_decl_char = ATT_DECL_CHARACTERISTIC;
	
		if (cfg_flag != NULL)
		{
			// Number of attributes is limited to 32
			memcpy(&db_cfg, cfg_flag, sizeof(uint32_t));
		}
		else
		{
			// Set all bits to 1
			db_cfg = -1;
		}
	
		// Compute number of attributes and maximal payload size
		for (i = 1; i < max_nb_att; i++)
		{
			// check within db_cfg flag if attribute is enabled or not
			if (((db_cfg >> i) & 1) == 1)
			{
				nb_att++;
			}
		}
	
		// Allocate database descriptor
		svc_desc = (struct gattm_svc_desc *) ke_malloc(sizeof(struct gattm_svc_desc) +
									nb_att * sizeof(struct gattm_att_desc),
									KE_MEM_NON_RETENTION);
	
		svc_desc->start_hdl = *shdl;
		svc_desc->task_id = dest_id;
		svc_desc->perm = svc_perm | PERM_VAL(SVC_UUID_LEN, PERM_UUID_16);
	
		*(uint16_t *)svc_desc->uuid = *(uint16_t *)att_db[0].value;
	
		for (i = 1; i < max_nb_att; i++)
		{
			// check within db_cfg flag if attribute is enabled or not
			if (((db_cfg >> i) & 1) == 1)
			{
				// Add attribute
				if (att_db[i].uuid_size == ATT_UUID_16_LEN)
				{
					svc_desc->atts[att_idx] = (struct gattm_att_desc) {
						.uuid = {att_db[i].uuid[0], att_db[i].uuid[1]},
						.perm = att_db[i].perm | PERM_VAL(UUID_LEN, PERM_UUID_16),
						.max_len = att_db[i].max_length
					};
				}
				else if (att_db[i].uuid_size == ATT_UUID_32_LEN)
				{
					svc_desc->atts[att_idx] = (struct gattm_att_desc) {
						.uuid = {att_db[i].uuid[0], att_db[i].uuid[1], att_db[i].uuid[2], att_db[i].uuid[3]},
						.perm = att_db[i].perm | PERM_VAL(UUID_LEN, PERM_UUID_32),
						.max_len = att_db[i].max_length
					};
				}
				else if (att_db[i].uuid_size == ATT_UUID_128_LEN)
				{
					memcpy(svc_desc->atts[att_idx].uuid, att_db[i].uuid, sizeof(struct att_uuid_128));
					svc_desc->atts[att_idx].perm = att_db[i].perm | PERM_VAL(UUID_LEN, PERM_UUID_128);
					svc_desc->atts[att_idx].max_len = att_db[i].max_length;
				}
				att_idx++;
			}
		}
		svc_desc->nb_att = att_idx;
	
		status = attmdb_add_service(svc_desc);
		if (status == ATT_ERR_NO_ERROR)
		{
			*shdl = svc_desc->start_hdl;
			att_idx = 0;
	
			for (i = 1; i < max_nb_att && (status == ATT_ERR_NO_ERROR); i++)
			{
				// check within db_cfg flag if attribute is enabled or not
				if (((db_cfg >> i) & 1) == 0)
					continue;
				att_idx++;
	
				// If attribute is marked with RI, or is empty don't store it in DB
				if (att_db[i].max_length & PERM(RI, ENABLE) || att_db[i].length == 0)
					continue;
	
				if(att_db[i].uuid_size == ATT_UUID_16_LEN &&
				  (memcmp(att_db[i].uuid, &att_decl_char, sizeof(att_decl_char)) == 0))
					  continue;
	
				status = attmdb_att_set_value(svc_desc->start_hdl + att_idx,
											att_db[i].length, 0, att_db[i].value);
			}
		}
	
		ke_free(svc_desc);
	
		return status;
	}

#endif // BLE MIJIA SERVER
	
	
#if (BLE_SPS_SERVER)

/**
****************************************************************************************
* @brief Handles creation of database with 128bit long UUIDs
* @param[in] cfg_flag configuration flag
* @param[in] max_nb_att max number of database attributes
* @param[in] dest_id task ID that requested the creation of the database.
* @param[in] att_db pointer to the database description table
* @param[out] shdl base handle of the database
* @param[out] att_tbl if not NULL, it returns the attribute handles
* @return status.
****************************************************************************************
*/

uint8_t attm_svc_create_db_sps(uint16_t *shdl, uint8_t *cfg_flag, uint8_t max_nb_att,
															uint8_t *att_tbl, ke_task_id_t const dest_id,
															const struct attm_desc_128 *att_db, uint8_t svc_perm)
 {
	 int32_t db_cfg;
	 uint8_t nb_att = 0;
	 uint8_t status;
	 uint8_t i;
	 uint8_t att_idx = 0;
	 struct gattm_svc_desc *svc_desc = NULL;
	 const uint16_t att_decl_char = ATT_DECL_CHARACTERISTIC;
 
	 if (cfg_flag != NULL)
	 {
		 // Number of attributes is limited to 32
		 memcpy(&db_cfg, cfg_flag, sizeof(uint32_t));
	 }
	 else
	 {
		 // Set all bits to 1
		 db_cfg = -1;
	 }
 
	 // Compute number of attributes and maximal payload size
	 for (i = 1; i < max_nb_att; i++)
	 {
		 // check within db_cfg flag if attribute is enabled or not
		 if (((db_cfg >> i) & 1) == 1)
		 {
			 nb_att++;
		 }
	 }
 
	 // Allocate database descriptor
	 svc_desc = (struct gattm_svc_desc *) ke_malloc(sizeof(struct gattm_svc_desc) +
								 nb_att * sizeof(struct gattm_att_desc),
								 KE_MEM_NON_RETENTION);
 
	 svc_desc->start_hdl = *shdl;
	 svc_desc->task_id = dest_id;
	 svc_desc->perm = svc_perm | PERM_VAL(SVC_UUID_LEN, PERM_UUID_16);
 
	 *(uint16_t *)svc_desc->uuid = *(uint16_t *)att_db[0].value;
 
	 for (i = 1; i < max_nb_att; i++)
	 {
		 // check within db_cfg flag if attribute is enabled or not
		 if (((db_cfg >> i) & 1) == 1)
		 {
			 // Add attribute
			 if (att_db[i].uuid_size == ATT_UUID_16_LEN)
			 {
				 svc_desc->atts[att_idx] = (struct gattm_att_desc) {
					 .uuid = {att_db[i].uuid[0], att_db[i].uuid[1]},
					 .perm = att_db[i].perm | PERM_VAL(UUID_LEN, PERM_UUID_16),
					 .max_len = att_db[i].max_length
				 };
			 }
			 else if (att_db[i].uuid_size == ATT_UUID_32_LEN)
			 {
				 svc_desc->atts[att_idx] = (struct gattm_att_desc) {
					 .uuid = {att_db[i].uuid[0], att_db[i].uuid[1], att_db[i].uuid[2], att_db[i].uuid[3]},
					 .perm = att_db[i].perm | PERM_VAL(UUID_LEN, PERM_UUID_32),
					 .max_len = att_db[i].max_length
				 };
			 }
			 else if (att_db[i].uuid_size == ATT_UUID_128_LEN)
			 {
				 memcpy(svc_desc->atts[att_idx].uuid, att_db[i].uuid, sizeof(struct att_uuid_128));
				 svc_desc->atts[att_idx].perm = att_db[i].perm | PERM_VAL(UUID_LEN, PERM_UUID_128);
				 svc_desc->atts[att_idx].max_len = att_db[i].max_length;
			 }
			 att_idx++;
		 }
	 }
	 svc_desc->nb_att = att_idx;
 
	 status = attmdb_add_service(svc_desc);
	 if (status == ATT_ERR_NO_ERROR)
	 {
		 *shdl = svc_desc->start_hdl;
		 att_idx = 0;
 
		 for (i = 1; i < max_nb_att && (status == ATT_ERR_NO_ERROR); i++)
		 {
			 // check within db_cfg flag if attribute is enabled or not
			 if (((db_cfg >> i) & 1) == 0)
				 continue;
			 att_idx++;
 
			 // If attribute is marked with RI, or is empty don't store it in DB
			 if (att_db[i].max_length & PERM(RI, ENABLE) || att_db[i].length == 0)
				 continue;
 
			 if(att_db[i].uuid_size == ATT_UUID_16_LEN &&
				 (memcmp(att_db[i].uuid, &att_decl_char, sizeof(att_decl_char)) == 0))
					 continue;
 
			 status = attmdb_att_set_value(svc_desc->start_hdl + att_idx,
										 att_db[i].length, 0, att_db[i].value);
		 }
	 }
 
	 ke_free(svc_desc);
 
	 return status;
 }
															 
#endif // BLE sps SERVER
	
	

	

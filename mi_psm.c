#include "mi_psm.h"
#include "mible_type.h"

#include "fds.h"
#include "fstorage.h"

#define NRF_LOG_MODULE_NAME "PSM"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define MI_RECORD_FILE_ID              0x4D49		// file used to storage

uint8_t m_psm_done;
static void mi_psm_fds_evt_handler(fds_evt_t const * const p_fds_evt)
{
    switch (p_fds_evt->id) {
	case FDS_EVT_INIT:
		if (p_fds_evt->result == FDS_SUCCESS) {
			NRF_LOG_INFO("FDS_EVT_INIT SUCCESS\n");
		}else{
			NRF_LOG_INFO("FDS_EVT_INIT FAILED\n");
		}
		break;
		
	case FDS_EVT_WRITE:
		if (p_fds_evt->result == FDS_SUCCESS) {
			NRF_LOG_INFO("FDS_EVT_WR SUCCESS\n");
			if ((uint32_t)p_fds_evt->write.file_id == MI_RECORD_FILE_ID) {
				m_psm_done = 1;
			}
		} else {
			NRF_LOG_INFO("FDS_EVT_WR FAILED\n");
		}
		break;
		
	case FDS_EVT_UPDATE:
		if (p_fds_evt->result == FDS_SUCCESS) {
			NRF_LOG_INFO("FDS_EVT_UPDATE SUCCESS\n");
			if ((uint32_t)p_fds_evt->write.file_id == MI_RECORD_FILE_ID) {
				m_psm_done = 1;
			}
		}else{
			NRF_LOG_INFO("FDS_EVT_UPDATE FAILED\n");
		}
		break;
			
	case FDS_EVT_DEL_RECORD:
		if (p_fds_evt->result == FDS_SUCCESS) {
			NRF_LOG_INFO("FDS_EVT_DEL_RECORD SUCCESS\n");
			if ((uint32_t)p_fds_evt->del.file_id == MI_RECORD_FILE_ID) {
				m_psm_done = 1;
			}
		}else{
			NRF_LOG_INFO("FDS_EVT_DEL_RECORD FAILED\n");
		}
		break;

	case FDS_EVT_DEL_FILE:
		if (p_fds_evt->result == FDS_SUCCESS) {
			NRF_LOG_INFO("FDS_EVT_DEL_FILE SUCCESS\n");
			if ((uint32_t)p_fds_evt->del.file_id == MI_RECORD_FILE_ID) {
				m_psm_done = 1;
			}
		}else{
			NRF_LOG_INFO("FDS_EVT_DEL_FILE FAILED\n");
		}
		break;

	case FDS_EVT_GC:
		if (p_fds_evt->result == FDS_SUCCESS) {
			NRF_LOG_INFO("FDS_EVT_GC SUCCESS\n");
		}else{
			NRF_LOG_INFO("FDS_EVT_GC FAILED\n");
		}
		break;
    }
}


/** @brief Function to determine if a flash write operation in in progress.
 *
 * @return true if a flash operation is in progress, false if not.
 */
bool flash_access_in_progress()
{
    uint32_t count;

    (void)fs_queued_op_count_get(&count);

    return (count != 0);
}

void mi_psm_init(void)
{
	uint32_t errno;
	errno = fds_register(mi_psm_fds_evt_handler);
    APP_ERROR_CHECK(errno);

    errno = fds_init();
    APP_ERROR_CHECK(errno);
}

/**@brief Flash Write function type. */
int mi_psm_record_write(uint16_t rec_key, uint8_t *in, uint16_t in_len)
{
    uint32_t ret = 0;
    fds_record_t        record;
    fds_record_chunk_t  record_chunk;
	fds_record_desc_t   record_desc;
    fds_find_token_t    ftok = {0};
	
	// Set up data.
    record_chunk.p_data         = in;
    record_chunk.length_words   = CEIL_DIV(in_len, sizeof(uint32_t));
	
	// Set up record.
    record.file_id           = MI_RECORD_FILE_ID;
    record.key               = rec_key;
    record.data.p_chunks     = &record_chunk;
    record.data.num_chunks   = 1;

	ret = fds_record_find(MI_RECORD_FILE_ID, rec_key, &record_desc, &ftok);
    if (ret == FDS_SUCCESS)
    {
        ret = fds_record_update(&record_desc, &record);
    }
	else {
		ret = fds_record_write(&record_desc, &record); 
	}

	if (ret == FDS_ERR_NO_SPACE_IN_QUEUES)
	{
		NRF_LOG_INFO("mi psm write KEY %X failed :%d \n", rec_key, ret);
		ret = MI_ERR_RESOURCES;
	}
	else if (ret == FDS_ERR_NO_SPACE_IN_FLASH)
	{
		NRF_LOG_INFO("mi psm startup fds_gc().\n");
		ret = fds_gc();
		if (ret != FDS_SUCCESS)
			NRF_LOG_ERROR("WTF? \n");
	}
    
    return ret;
}

/**@brief Flash Read function type. */
int mi_psm_record_read(uint16_t rec_key, uint8_t *out, uint16_t out_len)
{
	uint32_t ret = 0;
	fds_flash_record_t  flash_record;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok = {0};

	ret = fds_record_find(MI_RECORD_FILE_ID, rec_key, &record_desc, &ftok);
    if (ret == FDS_SUCCESS)
    {
        if (fds_record_open(&record_desc, &flash_record) != FDS_SUCCESS)
        {
            NRF_LOG_INFO("mi psm cann't find KEY %X! \n", rec_key);
            ret = MI_ERR_INVALID_PARAM;
        }

		uint16_t record_len = flash_record.p_header->tl.length_words * sizeof(uint32_t);
		if (out_len <= record_len)
			memcpy(out, (uint8_t*)flash_record.p_data, out_len);
		else
			memcpy(out, (uint8_t*)flash_record.p_data, record_len);

        if (fds_record_close(&record_desc) != FDS_SUCCESS)
        {
            NRF_LOG_INFO("mi psm close file failed! \n");
            ret = MI_ERR_INTERNAL;
        }
    }
	else {
		NRF_LOG_INFO("mi psm cann't find record file. \n");
	    ret = MI_ERR_INVALID_PARAM;
	}
	
	return ret;
}

int mi_psm_record_delete(uint16_t rec_key)
{
	uint32_t ret = 0;
    fds_record_desc_t   record_desc;
    fds_find_token_t    ftok = {0};

	ret = fds_record_find(MI_RECORD_FILE_ID, rec_key, &record_desc, &ftok);
    if (ret == FDS_SUCCESS)
    {
        if (fds_record_delete(&record_desc) != FDS_SUCCESS)
        {
            NRF_LOG_INFO("mi psm delete record failed! \n");
            ret = MI_ERR_INTERNAL;
        }
    }
	else {
		NRF_LOG_INFO("mi psm cann't find the record. \n");
	    ret = MI_ERR_INVALID_PARAM;
	}
	
	return ret;
}

int mi_psm_reset(void)
{
	return fds_file_delete(MI_RECORD_FILE_ID);
}

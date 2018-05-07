#include "mible_type.h"

#include "fds.h"
#include "fstorage.h"

#undef  NRF_LOG_MODULE_NAME
#define NRF_LOG_MODULE_NAME "PSM"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#include "mi_psm.h"

#define MI_RECORD_FILE_ID              0x4D49		// file used to storage
#define MI_RECORD_KEY                  0xBEEF
volatile uint8_t m_psm_done;
extern void mible_arch_event_callback(mible_arch_event_t evt, 
		mible_arch_evt_param_t* param);

static void mi_psm_fds_evt_handler(fds_evt_t const * const p_fds_evt)
{
    mible_arch_evt_param_t param;
    mible_arch_event_t event;
    switch (p_fds_evt->id) {
	case FDS_EVT_INIT:
		if (p_fds_evt->result == FDS_SUCCESS) {
			NRF_LOG_INFO("MI PSM INIT SUCCESS\n");
		}else{
			NRF_LOG_ERROR("MI PSM INIT FAILED\n");
		}
		break;
		
	case FDS_EVT_WRITE:
        NRF_LOG_INFO("REC %X \n", p_fds_evt->write.record_key);
        event = MIBLE_ARCH_EVT_RECORD_WRITE;
        if ((uint32_t)p_fds_evt->write.file_id == MI_RECORD_FILE_ID) {
            param.record.id = (uint16_t)p_fds_evt->write.record_key;
            if (p_fds_evt->result == FDS_SUCCESS) {
                NRF_LOG_INFO("WRITE SUCCESS\n");
				param.record.status = MI_SUCCESS;
            } else {
                NRF_LOG_ERROR("WRITE FAILED\n");
                param.record.status = MIBLE_ERR_UNKNOWN;
            }
            mible_arch_event_callback(event, &param);
        }
		break;
		
	case FDS_EVT_UPDATE:
        NRF_LOG_INFO("REC %X \n", p_fds_evt->write.record_key);
        event = MIBLE_ARCH_EVT_RECORD_WRITE;
        if ((uint32_t)p_fds_evt->write.file_id == MI_RECORD_FILE_ID) {
            param.record.id = (uint16_t)p_fds_evt->write.record_key;
            if (p_fds_evt->result == FDS_SUCCESS) {
                NRF_LOG_INFO("REWRITE SUCCESS\n");
				param.record.status = MI_SUCCESS;
            } else {
                NRF_LOG_ERROR("REWRITE FAILED\n");
                param.record.status = MIBLE_ERR_UNKNOWN;
            }
            mible_arch_event_callback(event, &param);
        }
		break;
		
	case FDS_EVT_DEL_RECORD:
        NRF_LOG_INFO("REC %X \n", p_fds_evt->del.record_key);
        event = MIBLE_ARCH_EVT_RECORD_DELETE;
        if ((uint32_t)p_fds_evt->del.file_id == MI_RECORD_FILE_ID) {
            param.record.id = (uint16_t)p_fds_evt->del.record_key;
            if (p_fds_evt->result == FDS_SUCCESS) {
                NRF_LOG_INFO("DELETE SUCCESS\n");
				param.record.status = MI_SUCCESS;
            } else {
                NRF_LOG_ERROR("DELETE FAILED\n");
                param.record.status = MIBLE_ERR_UNKNOWN;
            }
            mible_arch_event_callback(event, &param);
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

#ifdef DEBUG
    fds_stat_t stat;
    fds_stat(&stat);
    NRF_LOG_WARNING("FDS STAT:\n");
    NRF_LOG_RAW_INFO(" used:\t %d\n free:\t %d\n gc:\t %d\n", stat.words_used<<2, stat.largest_contig<<2, stat.freeable_words<<2);
#endif
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
int mi_psm_record_write(uint16_t rec_key, const uint8_t *in, uint16_t in_len)
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
		NRF_LOG_ERROR("mi psm write KEY %X failed :%d \n", rec_key, ret);
		ret = MI_ERR_RESOURCES;
	}
	else if (ret == FDS_ERR_NO_SPACE_IN_FLASH)
	{
		NRF_LOG_ERROR("mi psm startup fds_gc().\n");
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
            NRF_LOG_ERROR("mi psm cann't find KEY %X! \n", rec_key);
            ret = MI_ERR_INVALID_PARAM;
        }

		uint16_t record_len = flash_record.p_header->tl.length_words * sizeof(uint32_t);
		if (out_len <= record_len)
			memcpy(out, (uint8_t*)flash_record.p_data, out_len);
		else
			memcpy(out, (uint8_t*)flash_record.p_data, record_len);

        if (fds_record_close(&record_desc) != FDS_SUCCESS)
        {
            NRF_LOG_ERROR("mi psm close file failed! \n");
            ret = MI_ERR_INTERNAL;
        }
    }
	else if (rec_key == 0x10) {
        ret = mi_psm_record_read(MI_RECORD_KEY, out, out_len);
        if (ret == MI_SUCCESS) {
            ret = mi_psm_record_write(rec_key, out, out_len);
            if (ret == MI_SUCCESS)
                mi_psm_record_delete(MI_RECORD_KEY);
            else
                return MI_ERR_INTERNAL;
        } else {
            ret = MI_ERR_INVALID_PARAM;
        }
    }
    else {
		NRF_LOG_ERROR("mi psm cann't read the record %X. \n", rec_key);
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
            NRF_LOG_ERROR("mi psm delete record failed! \n");
            ret = MI_ERR_INTERNAL;
        }
    }
	else {
		NRF_LOG_ERROR("mi psm cann't delete the record %X. \n", rec_key);
	    ret = MI_ERR_INVALID_PARAM;
	}
	return ret;
}

int mi_psm_reset(void)
{
	return fds_file_delete(MI_RECORD_FILE_ID);
}

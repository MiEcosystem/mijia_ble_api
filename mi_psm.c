#include "mi_config.h"
#include "mible_type.h"

#undef  NRF_LOG_MODULE_NAME
#define NRF_LOG_MODULE_NAME "PSM"

#include "mi_psm.h"
#include "osal.h"

#define MI_RECORD_FILE_ID              0x4D49		// file used to storage
volatile uint8_t m_psm_done;
extern void mible_arch_event_callback(mible_arch_event_t evt, 
		mible_arch_evt_param_t* param);


/** @brief Function to determine if a flash write operation in in progress.
 *
 * @return true if a flash operation is in progress, false if not.
 */
bool flash_access_in_progress()
{
    uint32_t count;

    OS_ASSERT(0);

    return (count != 0);
}

void mi_psm_init(void)
{
    OS_ASSERT(0);
    return;
}

/**@brief Flash Write function type. */
int mi_psm_record_write(uint16_t rec_key, uint8_t *in, uint16_t in_len)
{
    uint32_t ret = 0;

    OS_ASSERT(0);
    return ret;
}

/**@brief Flash Read function type. */
int mi_psm_record_read(uint16_t rec_key, uint8_t *out, uint16_t out_len)
{
	uint32_t ret = 0;

	OS_ASSERT(0);
	return ret;
}

int mi_psm_record_delete(uint16_t rec_key)
{
        uint32_t ret = 0;

        OS_ASSERT(0);
	return ret;
}

int mi_psm_reset(void)
{
    uint32_t ret = 0;

    OS_ASSERT(0);
    return ret;
}

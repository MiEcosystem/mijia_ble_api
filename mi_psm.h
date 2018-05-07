#ifndef __MI_PSM_H
#define __MI_PSM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum {
	REC_DEVICE_ID          = 0x0001,
	REC_VERSION            = 0x0002,
	REC_STATUS             = 0x0003,

	REC_MKPK_KEY           = 0x0010,
} mi_psm_record_t;

extern volatile uint8_t m_psm_done;

void mi_psm_init(void);

int mi_psm_record_read(uint16_t rec_key, uint8_t *out, uint16_t out_len);

int mi_psm_record_write(uint16_t rec_key, const uint8_t *in, uint16_t in_len);

int mi_psm_record_delete(uint16_t rec_key);

int mi_psm_reset(void);

#ifdef __cplusplus
}
#endif

#endif  /* END OF __MI_PSM_H */ 

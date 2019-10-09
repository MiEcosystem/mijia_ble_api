#ifndef __NRF_EVT_H__
#define __NRF_EVT_H__
#include <stdint.h>

#include "mi_config.h"
void gap_evt_dispatch(ble_evt_hdr_t *hdr);
void gatts_evt_dispatch(ble_evt_hdr_t *hdr);


#endif  /* __NRF_EVT_H__ */ 

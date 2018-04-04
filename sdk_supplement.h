/*****************************************************************************
File Name:    sdk_supplement.h
Description:  SDK 接口函数补充  头文件
History:
Date                Author                   Description
2018-02-01         Lucien                    Creat
****************************************************************************/
#ifndef __SDK_SUPPLEMENT_H__
#define __SDK_SUPPLEMENT_H__


#include "mible_type.h"



mible_status_t app_manual_gap_advertise_start(mible_gap_adv_param_t * param);

void app_manual_gap_param_update_start(uint8_t conidx,mible_gap_conn_param_t *conn_params);


void get_mac_addr(uint8_t *paddr);


mible_status_t app_gap_adv_data_set(uint8_t const * p_data,
        uint8_t dlen, uint8_t const *p_sr_data, uint8_t srdlen);


#endif


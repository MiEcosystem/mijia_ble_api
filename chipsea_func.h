#ifndef __CHIPSEA_FUNC_H__
#define __CHIPSEA_FUNC_H__

#define MIBLE_TIMER_NUM     5
#define MIBLE_TIMER1_EVT    0x0010
#define MIBLE_TIMER2_EVT    0x0020
#define MIBLE_TIMER3_EVT    0x0040
#define MIBLE_TIMER4_EVT    0x0080
#define MIBLE_TIMER5_EVT    0x0100

#define MIBLE_TASK_EVT      0x0200

/*TIMER related function*/
typedef struct
{
    const uint16_t id;
    uint32_t timeout;
    mible_timer_mode mode;
    mible_timer_handler cb;
    void *para;
} timer_item_t;

mible_status_t mible_get_advertising_data(uint8 *dat, uint8 *datLen);
void mible_services_init(uint8 taskId);
mible_status_t timer_create(void **p_timer_id, mible_timer_handler timeout_handler, mible_timer_mode mode);
mible_status_t timer_delete(void *timer_id);
mible_status_t timer_start(void *timer_id, uint32_t timeout_value, void *p_context);
mible_status_t timer_stop(void *timer_id);
mible_status_t timer_execute(uint16_t id);

mible_status_t record_create(uint16_t record_id, uint8_t len);
mible_status_t record_delete(uint16_t record_id);
mible_status_t record_read(uint16_t record_id, uint8_t *p_data, uint8_t len);
mible_status_t record_write(uint16_t record_id, const uint8_t *p_data, uint8_t len);

mible_status_t task_post(mible_handler_t handler, void *arg);
void tasks_exec(void);

void gap_evt_dispatch(mible_gap_evt_t evt);
void gatts_evt_dispatch(mible_gatts_evt_t evt,uint8 param_id,uint8 len);

#endif /* __CHIPSEA_FUNC_H__ */

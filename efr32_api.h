#ifndef EFR32_API_H
#define EFR32_API_H

#include "mible_type.h"
#include "native_gecko.h"

/*
 * RECORD_INFO_KEY - Information key index - DO NOT CHANGE IT
 * MAX_SINGLE_PS_LENGTH - Max size of each PS key - DO NOT CHANGE IT
 */
#define RECORD_INFO_KEY                         0x4000
#define MAX_SINGLE_PS_LENGTH                    56

/* Connection handle value in disconnection state */
#define DISCONNECTION                           0xFF
#define INVALID_CONNECTION_HANDLE               0xFFFF

/* Invalid the permit since it is decided by application */
#define INVALID_PERMIT_VALUE                    0xFF

/* timer id value to indicate the timer hasn't been used */
#define TIMER_NOT_USED                          0

/* Max random data size for gecko API to generate random data - DO NOT CHANGE IT */
#define MAX_SINGLE_RANDOM_DATA_LENGTH           16

/* Not used for now */
#define RETRY_CONN_UPDATE_BIT_MASK              (1<<0)
#define RETRY_SCANNING_TIMEOUT_BIT_MASK         (1<<1)
#define RETRY_START_ADV_BIT_MASK                (1<<2)

/** Timer Frequency used. */
#define TIMER_CLK_FREQ                          ((uint64)32768)
/** Convert msec to timer ticks. */
#define MS_2_TIMERTICK(ms)                      ((TIMER_CLK_FREQ * (ms)) / 1000)
#define SEC_2_TIMERTICK(s)                      (TIMER_CLK_FREQ * (s))
#define UINT32_TO_BITSTREAM(p, n)                                              \
{ *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8);                         \
*(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }

#define RECORD_TO_STORE_BYTES(rid, len) {(uint8_t)((rid) >> 8), (uint8_t)(rid), (uint8_t)(len)};

#define TIMER_ID_RESTART                        0xFE
#define TIMER_ID_SCAN_TIMEOUT                   0xFF

#define MIBLE_TASK_BIT_MASK                     (1<<29)
#define START_ADV_RETRY_BIT_MASK                (1<<30)

#define MIBLE_TIMER_NUM                         20
#define MIBLE_USER_TIMER_INDEX                  10
#define MIBLE_USER_REC_ID_BASE                  0x50

typedef enum {
    idle_s,
	scanning_s,
	connecting_s,
	connected_s,
	conn_update_sent_s
} state_t;

typedef struct {
    state_t cur_state;
    mible_gap_connect_t target_to_connect;
} target_connect_t;

typedef struct {
    mible_handler_t handler;
    void *args;
} handler_item_t;

void mible_stack_event_handler(struct gecko_cmd_packet* event);

#endif

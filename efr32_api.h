#ifndef EFR32_API_H
#define EFR32_API_H

#include "mible_type.h"

/* Always define this for now, nonblocking hasn't been implemented so that it could cause problem if the user call the mible APIs earlier than system boot event recieved*/
#define BLOCKING_WAITING_FOR_BOOT_EVT

/* AES block size, always be 16 */
#define AES_BLOCK_SIZE                          16

/*
 * Record related configurations
 * MAX_RECORD_NUMBER - The number of max records
 * MAX_RECORD_SIZE - Maximum size of each record
 *
 * Keep the amount of bytes to be no larger than 1600 should be safe since there is no bonding will be used
 * */

#define MAX_RECORD_NUMBER                       12
#define MAX_RECORD_SIZE                         120

/*
 * RECORD_INFO_KEY - Information key index - DO NOT CHANGE IT
 * MAX_SINGLE_PS_LENGTH - Max size of each PS key - DO NOT CHANGE IT
 * PS_KEYS_FOR_EACH_RECORD - How many PS keys for each record. It can be adjusted by the user, the maximum data for each PS key is 56, so this value depends on the maximum size requirement for records.
 */
#define RECORD_INFO_KEY                         0x4000
#define MAX_SINGLE_PS_LENGTH                    56
#define PS_KEYS_FOR_EACH_RECORD                 3

/* Connection handle value in disconnection state */
#define DISCONNECTION                           0xFF
#define INVALID_CONNECTION_HANDLE               0xFFFF

/* Invalid the permit since it is decided by application */
#define INVALID_PERMIT_VALUE                    0xFF

/* timer id value to indicate the timer hasn't been used */
#define TIMER_NOT_USED                          0

/* Max random data size for gecko API to generate random data - DO NOT CHANGE IT */
#define MAX_SINGLE_RANDOM_DATA_LENGTH           16

/* Max number of connections allowed by stack simultaneously */
#ifndef MAX_CONNECTIONS
#define MAX_CONNECTIONS                         1
#endif

/* Not used for now */
#define RETRY_CONN_UPDATE_BIT_MASK              0x01
#define RETRY_SCANNING_TIMEOUT_BIT_MASK         0x02
#define RETRY_START_ADV_BIT_MASK                0x04

/** Timer Frequency used. */
#define TIMER_CLK_FREQ                          ((uint32)32768)
/** Convert msec to timer ticks. */
#define TIMER_MS_2_TIMERTICK(ms)                ((TIMER_CLK_FREQ * ms) / 1000)
/** Stop timer. */
#define TIMER_STOP                              0

#define UINT32_TO_BITSTREAM(p, n)                                              \
{ *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8);                         \
*(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }

#define RECORD_TO_STORE_BYTES(rid, len) {(uint8_t)((rid) >> 8), (uint8_t)(rid), (uint8_t)(len)};

#define SCAN_TIMEOUT_TIMER_ID                   0xFF

#define SCAN_RETRY_BIT_MASK                     0x80000000
#define START_ADV_RETRY_BIT_MASK                0x40000000
#define UPDATE_CON_RETRY_BIT_MASK               0x20000000

#define TIMERS_FOR_STACK                        1
#define MAX_TIMERS                              5
#define TOTAL_TIMERS                            MAX_TIMERS+TIMERS_FOR_STACK

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
    uint8_t timer_id;
    mible_timer_handler handler;
    mible_timer_mode mode;
    void *args;
} timer_item_t;

/* timer handler - 0xFF and one timer are always reserved for GAP */
typedef struct {
    uint8_t active_timers;
    timer_item_t timer[MAX_TIMERS];
} timer_pool_t;

typedef struct {
    mible_handler_t handler;
    void *args;
} handler_item_t;

void mible_stack_event_handler(struct gecko_cmd_packet* event);

#endif

#ifndef ERF32_API_H__
#define ERF32_API_H__

#include "mible_type.h"
/* Connection handle value in disconnection state */
#define DISCONNECTION                           0xFF
#define INVALID_CONNECTION_HANDLE               0xFFFF
#define USER_CMD_SERVICE_INIT					200

void mible_stack_event_handler(struct gecko_cmd_packet *evt);
mible_status_t mible_gap_address_set(mible_addr_t mac);

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


#endif


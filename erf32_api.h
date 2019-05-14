#ifndef ERF32_API_H__
#define ERF32_API_H__

//#include "mible_type.h"
#include "bg_types.h"
#include "gecko_bglib.h"
#include "host_gecko.h"
#include "mesh_generic_model_capi_types.h"
#include "mible_mesh_api.h"

/* Connection handle value in disconnection state */
#define DISCONNECTION 							0xFF
#define INVALID_CONNECTION_HANDLE 				0xFFFF
#define USER_CMD_SERVICE_INIT 					200
#define USER_CMD_VERSION 						201

/*typedef enum {*/
    //MIBLE_MESH_EVENT_STACK_INIT_DONE=0,     [>*< NULL <]
    //MIBLE_MESH_EVENT_PROVISIONER_INIT_DONE, [>*< NULL <]
    //MIBLE_MESH_EVENT_ADV_PACKAGE,           [>*< mible_gap_adv_report_t <]
    //MIBLE_MESH_EVENT_UNPROV_DEVICE,         [>*< mible_mesh_unprov_beacon_t <]
    //MIBLE_MESH_EVENT_IV_UPDATE,             [>*< mible_mesh_iv_t <]
    //MIBLE_MESH_EVENT_CONFIG_MESSAGE_CB,     [>*< Mesh Profile definition message <]
    //MIBLE_MESH_EVENT_GENERIC_MESSAGE_CB,    [>*< Mesh Model definition message <]
/*} mible_mesh_event_type_t;*/
void mible_stack_event_handler(struct gecko_cmd_packet *evt);
void mible_mesh_stack_event_handler(struct gecko_cmd_packet *evt); 
int mible_mesh_event_callback(mible_mesh_event_type_t type, void * data);

typedef enum {
    idle_s,
	scanning_s,
	connecting_s,
	connected_s,
	conn_update_sent_s
} state_t;


/*typedef struct {
    state_t cur_state;
    mible_gap_connect_t target_to_connect;
} target_connect_t;
*/
extern uint8_t ble_scanning; 
void get_time();
void hexdump(uint8_t *base_addr, uint8_t bytes);
void mesh_init();

void cmd_mutex_init();
void cmd_mutex_get();
void cmd_mutex_put();
#endif


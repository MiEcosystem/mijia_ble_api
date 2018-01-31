#include "nrf_drv_twi_patched.h"

#define NRF_LOG_MODULE_NAME "IIC"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

/* Indicates if operation on TWI has ended. */
volatile bool m_twi0_xfer_done = false;

/* TWI instance. */
const nrf_drv_twi_t TWI0 = NRF_DRV_TWI_INSTANCE(0);

/**
 * @brief TWI events handler.
 */
void twi0_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	switch (p_event->type) {
	case NRF_DRV_TWI_EVT_DONE:
		NRF_LOG_INFO("TWI evt done: %d\n", p_event->xfer_desc.type);
		m_twi0_xfer_done = true;
		break;

	default:
		NRF_LOG_ERROR("TWI evt error %d.\n", p_event->type);
		break;
    }
}

void twi0_init (void)
{
    ret_code_t err_code;

    const nrf_drv_twi_config_t msc_config = {
       .scl                = 28,
       .sda                = 29,
       .frequency          = NRF_TWI_FREQ_100K,
       .interrupt_priority = APP_IRQ_PRIORITY_HIGH,
       .clear_bus_init     = true
    };

    err_code = nrf_drv_twi_init(&TWI0, &msc_config, twi0_handler, NULL);
    APP_ERROR_CHECK(err_code);

    nrf_drv_twi_enable(&TWI0);
}

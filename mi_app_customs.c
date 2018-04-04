#include "rwble_config.h"              // SW configuration
#include "gattc_task.h"
#include "att.h"
#include "attm_db.h"
#include "prf_types.h"
#include "prf_utils.h"
#include "mijia.h"


#if (BLE_MIJIA_SERVER)
uint16_t mijia_get_att_handle(uint8_t att_idx)
{
    struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    uint16_t handle = ATT_INVALID_HDL;

    if (att_idx < mijia_env->max_nb_att)
    {
        handle = mijia_env->shdl + att_idx;
    }

    return handle;
}

uint8_t mijia_get_att_idx(uint16_t handle, uint8_t *att_idx)
{
    struct mijia_env_tag *mijia_env = PRF_ENV_GET(MIJIA, mijia);
    uint8_t status = PRF_APP_ERROR;

    if ((handle >= mijia_env->shdl) && (handle < mijia_env->shdl + mijia_env->max_nb_att))
    {
        *att_idx = handle - mijia_env->shdl;
        status = ATT_ERR_NO_ERROR;
    }

    return status;
}
#endif // (BLE_MIJIA_SERVER)
/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_nvm.c
* @brief     xiaomi ble nvm api
* @details   NVM data types and functions.
* @author    hector_huang
* @date      2018-12-26
* @version   v1.0
* *********************************************************************************************************
*/
#include <string.h>
#include "mible_api.h"
#include "flash_device.h"
#define MI_LOG_MODULE_NAME "RTK_NVM"
#include "mible_log.h"
#include "mijia_mesh_config.h"
#include "ftl.h"
#include "platform_types.h"
#include "silent_dfu_flash.h"

#include "patch_header_check.h"
#include "otp.h"
#include "platform_misc.h"

#define CREATE_WHEN_WRITE            1

#define MAX_RECORD_NUM               5
#define INVALID_RECORD_INDEX         0xFF
#define OFFSET_HEADER                MI_RECORD_OFFSET
#define CALC_LEN(len)                (((len) + 3) / 4 * 4)

typedef struct
{
    uint16_t record_id;
    uint16_t offset;
    uint8_t len;
    uint8_t valid;
    uint8_t padding[2];
} record_t;
static record_t records[MAX_RECORD_NUM];

#define OFFSET_RECORD               (MI_RECORD_OFFSET + sizeof(records))

/* dfu releated parameters, support pasue and resume download */
#define PATCH_SIZE                  (40 * 1024)
#define SEC_BOOT_SIZE               (4 * 1024)
#define OTA_RECORD_OFFSET           (MI_RECORD_OFFSET - 4)

typedef union
{
    struct
    {
        //bool patch_exist;
        bool secboot_exist;
        bool app_exist;
    };
    uint8_t pad[4];
} ota_ctx_t;

static ota_ctx_t ota_ctx;


static void dump_record_header(void)
{
    MI_LOG_DEBUG("record header:");
    for (uint8_t i = 0; i < MAX_RECORD_NUM; ++i)
    {
        MI_LOG_DEBUG("id: %d, offset: %d, len: %d, valid: %d",
                     records[i].record_id, records[i].offset,
                     records[i].len, records[i].valid);
    }
}

static mible_status_t mible_record_save_header(void)
{
    if (0 != ftl_save((void *)records, OFFSET_HEADER, sizeof(records)))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    //dump_record_header();
    return MI_SUCCESS;
}

static mible_status_t mible_record_load_header(void)
{
    if (0 != ftl_load((void *)records, OFFSET_HEADER, sizeof(records)))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    return MI_SUCCESS;
}

mible_status_t mible_record_init(void)
{
    MI_LOG_DEBUG("mible record init");
    mible_record_load_header();
    uint8_t index = 0;
    /* find exists record */
    for (index = 0; index < MAX_RECORD_NUM; ++index)
    {
        if ((0 != records[index].valid) && (1 != records[index].valid))
        {
            /* new flash, initialize it */
            memset(&records[index], 0, sizeof(record_t));
        }
    }
    mible_record_save_header();

    dump_record_header();

    return MI_SUCCESS;
}

void mible_record_clear(void)
{
    memset(records, 0, sizeof(records));
    ftl_save((void *)records, OFFSET_HEADER, sizeof(records));
}

static uint8_t mible_record_find_index(uint16_t record_id, uint8_t len)
{
    uint8_t index;
    /* find exists record */
    for (index = 0; index < MAX_RECORD_NUM; ++index)
    {
        if ((records[index].record_id == record_id) && (records[index].len == len))
        {
            break;
        }
    }

    if (index >= MAX_RECORD_NUM)
    {
        /* find empty record */
        for (index = 0; index < MAX_RECORD_NUM; ++index)
        {
            if (!records[index].valid)
            {
                break;
            }
        }
        if (index >= MAX_RECORD_NUM)
        {
            index = INVALID_RECORD_INDEX;
        }
    }


    return index;
}

static uint16_t mible_record_get_offset(void)
{
    uint8_t max_index = 0;
    uint16_t max_offset = 0;
    for (uint8_t i = 0; i < MAX_RECORD_NUM; ++i)
    {
        if (records[i].offset > max_offset)
        {
            max_index = i;
            max_offset = records[i].offset;
        }
    }
    if (0 == max_offset)
    {
        max_offset = OFFSET_RECORD;
    }
    max_offset += CALC_LEN(records[max_index].len);

    return max_offset;
}

mible_status_t mible_record_create(uint16_t record_id, uint8_t len)
{
    MI_LOG_DEBUG("mible record create: id(%d), len(%d)", record_id, len);
    uint8_t index = mible_record_find_index(record_id, len);
    if (INVALID_RECORD_INDEX == index)
    {
        return MI_ERR_NO_MEM;
    }

    uint16_t offset = records[index].offset;
    if (records[index].record_id != record_id)
    {
        offset = mible_record_get_offset();
    }

    records[index].record_id = record_id;
    records[index].len = len;
    records[index].offset = offset;
    records[index].valid = 1;

    mible_record_save_header();

    return MI_SUCCESS;
}

mible_status_t mible_record_delete(uint16_t record_id)
{
    MI_LOG_DEBUG("mible record delete: %d", record_id);
    uint8_t index = 0;
    for (; index < MAX_RECORD_NUM; ++index)
    {
        if (records[index].valid && (records[index].record_id == record_id))
        {
            records[index].valid = 0;
            mible_record_save_header();
            break;
        }
    }

    mible_arch_evt_param_t param;
    memset(&param, 0, sizeof(param));
    param.record.status = MI_SUCCESS;
    param.record.id = record_id;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_DELETE, &param);

    return MI_SUCCESS;
}

mible_status_t mible_record_read(uint16_t record_id, uint8_t *p_data,
                                 uint8_t len)
{
    MI_LOG_DEBUG("mible record read: id(%d), len(%d)", record_id, len);
    uint8_t index = 0;
    for (; index < MAX_RECORD_NUM; ++index)
    {
        if (records[index].valid && (records[index].record_id == record_id))
        {
            break;
        }
    }

    if (index >= MAX_RECORD_NUM)
    {
        return MI_ERR_INVALID_PARAM;
    }

    if (NULL == p_data)
    {
        return MI_ERR_INVALID_ADDR;
    }

    if ((0 == len) || (len > records[index].len))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    if (0 != ftl_load((void *)p_data, records[index].offset, len))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    return MI_SUCCESS;
}

mible_status_t mible_record_write(uint16_t record_id, const uint8_t *p_data,
                                  uint8_t len)
{
    MI_LOG_DEBUG("mible record write: id(%d), len(%d)", record_id, len);
    uint8_t index = 0;
    for (; index < MAX_RECORD_NUM; ++index)
    {
        if (records[index].valid && (records[index].record_id == record_id))
        {
            break;
        }
    }

    if (index >= MAX_RECORD_NUM)
    {
#if !CREATE_WHEN_WRITE
        return MI_ERR_INVALID_PARAM;
#else
        mible_status_t ret = mible_record_create(record_id, len);
        if (MI_SUCCESS != ret)
        {
            return ret;
        }

        /* find index again */
        for (index = 0; index < MAX_RECORD_NUM; ++index)
        {
            if (records[index].valid && (records[index].record_id == record_id))
            {
                break;
            }
        }

        if (index >= MAX_RECORD_NUM)
        {
            return MI_ERR_INVALID_PARAM;
        }
#endif
    }

    if (NULL == p_data)
    {
        return MI_ERR_INVALID_ADDR;
    }

    if ((0 == len) || (len > records[index].len))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    if (0 != ftl_save((void *)p_data, records[index].offset, len))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    mible_arch_evt_param_t param;
    memset(&param, 0, sizeof(param));
    param.record.status = MI_SUCCESS;
    param.record.id = record_id;
    mible_arch_event_callback(MIBLE_ARCH_EVT_RECORD_WRITE, &param);

    return MI_SUCCESS;
}

mible_status_t mible_nvm_init(void)
{
    return MI_SUCCESS;
}

static mible_status_t mible_ota_ctx_save(void)
{
    if (0 != ftl_save((void *)&ota_ctx, OTA_RECORD_OFFSET, sizeof(ota_ctx_t)))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    return MI_SUCCESS;
}

static mible_status_t mible_ota_ctx_load(void)
{
    if (0 != ftl_load((void *)&ota_ctx, OTA_RECORD_OFFSET, sizeof(ota_ctx_t)))
    {
        return MI_ERR_INVALID_LENGTH;
    }

    return MI_SUCCESS;
}

mible_status_t mible_upgrade_firmware(void)
{
    /* load ota context */
    mible_ota_ctx_load();
    MI_LOG_DEBUG("mible_upgrade_firmware: app_exist(%d), secboot_exist(%d)", ota_ctx.app_exist,
                 ota_ctx.secboot_exist);

    uint32_t base_addr = 0;
    T_IMG_CTRL_HEADER_FORMAT *p_ota_header;

    unlock_flash_all();
    if (ota_ctx.app_exist)
    {
        base_addr = PATCH_SIZE + ota_ctx.secboot_exist * SEC_BOOT_SIZE + get_temp_ota_bank_addr_by_img_id(
                        RomPatch);
        MI_LOG_DEBUG("mible_upgrade_firmware: app base addr: 0x%x", base_addr);
        p_ota_header = (T_IMG_CTRL_HEADER_FORMAT *)base_addr;
        p_ota_header->ctrl_flag.flag_value.not_ready = 0;
    }

    if (ota_ctx.secboot_exist)
    {
        base_addr = PATCH_SIZE + get_temp_ota_bank_addr_by_img_id(RomPatch);
        MI_LOG_DEBUG("mible_upgrade_firmware: secure boot base addr: 0x%x", base_addr);
        p_ota_header = (T_IMG_CTRL_HEADER_FORMAT *)base_addr;
        p_ota_header->ctrl_flag.flag_value.not_ready = 0;
    }

    base_addr = get_temp_ota_bank_addr_by_img_id(RomPatch);;
    MI_LOG_DEBUG("mible_upgrade_firmware: patch base addr: 0x%x", base_addr);
    p_ota_header = (T_IMG_CTRL_HEADER_FORMAT *)base_addr;
    p_ota_header->ctrl_flag.flag_value.not_ready = 0;

    MI_LOG_DEBUG("upgrade verify ok, restarting....");
    plt_reset(0);
    return MI_SUCCESS;
}

mible_status_t mible_nvm_read(void *p_data, uint32_t length, uint32_t address)
{
    if (!flash_read_locked(address, length, p_data))
    {
        return MI_ERR_INTERNAL;
    }

    return MI_SUCCESS;
}

mible_status_t mible_nvm_write(void *p_data, uint32_t length, uint32_t address)
{
    MI_LOG_DEBUG("mible_nvm_write: address = 0x%x, length = %d, p_data = 0x%x", address, length,
                 p_data);

    if (NULL == p_data)
    {
        MI_LOG_WARNING("mible_nvm_write: invalid data");
        return MI_ERR_INVALID_PARAM;
    }

    if ((address >= OTP->ota_tmp_addr) && (address < (OTP->ota_tmp_addr + OTP->ota_tmp_size)))
    {
        if (address == OTP->ota_tmp_addr)
        {
            /* ota started, clear context first */
            //ota_ctx.patch_exist = FALSE;
            ota_ctx.app_exist = FALSE;
            ota_ctx.secboot_exist = FALSE;
            mible_ota_ctx_save();

            T_IMG_CTRL_HEADER_FORMAT *p_header = (T_IMG_CTRL_HEADER_FORMAT *)p_data;
            if (p_header->image_id == RomPatch)
            {
                //ota_ctx.patch_exist = TRUE;
                if (p_header->ctrl_flag.flag_value.rsvd & 0x08)
                {
                    ota_ctx.secboot_exist = TRUE;
                }
                if (p_header->ctrl_flag.flag_value.rsvd & 0x10)
                {
                    ota_ctx.app_exist = TRUE;
                }
                mible_ota_ctx_save();
            }
            if (AppPatch == p_header->image_id)
            {
                MI_LOG_WARNING("mible_nvm_write: invalid id(AppPatch)");
                return MI_ERR_INTERNAL;
            }

            if (SecureBoot == p_header->image_id)
            {
                MI_LOG_WARNING("mible_nvm_write: invalid id(SecureBoot)");
                return MI_ERR_INTERNAL;
            }

            MI_LOG_DEBUG("mible_nvm_write: new image header:0x%08x, signature:0x%08x, dfu_base_addr:0x%08x",
                         length, p_header->image_id, address);
        }

        /* new page starts */
        if (0 == (address % FMC_SEC_SECTION_LEN))
        {
            flash_erase_locked(FLASH_ERASE_SECTOR, address);
        }
    }

    flash_write_locked(address, length, p_data);

    return MI_SUCCESS;
}


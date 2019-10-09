/**
 ****************************************************************************************
 *
 * @file appl_storage.c
 *
 * @brief HAP storage implementation
 *
 * Copyright (C) 2015-2018 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "osal.h"
#include <appl_storage.h>
#include "ad_nvms.h"
//#include "appl_support.h"

#define STORAGE_BASE_ADDRESS                    (0x00)
#define STORAGE_HEADER_VALID                    (0x1235)

#define UINT_TO_PTR(x) ((void *) ((uintptr_t) (x)))
#define PTR_TO_UINT(x) ((unsigned int) ((uintptr_t) (x)))

#define INT_TO_PTR(x) ((void *) ((intptr_t) (x)))
#define PTR_TO_INT(x) ((unsigned int) ((intptr_t) (x)))

typedef struct {
        queue_t storage;
        size_t size;
        uint16_t num_of_items;
        nvms_t nvms;
        OS_MUTEX lock;
        /* Memory is dirty required save to flash */
        bool save_required;
} appl_storage_t;

typedef struct {
        void *next;
        bool persistent;
        uint32_t flash_addr;
        storage_key_t key;
        size_t value_size;
        void *value;
} appl_storage_item_t;

typedef struct {
        uint16_t valid;
        uint16_t num_of_items;
        size_t queue_size;
} appl_storage_header_t;

typedef struct {
        appl_storage_header_t header;
        uint32_t offset;
} appl_storage_user_data_t;

PRIVILEGED_DATA appl_storage_t *appl_storage;

static uint32_t storage_save_item(appl_storage_item_t *item, uint32_t address)
{
        /* Save item key */
        ad_nvms_write(appl_storage->nvms, address, (uint8_t *)&item->key,
                sizeof(storage_key_t) + sizeof(size_t));
        address += sizeof(storage_key_t) + sizeof(size_t);

        item->flash_addr = address;
        ad_nvms_write(appl_storage->nvms, address, item->value, item->value_size);
        address += item->value_size;

        return address;
}

static void storage_foreach(void *data, void *user_data)
{
        appl_storage_item_t *item = data;
        appl_storage_user_data_t *save_data = user_data;

        if (!item->persistent) {
                return;
        }

        save_data->header.queue_size += item->value_size + sizeof(storage_key_t) + sizeof(size_t);
        save_data->header.num_of_items++;

        /* Save item key */
        save_data->offset = storage_save_item(item, save_data->offset);
}

static appl_storage_error_t appl_storage_save(void)
{
        appl_storage_user_data_t data;

        memset(&data.header, 0, sizeof(appl_storage_header_t));

        data.offset = STORAGE_BASE_ADDRESS + sizeof(appl_storage_header_t);

        queue_foreach(&appl_storage->storage, storage_foreach, &data);

        data.header.valid = STORAGE_HEADER_VALID;
        ad_nvms_write(appl_storage->nvms, STORAGE_BASE_ADDRESS, (uint8_t *)&data.header,
                sizeof(appl_storage_header_t));

        return APPL_STORAGE_OK;
}

static void initialize_storage_header(appl_storage_header_t *header)
{
        memset(header, 0, sizeof(appl_storage_header_t));
        header->valid = STORAGE_HEADER_VALID;
        ad_nvms_write(appl_storage->nvms, STORAGE_BASE_ADDRESS, (uint8_t *)header,
                sizeof(appl_storage_header_t));
}

static appl_storage_error_t appl_storage_load(void)
{
        appl_storage_header_t header;
        uint32_t offset = STORAGE_BASE_ADDRESS;
        uint16_t i;
        storage_key_t key;
        size_t value_size;
        uint8_t *buffer;
        appl_storage_item_t *new_item;

        ad_nvms_read(appl_storage->nvms, offset, (uint8_t *)&header, sizeof(appl_storage_header_t));
        offset += sizeof(appl_storage_header_t);

        if (header.valid != STORAGE_HEADER_VALID) {
                size_t part_size = ad_nvms_get_size(appl_storage->nvms);

                /*
                 * In case HEADER is invalid lets clear whole storage.
                 * It can happen when storage structure changes
                 */
                ad_nvms_erase_region(appl_storage->nvms, STORAGE_BASE_ADDRESS, part_size);

                initialize_storage_header(&header);

                return APPL_STORAGE_ERROR;
        }

        for (i = 0; i < header.num_of_items; i++) {
                ad_nvms_read(appl_storage->nvms, offset, (uint8_t *)&key, sizeof(storage_key_t));
                offset += sizeof(storage_key_t);

                ad_nvms_read(appl_storage->nvms, offset, (uint8_t *)&value_size, sizeof(size_t));
                offset += sizeof(size_t);

                size_t part_size = ad_nvms_get_size(appl_storage->nvms);
                if ((value_size == 0) || (value_size > (part_size - offset))) {
                        printf("value_size %d exceeds available space %ld of appl_storage. "
                                    "Initialising storage!\r\n", value_size, (part_size - offset));

                        ad_nvms_erase_region(appl_storage->nvms, STORAGE_BASE_ADDRESS, part_size);

                        initialize_storage_header(&header);

                        return APPL_STORAGE_ERROR;
                }

                buffer = OS_MALLOC(sizeof(uint8_t) * value_size);

                ad_nvms_read(appl_storage->nvms, offset, buffer, value_size);

                new_item = OS_MALLOC(sizeof(appl_storage_item_t));

                new_item->key = key;
                new_item->persistent = true;
                new_item->flash_addr = offset;
                new_item->value = buffer;
                new_item->value_size = value_size;

                appl_storage->size += value_size;
                appl_storage->num_of_items++;

                offset += value_size;
                queue_push_back(&appl_storage->storage, new_item);
        }

        return APPL_STORAGE_OK;
}

static bool storage_key_match(const void *elem, const void *ud)
{
        const appl_storage_item_t *item = elem;
        storage_key_t key = (uint32_t)ud;

        return (item->key == key);
}

void appl_storage_acquire(void)
{
        OS_MUTEX_GET(appl_storage->lock, OS_MUTEX_FOREVER);
}

void appl_storage_release(void)
{
        OS_MUTEX_PUT(appl_storage->lock);
}

appl_storage_error_t appl_storage_init(void)
{
        OS_BASE_TYPE status __attribute__((unused));

        if (appl_storage) {
                return APPL_STORAGE_OK;
        }

        appl_storage = OS_MALLOC(sizeof(appl_storage_t));
        appl_storage->size = 0;
        appl_storage->num_of_items = 0;

        appl_storage->nvms = ad_nvms_open(NVMS_GENERIC_PART);

        if (!appl_storage->nvms) {
                OS_FREE(appl_storage);

                return APPL_STORAGE_ERROR;
        }

        status = OS_MUTEX_CREATE(appl_storage->lock);
        OS_ASSERT(status == OS_MUTEX_CREATE_SUCCESS);

        dialog_queue_init(&appl_storage->storage);

        appl_storage_load();

        return APPL_STORAGE_OK;
}

void appl_storage_put_item_no_copy(storage_key_t key, void *item, size_t size, bool persistent)
{
        appl_storage_item_t *new_item;
        appl_storage_header_t header;
        uint32_t offset;
        appl_storage_item_t *existing_item;

        appl_storage_acquire();
        existing_item = queue_find(&appl_storage->storage, storage_key_match,
                (void *)(uint32_t)key);

        if (existing_item != NULL) {
                if (size == existing_item->value_size) {
                        OS_FREE(existing_item->value);
                        existing_item->value = item;
                        if (existing_item->persistent) {
                                ad_nvms_write(appl_storage->nvms, existing_item->flash_addr, item,
                                        size);
                        }
                        appl_storage_release();
                        return;
                } else {
                        appl_storage_release();
                        /* Avoid having duplicate keys. */
                        appl_storage_drop_item(key);
                        appl_storage_acquire();
                }
        }

        /* Create new key storage item */
        new_item = OS_MALLOC(sizeof(*new_item));
        memset(new_item, 0, sizeof(*new_item));

        new_item->key = key;
        new_item->persistent = persistent;
        new_item->value = item;
        new_item->value_size = size;

        appl_storage->size += size;
        appl_storage->num_of_items++;

        queue_push_back(&appl_storage->storage, new_item);

        if (persistent) {
                ad_nvms_read(appl_storage->nvms, STORAGE_BASE_ADDRESS, (uint8_t *)&header,
                        sizeof(appl_storage_header_t));

                offset = STORAGE_BASE_ADDRESS + header.queue_size + sizeof(appl_storage_header_t);

                storage_save_item(new_item, offset);

                header.num_of_items++;
                header.queue_size += size + sizeof(storage_key_t) + sizeof(size_t);

                ad_nvms_write(appl_storage->nvms, STORAGE_BASE_ADDRESS, (uint8_t *)&header,
                        sizeof(appl_storage_header_t));
        }

        appl_storage_release();
}

void appl_storage_put_item(storage_key_t key, void *item, size_t size, bool persistent)
{
        uint8_t *new_item = OS_MALLOC(size);

        memcpy(new_item, item, size);
        appl_storage_put_item_no_copy(key, new_item, size, persistent);
}

static void storage_destroy_item(appl_storage_item_t *item)
{
        bool persistent = false;

        persistent = item->persistent;

        appl_storage->size -= item->value_size;
        appl_storage->num_of_items--;

        OS_FREE(item->value);
        OS_FREE(item);

        if (persistent) {
                appl_storage_save();
        }
}

void appl_storage_drop_item(storage_key_t key)
{
        appl_storage_item_t *item;

        appl_storage_acquire();

        item = queue_remove(&appl_storage->storage, storage_key_match, (void *)(uint32_t)key);

        if (!item) {
                appl_storage_release();

                return;
        }

        storage_destroy_item(item);

        appl_storage_release();
}

appl_storage_error_t appl_storage_get_item(storage_key_t key, void **value, size_t *length)
{
        appl_storage_item_t *item;

        appl_storage_acquire();

        item = queue_find(&appl_storage->storage, storage_key_match, (void *)(uint32_t)key);

        if (item == NULL) {
                appl_storage_release();

                return APPL_STORAGE_NOT_FOUND;
        }

        *value = item->value;
        *length = item->value_size;

        appl_storage_release();

        return APPL_STORAGE_OK;
}

static void storage_queue_destroy_item(appl_storage_item_t *item)
{
        appl_storage->size -= item->value_size;
        appl_storage->num_of_items--;

        OS_FREE(item->value);
        OS_FREE(item);
}

void appl_storage_queue_drop_item(storage_key_t key)
{
        appl_storage_item_t *item;

        item = queue_remove(&appl_storage->storage, storage_key_match, (void *)(uint32_t)key);

        if (!item) {
                return;
        }

        if (item->persistent) {
                appl_storage->save_required = true;
        }

        storage_queue_destroy_item(item);
}

void appl_storage_queue_finalize(void)
{
        if (!appl_storage->save_required) {
                return;
        }

        if (appl_storage_save() == APPL_STORAGE_OK) {
                appl_storage->save_required = false;
        }
}

void appl_storage_put_u8(storage_key_t key, uint8_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(uint8_t), persistent);
}

void appl_storage_put_u16(storage_key_t key, uint16_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(uint16_t), persistent);
}

void appl_storage_put_u32(storage_key_t key, uint32_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(uint32_t), persistent);
}

void appl_storage_put_u64(storage_key_t key, uint64_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(uint64_t), persistent);
}

void appl_storage_put_i8(storage_key_t key, int8_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(int8_t), persistent);
}

void appl_storage_put_i16(storage_key_t key, int16_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(int16_t), persistent);
}

void appl_storage_put_i32(storage_key_t key, int32_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(int32_t), persistent);
}

void appl_storage_put_i64(storage_key_t key, int64_t value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(int64_t), persistent);
}

void appl_storage_put_float(storage_key_t key, float value, bool persistent)
{
        appl_storage_put_item(key, &value, sizeof(float), persistent);
}

appl_storage_error_t appl_storage_get_u8(storage_key_t key, uint8_t *value)
{
        uint8_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = (uint8_t)*item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_u16(storage_key_t key, uint16_t *value)
{
        uint16_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = (uint16_t)*item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_u32(storage_key_t key, uint32_t *value)
{
        uint32_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = (uint32_t)*item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_u64(storage_key_t key, uint64_t *value)
{
        uint64_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = *item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_i8(storage_key_t key, int8_t *value)
{
        int8_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = *item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_i16(storage_key_t key, int16_t *value)
{
        int16_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = *item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_i32(storage_key_t key, int32_t *value)
{
        int32_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = (int32_t)*item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_i64(storage_key_t key, int64_t *value)
{
        int64_t *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = *item;

        return APPL_STORAGE_OK;
}

appl_storage_error_t appl_storage_get_float(storage_key_t key, float *value)
{
        float *item;
        size_t len;

        if (appl_storage_get_item(key, (void **)&item, &len)) {
                return APPL_STORAGE_NOT_FOUND;
        }

        *value = (float)*item;

        return APPL_STORAGE_OK;
}

typedef struct value_match {
        uint8_t category;
        void *value;
        size_t len;
} value_match_t;

static bool storage_value_match(const void *elem, const void *ud)
{
        const appl_storage_item_t *item = elem;
        const value_match_t *vm = ud;

        /*  check if item matches search value and key is in the same category */
        if (((item->key >> 24) == vm->category) && item->value_size == vm->len) {
                if (memcmp(item->value, vm->value, vm->len) == 0) {
                        return true;
                }
        }

        return false;
}

appl_storage_error_t appl_storage_find_key_by_value(uint8_t category, void *value, size_t length,
        storage_key_t *key)
{
        appl_storage_item_t *item;
        value_match_t vm;

        appl_storage_acquire();

        /* set search info for value match */
        vm.category = category;
        vm.value = value;
        vm.len = length;

        item = queue_find(&appl_storage->storage, storage_value_match, (void *)&vm);

        if (item == NULL) {
                appl_storage_release();

                return APPL_STORAGE_NOT_FOUND;
        }

        *key = item->key;

        appl_storage_release();

        return APPL_STORAGE_OK;
}

typedef struct {
        uint8_t category;
        storage_key_t max;
} max_key_info_t;

static void max_key_foreach(void *data, void *user_data)
{
        appl_storage_item_t *item = data;
        max_key_info_t *key_info = user_data;

        /* check if item belongs to requested category */
        if ((item->key >> 24) != key_info->category)
                return;

        /*  compare max info with this item key */
        if (item->key >= key_info->max) {
                key_info->max = item->key;
        }
}

void appl_storage_find_max_key(uint8_t category, storage_key_t *max_key)
{
        max_key_info_t key_info;

        /* prepare max info for iteration */
        key_info.category = category;
        key_info.max = APPL_STORAGE_CAT(category) | 0x0;

        appl_storage_acquire();

        queue_foreach(&appl_storage->storage, max_key_foreach, &key_info);

        appl_storage_release();

        /* update requested max key value */
        *max_key = key_info.max;
}

static bool storage_category_match_reset(const void *elem, const void *ud)
{
        appl_storage_item_t *item = (appl_storage_item_t *)elem;
        uint8_t category = PTR_TO_UINT(ud);

        if ((item->key >> 24) == category) {
                /* value is about to be removed during reset,
                 * avoid calls to appl_storage_save for each value of this category
                 */
                item->persistent = false;
                return true;
        } else {
                return false;
        }
}

void appl_storage_reset(uint8_t category)
{
        printf("Hap Storage reset category %2u\r\n", category);

        appl_storage_acquire();

        queue_filter(&appl_storage->storage, storage_category_match_reset, UINT_TO_PTR(category),
                (queue_destroy_func_t)storage_destroy_item);

        /* Do one save to reflect changes for all items that were removed */
        appl_storage_save();

        appl_storage_release();
}

static void print_foreach(void *data, void *user_data)
{
        appl_storage_item_t *item = data;
        uint16_t *index = user_data;

        uint16_t i, print_size;
        uint8_t *val;

        uint8_t cat = item->key >> 24;
        uint16_t key = item->key & 0x0000FFFF;

        /* print item info */
        printf("item %2d cat %2x key %2u size %2d perm %d data:", *index, cat, key,
                item->value_size, item->persistent);

        /* print up to 4 bytes of raw data */
        print_size = item->value_size > 4 ? 4 : item->value_size;
        val = item->value;

        for (i = 0; i < print_size; i++) {
                printf(" %x", val[i]);
        }
        printf("\r\n");

        /* increment item index */
        (*index)++;
}

void appl_storage_dump(void)
{
        uint16_t index = 0;

        printf("Hap Storage size %d items %d\r\n", appl_storage->size,
                     appl_storage->num_of_items);

        appl_storage_acquire();

        queue_foreach(&appl_storage->storage, print_foreach, &index);

        appl_storage_release();
}

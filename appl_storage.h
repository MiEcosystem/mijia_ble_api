/**
 ****************************************************************************************
 *
 * @file appl_storage.h
 *
 * @brief HAP storage implementation
 *
 * Copyright (C) 2015-2018 Dialog Semiconductor.
 * This computer program includes Confidential, Proprietary Information
 * of Dialog Semiconductor. All Rights Reserved.
 *
 ****************************************************************************************
 */

#ifndef APPL_STORAGE_H_

#define APPL_STORAGE_H_

#include <appl_storage_keys.h>
#include "sdk_queue.h"

typedef void (*appl_storage_destroy_func_t)(void *data);

typedef enum {
        APPL_STORAGE_OK,
        APPL_STORAGE_ERROR,
        APPL_STORAGE_NOT_FOUND,
} appl_storage_error_t;

/**
 * Initialize HAP generic storage
 *
 * \return APPL_STORAGE_OK when initialization finished successful or APPL_STORAGE_ERROR on NVMS error
 */
appl_storage_error_t appl_storage_init(void);

/**
 * Add new item to HAP generic storage
 * Note, this function allocates memory for the item and does copy of it for storage purposes.
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   item                   value of the item
 * \param [in]   size                   size of the item value
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_item(storage_key_t key, void *item, size_t size, bool persistent);

/**
 * Add new item to HAP generic storage
 * Note, this function DOES NOT allocate memory for the item. Make sure item is on the heap.
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   item                   value of the item
 * \param [in]   size                   size of the item value
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_item_no_copy(storage_key_t key, void *item, size_t size, bool persistent);

/**
 * Remove item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 *
 */
void appl_storage_drop_item(storage_key_t key);

/**
 * Get item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 * \param [out]  length                 length of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_item(storage_key_t key, void **value, size_t *length);

/**
 * Add new uint8_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_u8(storage_key_t key, uint8_t value, bool persistent);

/**
 * Add new uint16_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_u16(storage_key_t key, uint16_t value, bool persistent);

/**
 * Add new uint32_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_u32(storage_key_t key, uint32_t value, bool persistent);

/**
 * Add new uint64_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_u64(storage_key_t key, uint64_t value, bool persistent);

/**
 * Add new int8_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_i8(storage_key_t key, int8_t value, bool persistent);

/**
 * Add new int16_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_i16(storage_key_t key, int16_t value, bool persistent);

/**
 * Add new int32_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_i32(storage_key_t key, int32_t value, bool persistent);

/**
 * Add new int64_t item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_i64(storage_key_t key, int64_t value, bool persistent);

/**
 * Add new float item to HAP generic storage
 *
 * \param [in]   key                    unique key for new item
 * \param [in]   value                  value of the item
 * \param [in]   persistent             set to true if item should be saved in flash memory
 *
 */
void appl_storage_put_float(storage_key_t key, float value, bool persistent);

/**
 * Get uint8_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_u8(storage_key_t key, uint8_t *value);

/**
 * Get uint16_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_u16(storage_key_t key, uint16_t *value);

/**
 * Get uint32_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_u32(storage_key_t key, uint32_t *value);

/**
 * Get uint64_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_u64(storage_key_t key, uint64_t *value);

/**
 * Get int8_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_i8(storage_key_t key, int8_t *value);

/**
 * Get int16_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_i16(storage_key_t key, int16_t *value);

/**
 * Get int32_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_i32(storage_key_t key, int32_t *value);

/**
 * Get int64_t item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_i64(storage_key_t key, int64_t *value);

/**
 * Get float item from HAP generic storage
 *
 * \param [in]   key                    unique item key
 * \param [out]  value                  value of the item
 *
 * \return APPL_STORAGE_OK when item found or APPL_STORAGE_NOT_FOUND when item was not found
 */
appl_storage_error_t appl_storage_get_float(storage_key_t key, float *value);

/**
 * Get key for a value that matches an item from HAP generic storage
 *
 * \param [in]  category               search for matching items in this storage category
 * \param [in]  value                  value of the item
 * \param [in]  length                 length of the item
 * \param [out] key                    unique item key for item that matches search value
 *
 * \return APPL_STORAGE_OK and update key when item found or APPL_STORAGE_NOT_FOUND when no matching
 * item was found
 */
appl_storage_error_t appl_storage_find_key_by_value(uint8_t category, void *value, size_t length,
        storage_key_t *key);

/**
 * Find max key for all items of a category from HAP generic storage
 *
 * \param [in]  category               storage category
 * \param [out] max_key                max key for this category
 *
 */
void appl_storage_find_max_key(uint8_t category, storage_key_t *max_key);

/**
 * Remove all items of a category from HAP generic storage
 *
 * \param [in]   category                storage category
 *
 */
void appl_storage_reset(uint8_t category);

/**
 * Dump items in HAP storage queue
 */
void appl_storage_dump(void);

/**
 * Hap storage acquire
 */
void appl_storage_acquire(void);

/**
 * Hap storage release
 */
void appl_storage_release(void);

/**
 * Hap storage drop item from hap storage queue. This function don't put any data to flash
 *
 * When all required item are dropped than appl_storage_queue_drop_item need to be executed
 *
 * \param [in]  key      unique item key
 */
void appl_storage_queue_drop_item(storage_key_t key);

/**
 * Push hap storage queue placed at ram to hap storage flash location
 *
 */
void appl_storage_queue_finalize();

#endif /* APPL_STORAGE_H_ */

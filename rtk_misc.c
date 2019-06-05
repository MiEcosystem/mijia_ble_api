/**
*********************************************************************************************************
*               Copyright(c) 2018, Realtek Semiconductor Corporation. All rights reserved.
*********************************************************************************************************
* @file      rtk_misc.c
* @brief     xiaomi ble misc api
* @details   Misc data types and functions.
* @author    hector_huang
* @date      2018-12-26
* @version   v1.0
* *********************************************************************************************************
*/
#include <string.h>
#include "mible_api.h"
#include "platform_misc.h"
#define MI_LOG_MODULE_NAME "RTK_MISC"
#include "mible_log.h"

extern bool aes128_ecb_encrypt(uint8_t *input, const uint8_t *key, uint8_t *output);

mible_status_t mible_rand_num_generator(uint8_t *p_buf, uint8_t len)
{
    plt_rand(p_buf, len);
    return MI_SUCCESS;
}

/**
 * @brief   Encrypts a block according to the specified parameters. 128-bit
 * AES encryption. (zero padding)
 * @param   [in] key: encryption key
 *          [in] plaintext: pointer to plain text
 *          [in] plen: plain text length
 *          [out] ciphertext: pointer to cipher text
 * @return  MI_SUCCESS              The encryption operation completed.
 *          MI_ERR_INVALID_ADDR     Invalid pointer supplied.
 *          MI_ERR_INVALID_STATE    Encryption module is not initialized.
 *          MI_ERR_INVALID_LENGTH   Length bigger than 16.
 *          MI_ERR_BUSY             Encryption module already in progress.
 * @note    SHOULD use synchronous mode to implement this function
 * */
mible_status_t mible_aes128_encrypt(const uint8_t *key,
                                    const uint8_t *plaintext, uint8_t plen, uint8_t *ciphertext)
{
    if ((NULL == key) || (NULL == plaintext) || (NULL == ciphertext))
    {
        return MI_ERR_INVALID_ADDR;
    }

    if (plen > 16)
    {
        return MI_ERR_INVALID_LENGTH;
    }

    uint8_t key_tmp[16], plaintext_tmp[16];
    memcpy(key_tmp, key, 16);
    memcpy(plaintext_tmp, plaintext, 16);
    aes128_ecb_encrypt(plaintext_tmp, key_tmp, ciphertext);

    return MI_SUCCESS;
}



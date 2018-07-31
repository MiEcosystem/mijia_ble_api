#include <stdio.h>
#include "aes.h"

void print(const uint8_t *msg, const uint8_t *buf)
{
	printf("%s", msg);
	int i;
	for(i=0; i<16; ++i)
		printf("%02x ", buf[i]);
	printf("\n");
}

int main_aes_text()
{
	uint8_t ret_text[16] = {0};
	uint8_t text[16] = {
		0x01,0x23,0x45,0x67,
		0x89,0xab,0xcd,0xef,
		0xfe,0xdc,0xba,0x98,
		0x76,0x54,0x32,0x10
	};
	uint8_t cipher_text[16] = {0};
	uint8_t key[32] = {
		0x0f,0x15,0x71,0xc9,
		0x47,0xd9,0xe8,0x59,
		0x0c,0xb7,0xad,0xd6,
		0xaf,0x7f,0x67,0x98,
		0x0f,0x15,0x71,0xc9,
		0x47,0xd9,0xe8,0x59,
		0x0c,0xb7,0xad,0xd6,
		0xaf,0x7f,0x67,0x98
	};
	
	uint32_t key_bit[3] = {128, 192, 256};
	
	aes_context ctx;
	int i;
	for (i = 0; i < sizeof(key_bit)/sizeof(key_bit[0]); ++i)
	{
		if (aes_set_key(&ctx, key, key_bit[i]) != SUCCESS)
		{
			perror("aes_set_key error.");
			return -1;
		}
		if(aes_encrypt_block(&ctx, cipher_text, text) != SUCCESS)
		{
			perror("aes_encrypt_block error.");
			return -1;
		}
		if(aes_decrypt_block(&ctx, ret_text, cipher_text) != SUCCESS)
		{
			perror("aes_decrypt_block error.");
			return -1;
		}
		printf("key_bit %d: \n", key_bit[i]);
		print("\tinput  :  ", text);
		print("\tencrypt:  ", cipher_text);
		print("\tdecrypt:  ", ret_text);
	}
	return 0;
}

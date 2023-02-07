#ifndef SM4NI_H
#define SM4NI_H

#define SM4_BLOCK_SIZE    16
#define SM4_KEY_SCHEDULE  32

#include <stdint.h>

// Encrypt 4 blocks (64 bytes) in ECB mode
void sm4_key_schedule(const uint8_t key[], uint32_t rk[]);
void sm4_encrypt4(const uint32_t rk[32], void *src, const void *dst);

#endif
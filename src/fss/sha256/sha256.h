#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t state[8];      // running hash state (a,b,c,d,e,f,g,h)
    uint8_t  buffer[64];    // current incomplete block
    uint64_t bit_len;       // total bits processed so far
    uint32_t buf_len;       // bytes currently in buffer
} SHA256_CTX;

void sha256_init(SHA256_CTX* ctx);
void sha256_update(SHA256_CTX* ctx, const uint8_t* data, size_t len);
void sha256_final(SHA256_CTX* ctx, uint8_t digest[32]);

// one-shot convenience wrapper
void sha256(const uint8_t* data, size_t len, uint8_t digest[32]);

#endif
#ifndef FSS_H
#define FSS_H

#include <stdint.h>
#include <stddef.h>
#include <gmp.h>

typedef struct {
    mpz_t p;   //safe prime p = 2q + 1
    mpz_t q;   // subgroup order
    mpz_t g;   // generator of order q  (g = 4)
} FSParams;

typedef struct {
    mpz_t x;   // private scalar x ∈ [1, q-1]
} FSSkey;

typedef struct{
    mpz_t y;
}FSPkey;

typedef struct {
    mpz_t e;   // challenge: SHA-256(R ‖ msg) mod q
    mpz_t s;   // response:  s = k − xe mod q
} FSSignature;

void fs_param_init(FSParams *params);
void fs_param_clear(FSParams *params);
void fs_skey_init(FSSkey *skey);
void fs_skey_clear(FSSkey *skey);
void fs_pkey_init(FSPkey *pkey);
void fs_pkey_clear(FSPkey *pkey);
void fs_sig_init(FSSignature *sig);
void fs_sig_clear(FSSignature *sig);

void fs_key_gen(FSParams *params, FSSkey *skey , FSPkey *pkey);
void fs_sign(FSParams *params, FSSkey *skey, const uint8_t *msg, size_t msg_len, FSSignature *sig);

//returns 1 on correct.
int fs_verify(FSParams *params, FSPkey *pkey, const uint8_t *msg, size_t msg_len, FSSignature *sig);

#endif
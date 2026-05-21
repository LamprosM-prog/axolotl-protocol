#ifndef ELGAMAL_H
#define ELGAMAL_H

#include <stdint.h>
#include <gmp.h>

typedef struct{
    mpz_t p;
    mpz_t q;
    mpz_t g;
}ElGamalParam;

typedef struct{
    mpz_t c1;
    mpz_t c2;
}ElgamalCiphertext;

void generate_param(ElGamalParam* params);
void generate_skey(mpz_t result, mpz_t q);
void generate_pkey(mpz_t result, mpz_t g, mpz_t x, mpz_t p);
void encrypt(ElgamalCiphertext* result, mpz_t msg, mpz_t pkey, ElGamalParam* params);
void decrypt(mpz_t result, ElgamalCiphertext* ct, mpz_t skey, mpz_t p);

#endif
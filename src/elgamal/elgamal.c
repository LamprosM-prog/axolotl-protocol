#include "elgamal.h"
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void generate_param(ElGamalParam* params){
    mpz_init(params->p);
    mpz_init(params->q);
    mpz_init(params->g);
    
    // known 512-bit safe prime (Which funnily enough is not secure, but usuable for our purposes.4096 bits is the standard)
    mpz_set_str(params->p, "935c5ab472131b14d56d00cac82ca313893b4b9d8a4bcb4bc5c9c91042fe2839779827849ae546d572c43fd7eaa6e99cd6814a58e21f1fc1b9fe96eb884cc293", 16);  // hex string
    // q = (p-1)/2
    mpz_sub_ui(params->q, params->p, 1);   // q = p - 1
    mpz_divexact_ui(params->q, params->q, 2); // q = q / 2   
    
    // g = 2 (valid generator for safe primes)
    
    mpz_set_ui(params->g, 2);
}

void generate_skey(mpz_t result, mpz_t q){
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    unsigned long seed;
    FILE *f = fopen("/dev/urandom", "rb");
    fread(&seed, sizeof(seed), 1, f);
    fclose(f);
    gmp_randseed_ui(state, seed);
    mpz_t range;
    mpz_init(range);
   
    mpz_sub_ui(range,q,1); //range = q-1 
    mpz_urandomm(result, state, range);
    mpz_add_ui(result, result, 1); // result = result + 1 (incase result = 0)
    

    mpz_clear(range);
    gmp_randclear(state);
}

void generate_pkey(mpz_t result, mpz_t g, mpz_t x, mpz_t p){
    mpz_powm_sec(result, g, x, p);
}

void encrypt(ElgamalCiphertext* result, mpz_t msg, mpz_t pkey, ElGamalParam* params){
    gmp_randstate_t state;
    gmp_randinit_mt(state);  
    unsigned long seed;
    FILE *f = fopen("/dev/urandom", "rb");
    fread(&seed, sizeof(seed), 1, f);
    fclose(f);
    gmp_randseed_ui(state, seed);
    mpz_t range;
    mpz_init(range);  
    mpz_t k;
    mpz_init(k);
    mpz_t temp;
    mpz_init(temp);

    // random k in (1, q-1)
    mpz_sub_ui(range,params->q,1);
    mpz_urandomm(k, state, range);
    mpz_add_ui(k,k,1);
    
    //c1 = g^k mod p
    mpz_powm_sec(result->c1 ,params->g,k,params->p);

    //c2 = msg * pkey^k mod p
    mpz_powm_sec(temp, pkey, k,params->p); // temp = pkey^k
    mpz_mul(result->c2,temp,msg); // c2 = msg * temp
    mpz_mod(result->c2, result->c2, params->p);

    mpz_clear(range);
    gmp_randclear(state);
    mpz_clear(k);
    mpz_clear(temp);
}
void decrypt(mpz_t result, ElgamalCiphertext* ct, mpz_t skey, mpz_t p){ //Elgamal!
    mpz_t s;
    mpz_init(s);
    mpz_t s_inv;
    mpz_init(s_inv);

    //s := c1 ^ x (mod p) , c1 = g^k (mod p) => c1^x = g^k*x (mod p) , g^k*x (modp) = h^x (same as encrypt) 
    mpz_powm_sec(s, ct->c1, skey, p);

    mpz_invert(s_inv, s, p);

    // m := c2 * s^-1 (mod p)
    mpz_mul(result, ct->c2,  s_inv);
    mpz_mod(result, result, p);


    mpz_clear(s);
    mpz_clear(s_inv);
}

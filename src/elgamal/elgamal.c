#include "elgamal.h"
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>




static void random_scalar(const mpz_t q, mpz_t out) {
    size_t n_bytes = (mpz_sizeinbase(q, 2) + 7) / 8;
    uint8_t *buf = malloc(n_bytes);
    FILE *f = fopen("/dev/urandom", "rb");
    do {
        fread(buf, 1, n_bytes, f);
        mpz_import(out, n_bytes, 1, 1, 0, 0, buf);
        mpz_mod(out, out, q);
    } while (mpz_cmp_ui(out, 0) == 0);
    fclose(f);
    explicit_bzero(buf, n_bytes);
    free(buf);
}




void generate_param(ElGamalParam* params){
    mpz_init(params->p);
    mpz_init(params->q);
    mpz_init(params->g);
    
    
    mpz_set_str(params->p, "c7b3e5327f524c173740a56bda653211f4e00d105111f5c361b99bbfcad8c23d6f9ddba178af1a4e2856164fdb4388b06923f0a11fe5621a4ab2511ca8136eb96858bb1da79e6b633bff89f5d63d4e53e28c92dec6ea619c5cf49b1f94feb7e1cb5353462ac4324cc6c26754bb7f4a07fe042f5063c1f6ca5f8775f1f25dc81895e4ba14ead4419bdf85a361595fee6ff3e216e4bb3c659e98188d0c5e25a22d7f8cd97376e813b568cbe077d28e309096aa9c8cdbcd68e801053f3d44a4a501d21f8d01c5e48d17426f0e35a9823432775d0ec295a8ef8d90e8cb8559f1a0257e73a107aa2aeaefb91ddd7f323ec72a7218841e0f9e1d17862f162346861efcc2f6f4357dfc42bc10d30816effc9888d8f54381a4382b492c79b829a8168fe6b3c39924cd2e799304850a6224582cd9c72526577b55abac40db6277686192d2334af3753270075cce9952ef4665cd268dfed5bda960397b7daa87bf7751ef891950b3ec7442bd0a1d73f3d53c54673228f1f7f09e27efd9ffb1165495ea117d77f68112429a7e345dd9bba4b017b5b11b4adbff06a633440e72205187993e3cca160edda0f7eeb62aff64b2947299fc407db2e3e51f440d42f388a9e696a77000f83894d1fbb91eb10bddefcfcd900c879e7c2411c6728942f215f8a68935025cd53cb5332302d6a8af0285785ade9a0a11b3c0c979218da4dc6097a53b406f", 
        16);  // hex string
    // q = (p-1)/2
    mpz_sub_ui(params->q, params->p, 1);   // q = p - 1
    mpz_divexact_ui(params->q, params->q, 2); // q = q / 2   
    
    // g = 2 (valid generator for safe primes)
    
    mpz_set_ui(params->g, 2);
}

void generate_skey(mpz_t result, mpz_t q){
    random_scalar(q, result);
}

void generate_pkey(mpz_t result, mpz_t g, mpz_t x, mpz_t p){
    mpz_powm_sec(result, g, x, p);
}





void encrypt(ElgamalCiphertext* result, mpz_t msg, mpz_t pkey, ElGamalParam* params){
    mpz_t range;
    mpz_init(range);  
    mpz_t k;
    mpz_init(k);
    mpz_t temp;
    mpz_init(temp);

    // random k in (1, q-1)
   random_scalar(params->q, k);
    //c1 = g^k mod p
    mpz_powm_sec(result->c1 ,params->g,k,params->p);

    //c2 = msg * pkey^k mod p
    mpz_powm_sec(temp, pkey, k,params->p); // temp = pkey^k
    mpz_mul(result->c2,temp,msg); // c2 = msg * temp
    mpz_mod(result->c2, result->c2, params->p);

    mpz_clear(range);
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

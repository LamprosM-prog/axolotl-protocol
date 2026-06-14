#include "sha256/sha256.h"
#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include "fss.h"
#include <strings.h>
#include <string.h>



void fs_param_init(FSParams *params) { 
    mpz_init(params->p);
    mpz_init(params->q);
    mpz_init(params->g); 

    //2048 bit prime
    mpz_set_str(params->p, "14818896a2e36d88de22eac445f898c0ccd3fd5ea3be58178ca4298ec4d1c9902eee19bdf331a6cedb2ca6b6abb42e4d8d6f1d2c6fe37e2ef91e70187d4d857f2e448044ed7a488b2b8adc644f9dc2e7f4403204726138cfe6d25f35d90c3dc784f71919b5c9cab1cfd5af5eb2fcd1e2de8b78156afbe07857e0a7fc1faebd6b40e34cddd1ca6faa0966520ae8530cdf3a050c9ee806984768bff0a26bd1f20bc4ad43220f21d08ccaf445e17468f425ecd5624c706e0f0b59eb4c8a484d148b96ad7cb6e453d41b4fbaa19c51934a78f8f26f7d5d52b1db328e7132c9e5ef1402a5201fba86c5303a466520025e6fee8448fdf1b5f8faccd8955bf0ace3c524b", 16);

    mpz_sub_ui(params->q, params->p, 1);
    mpz_divexact_ui(params->q, params->q, 2);

    mpz_set_ui(params->g, 4);

}
void fs_param_clear(FSParams *params){
    mpz_clear(params->g);
    mpz_clear(params->p);
    mpz_clear(params->q);
}
void fs_skey_init(FSSkey *skey){
    mpz_init(skey->x);
}
void fs_skey_clear(FSSkey *skey){
    mpz_clear(skey->x);
}

void fs_pkey_init(FSPkey *pkey){
    mpz_init(pkey->y); 
}
void fs_pkey_clear(FSPkey *pkey){
    mpz_clear(pkey->y);
}
void fs_sig_init(FSSignature *sig){
    mpz_init(sig->e);
    mpz_init(sig->s);
}
void fs_sig_clear(FSSignature *sig) { 
    mpz_clear(sig->e);
    mpz_clear(sig->s);
}




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


void fs_key_gen(FSParams *params, FSSkey *skey , FSPkey *pkey){
    random_scalar(params->q, skey->x);
    mpz_powm_sec(pkey->y, params->g, skey->x, params->p);
}



// e = SHA-256(R_fixed_width || msg) mod q
static void compute_challenge(const mpz_t R, const mpz_t p,
                               const uint8_t *msg, size_t msg_len,  const mpz_t q, mpz_t e)
{
    // Serialise R as fixed-width big-endian (width = |p| in bytes)
    size_t p_bytes = (mpz_sizeinbase(p, 2) + 7) / 8;
    uint8_t *r_buf = calloc(p_bytes, 1);
    size_t written;
    uint8_t *tmp = malloc(p_bytes);
    mpz_export(tmp, &written, 1, 1, 0, 0, R);
    memcpy(r_buf + (p_bytes - written), tmp, written);
    free(tmp);

    // Hash R || msg
    uint8_t digest[32];
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, r_buf, p_bytes);
    sha256_update(&ctx, msg, msg_len);
    sha256_final(&ctx, digest);
    free(r_buf);

    // e = digest mod q
    mpz_import(e, 32, 1, 1, 0, 0, digest);
    mpz_mod(e, e, q);
}



void fs_sign(FSParams *params, FSSkey *skey, const uint8_t *msg, size_t msg_len, FSSignature *sig){
    mpz_t k;
    mpz_init(k);
    mpz_t R;
    mpz_init(R);
    mpz_t xe;
    mpz_init(xe);

    random_scalar(params->q, k);

    // R = g^k (mod P)
    mpz_powm_sec(R, params->g,k,params->p); 
    //e 
    compute_challenge(R,params->p, msg, msg_len, params->q,sig->e);

    // x * e
    mpz_mul(xe, sig->e, skey->x);
    // s = (k - xe) 
    mpz_sub(sig->s, k, xe);
    
    // s mod q
    mpz_mod(sig->s, sig->s, params->q);

    explicit_bzero(mpz_limbs_modify(k, mpz_size(k)), mpz_size(k) * sizeof(mp_limb_t)); //scrubbing k
    mpz_clear(xe);
    mpz_clear(k);
    mpz_clear(R);
}

int fs_verify(FSParams *params, FSPkey *pkey, const uint8_t *msg, size_t msg_len, FSSignature *sig){
    mpz_t gr;
    mpz_init(gr);
    mpz_t yc;
    mpz_init(yc);
    mpz_t pR;
    mpz_init(pR);
    mpz_t pe;
    mpz_init(pe);

    mpz_powm_sec(gr, params->g, sig->s, params->p);
    mpz_powm_sec(yc, pkey->y, sig->e, params->p);

    mpz_mul(pR, yc, gr);

    mpz_mod(pR,pR,params->p);

    compute_challenge(pR,params->p, msg, msg_len, params->q, pe);
    mpz_clear(gr);
    mpz_clear(yc);
    mpz_clear(pR);
    int result = mpz_cmp(pe, sig->e) == 0;
    mpz_clear(pe);

    return result;
}
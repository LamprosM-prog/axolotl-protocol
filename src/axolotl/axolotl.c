#include "axolotl.h"
#include "../elgamal/elgamal.h"
#include "../rs/rs_encoder.h"
#include <stdlib.h>
#include <string.h>  
#include <sys/time.h>
#include "../rs/gf.h"
#include "../rs/rs_decode.h"

static void axolotl_encrypt(uint8_t *chunk, uint8_t *c1_out, uint8_t *c2_out,
                             mpz_t pkey, ElGamalParam *params) {
    mpz_t msg;
    mpz_init(msg);
    mpz_import(msg, RAW_CHUNK_SIZE, 1, 1, 0, 0, chunk);

    ElgamalCiphertext ct;
    mpz_init(ct.c1);
    mpz_init(ct.c2);
    encrypt(&ct, msg, pkey, params);

    // export padded to exactly 64 bytes
    size_t written;
    uint8_t tmp[64];
    memset(c1_out, 0, 64);
    mpz_export(tmp, &written, 1, 1, 0, 0, ct.c1);
    memcpy(c1_out + (64 - written), tmp, written);

    memset(c2_out, 0, 64);
    mpz_export(tmp, &written, 1, 1, 0, 0, ct.c2);
    memcpy(c2_out + (64 - written), tmp, written);

    mpz_clear(msg);
    mpz_clear(ct.c1);
    mpz_clear(ct.c2);
}

static void axolotl_decrypt(uint8_t *c1_in, uint8_t *c2_in, uint8_t *chunk_out,
 mpz_t skey, ElGamalParam *params) {
    ElgamalCiphertext ct;
    mpz_init(ct.c1);
    mpz_init(ct.c2);
    mpz_import(ct.c1, 64, 1, 1, 0, 0, c1_in);
    mpz_import(ct.c2, 64, 1, 1, 0, 0, c2_in);

    mpz_t result;
    mpz_init(result);
    decrypt(result, &ct, skey, params->p);

    memset(chunk_out, 0, RAW_CHUNK_SIZE);
    size_t written;
    mpz_export(chunk_out, &written, 1, 1, 0, 0, result);

    mpz_clear(ct.c1);
    mpz_clear(ct.c2);
    mpz_clear(result);
}





AxolotlSession *axolotl_init(uint8_t *data, uint32_t len,mpz_t pkey,
    ElGamalParam *params) {
    gf_init();
    uint32_t num_limbs = (len + RAW_CHUNK_SIZE - 1) / RAW_CHUNK_SIZE;

    AxolotlSession *sess = malloc(sizeof(AxolotlSession));
    sess->limbs          = malloc(sizeof(Limb) * num_limbs);
    sess->num_limbs      = num_limbs;
    sess->total_data_len = len;

    for (uint32_t l = 0; l < num_limbs; l++) {
        
    
        uint8_t chunk[RAW_CHUNK_SIZE] = {0};
        uint32_t offset = l * RAW_CHUNK_SIZE;
        uint32_t bytes_left = len - offset;
        uint32_t copy_len = bytes_left < RAW_CHUNK_SIZE ? bytes_left : RAW_CHUNK_SIZE;
        memcpy(chunk, &data[offset], copy_len);

        uint8_t c1[64], c2[64];
        axolotl_encrypt(chunk, c1, c2, pkey, params);

    
        uint8_t ciphertext[CIPHERTEXT_SIZE];
        memcpy(ciphertext,      c1, 64);
        memcpy(ciphertext + 64, c2, 64);

        int gen_len, codeword_len;
        uint8_t *gen = build(128, &gen_len);    
        uint8_t *codeword_ptr = encode(ciphertext, gen, 128, 128, &codeword_len);
        free(gen);
        // copy into fixed buffer then free
        uint8_t codeword[LIMB_SIZE];
        memcpy(codeword, codeword_ptr, LIMB_SIZE);
        free(codeword_ptr);

        sess->limbs[l].limb_id  = l;
        sess->limbs[l].complete = false;
        for (int p = 0; p < PACKETS_PER_LIMB; p++) {
            memcpy(sess->limbs[l].slots[p].data,
                   &codeword[p * PACKET_SIZE],
                   PACKET_SIZE);
            sess->limbs[l].slots[p].received = true;
        }
    }

    return sess;
}

int axolotl_send(int sockfd, struct sockaddr_in *dest, AxolotlSession *sess) {

    // -- HANDSHAKE --
    SessionOpen open;
    open.total_data_len = sess->total_data_len;
    open.num_limbs      = sess->num_limbs;
    sendto(sockfd, &open, sizeof(open), 0,
           (struct sockaddr *)dest, sizeof(*dest));

    // wait for SessionAck
    char ack[16];
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    struct timeval tv = {5, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int ret = recvfrom(sockfd, ack, sizeof(ack), 0,
                       (struct sockaddr *)&from, &from_len);
    if (ret <= 0) return -1;

    // -- TRANSMISSION --
    for (uint32_t l = 0; l < sess->num_limbs; l++) {
        for (int p = 0; p < PACKETS_PER_LIMB; p++) {
            sendto(sockfd, sess->limbs[l].slots[p].data, PACKET_SIZE, 0,
                   (struct sockaddr *)dest, sizeof(*dest));
        }
    }

    return 0;
}

int axolotl_recv(int sockfd, uint8_t *out_buf, uint32_t *out_len, mpz_t skey, ElGamalParam *params) {


    SessionOpen open;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct timeval tv = {0, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    recvfrom(sockfd, &open, sizeof(open), 0,(struct sockaddr *)&client_addr, &client_len);


    sendto(sockfd, "ACK", 3, 0,
           (struct sockaddr *)&client_addr, client_len);

    
    Limb *limbs = malloc(sizeof(Limb) * open.num_limbs);


    tv.tv_sec  = 0;
    tv.tv_usec = 500000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));


    for (uint32_t l = 0; l < open.num_limbs; l++) {
        limbs[l].limb_id  = l;
        limbs[l].complete = false;

        for (int p = 0; p < PACKETS_PER_LIMB; p++) {
            int ret = recvfrom(sockfd, limbs[l].slots[p].data, PACKET_SIZE, 0,
                               (struct sockaddr *)&client_addr, &client_len);
            if (ret > 0) {
                limbs[l].slots[p].received = true;
            } else {
                memset(limbs[l].slots[p].data, 0, PACKET_SIZE);
                limbs[l].slots[p].received = false;  // known erasure
            }
        }
        limbs[l].complete = true;
    }

    *out_len = 0;
    for (uint32_t l = 0; l < open.num_limbs; l++) {

        // reassemble codeword from slots
        uint8_t codeword[LIMB_SIZE];
        for (int p = 0; p < PACKETS_PER_LIMB; p++) {
            memcpy(&codeword[p * PACKET_SIZE],
                   limbs[l].slots[p].data,
                   PACKET_SIZE);
        }

        

        uint8_t *syn = compute_syndromes(codeword, 256, 128);
        if (check_errors(syn, 128)) {
            int bm_len, err_count;
            uint8_t *lambda = berlekamp_massey(syn, 128, &bm_len);
            int *pos = chien_search(lambda, bm_len, 256, &err_count);
            forney(lambda, bm_len, syn, 128, codeword, 256, pos, err_count);
            free(lambda);
            free(pos);
        }
        free(syn);

        // extract c1, c2
        uint8_t c1[64], c2[64];
        memcpy(c1, codeword,      64);
        memcpy(c2, codeword + 64, 64);

        // ElGamal decrypt
        uint8_t chunk[RAW_CHUNK_SIZE];
        axolotl_decrypt(c1, c2, chunk, skey, params);


        memcpy(out_buf + *out_len, chunk, RAW_CHUNK_SIZE);
        *out_len += RAW_CHUNK_SIZE;
    }

    // strip to actual length
    *out_len = open.total_data_len;

    free(limbs);
    return 0;
}

void axolotl_free(AxolotlSession *sess) {
    free(sess->limbs);
    free(sess);
}
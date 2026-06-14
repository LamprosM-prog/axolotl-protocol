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

    // export padded to exactly c1_size bytes
    size_t written;
    uint8_t tmp[C1_SIZE];
    memset(c1_out, 0, C1_SIZE);
    mpz_export(tmp, &written, 1, 1, 0, 0, ct.c1);
    memcpy(c1_out + (C1_SIZE - written), tmp, written);

    memset(c2_out, 0, C2_SIZE);
    mpz_export(tmp, &written, 1, 1, 0, 0, ct.c2);
    memcpy(c2_out + (C2_SIZE - written), tmp, written);

    mpz_clear(msg);
    mpz_clear(ct.c1);
    mpz_clear(ct.c2);
}

static void axolotl_decrypt(uint8_t *c1_in, uint8_t *c2_in, uint8_t *chunk_out,
 mpz_t skey, ElGamalParam *params) {
    ElgamalCiphertext ct;
    mpz_init(ct.c1);
    mpz_init(ct.c2);
    mpz_import(ct.c1, C1_SIZE, 1, 1, 0, 0, c1_in);
    mpz_import(ct.c2, C2_SIZE, 1, 1, 0, 0, c2_in);

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
    ElGamalParam *params, FSParams *fss_params, FSSkey *fss_skey) {
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

        uint8_t c1[C1_SIZE], c2[C2_SIZE];
        axolotl_encrypt(chunk, c1, c2, pkey, params);
    
        uint8_t ciphertext[C1_SIZE + C2_SIZE];
        memcpy(ciphertext, c1, C1_SIZE);
        memcpy(ciphertext + C1_SIZE, c2, C1_SIZE);


        //Encrypt-then-sign scheme
        FSSignature sig;
        fs_sig_init(&sig);  
        
        fs_sign(fss_params,fss_skey, ciphertext, (C1_SIZE+C2_SIZE), &sig);
        
        //32 for e and 256 for s
        size_t written_e;
        size_t written_s;
        uint8_t e[E_SIZE]; //32
        uint8_t s[S_SIZE]; //256
        uint8_t tmp_e[E_SIZE];
        uint8_t tmp_s[S_SIZE];
        memset(e, 0, E_SIZE);
        mpz_export(tmp_e, &written_e, 1, 1, 0, 0, sig.e);
        memcpy(e + (E_SIZE - written_e), tmp_e, written_e);
        memset(s, 0, S_SIZE);
        mpz_export(tmp_s, &written_s, 1,1,0,0, sig.s);
        memcpy(s + (S_SIZE - written_s), tmp_s, written_s);

        uint8_t frame[DATA_BYTES];
        memcpy(frame, c1, C1_SIZE); // 512
        memcpy(frame + 512, c2, C2_SIZE); // 512
        memcpy(frame + 1024, e, E_SIZE); // 32
        memcpy(frame + 1056, s, S_SIZE); // 256
        memset(frame + 1312, 0, PADDING_SIZE);  // 288

        uint16_t symbols[800];
        for (int i = 0; i < 800; i++){
            symbols[i] = ((uint16_t)frame[i*2] << 8) | frame[i*2+1];
        }

        int gen_len, codeword_len;
        uint16_t *gen = build(800, &gen_len);    
        uint16_t *codeword_ptr = encode(symbols, gen, 800, 800, &codeword_len);
        free(gen);

        uint8_t codeword[LIMB_SIZE];
        for (int i = 0; i < 1600; i++) {
            codeword[i*2]   = (codeword_ptr[i] >> 8) & 0xFF;
            codeword[i*2+1] =  codeword_ptr[i] & 0xFF;
        }

        free(codeword_ptr);
        fs_sig_clear(&sig);

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

int axolotl_recv(int sockfd, uint8_t *out_buf, uint32_t *out_len, mpz_t skey, ElGamalParam *params,FSParams *fss_params, FSPkey *fss_pkey, LimbStatus **limb_statuses){


    SessionOpen open;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct timeval tv = {0, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    recvfrom(sockfd, &open, sizeof(open), 0,(struct sockaddr *)&client_addr, &client_len);


    sendto(sockfd, "ACK", 3, 0,
           (struct sockaddr *)&client_addr, client_len);

    
    Limb *limbs = malloc(sizeof(Limb) * open.num_limbs);
    *limb_statuses = malloc(sizeof(LimbStatus) * open.num_limbs);

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
                limbs[l].slots[p].received = false;  
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

        uint16_t sym_in[1600];
        for (int i = 0; i < 1600; i++) {
            sym_in[i] = ((uint16_t)codeword[i*2] << 8) | codeword[i*2+1];
        }
        

        uint16_t *syn = compute_syndromes(sym_in, 1600, 800);
        if (check_errors(syn, 800)) {
            int bm_len, err_count;
            uint16_t *lambda = berlekamp_massey(syn, 800, &bm_len);
            int *pos = chien_search(lambda, bm_len, 1600, &err_count);
            if(err_count < 0 || err_count > 400){
                (*limb_statuses)[l] = LIMB_LOST;
                free(syn);
                free(lambda);
                free(pos);
                continue;
            }
            
            
            uint16_t* fixed = forney(lambda, bm_len, syn, 800, sym_in, 1600, pos, err_count);
            memcpy(sym_in, fixed, 1600 * sizeof(uint16_t));
            
            free(fixed);
            free(lambda);
            free(pos);
        }
        free(syn);
        
        uint8_t frame[DATA_BYTES];
        for (int i = 0; i < 800; i++) {
            frame[i*2] =(sym_in[i] >> 8) & 0xFF;
            frame[i*2+1] = sym_in[i] & 0xFF;
        }

        uint8_t c1[C1_SIZE], c2[C2_SIZE];
        memcpy(c1, frame, C1_SIZE);
        memcpy(c2, frame + 512, C2_SIZE);

        uint8_t ciphertext[C1_SIZE + C2_SIZE];
        memcpy(ciphertext, c1, C1_SIZE);
        memcpy(ciphertext + C1_SIZE, c2, C2_SIZE);

        uint8_t e[E_SIZE], s[S_SIZE];
        memcpy(e, frame + 1024, E_SIZE);
        memcpy(s, frame + 1056, S_SIZE);

        FSSignature sig;
        fs_sig_init(&sig);
        mpz_import(sig.e, E_SIZE, 1, 1, 0, 0, e);
        mpz_import(sig.s, S_SIZE, 1, 1, 0, 0, s);

        if (!fs_verify(fss_params, fss_pkey, ciphertext, C1_SIZE + C2_SIZE, &sig)) {
            (*limb_statuses)[l] = LIMB_TAMPERED;
            fs_sig_clear(&sig);
            continue;
        }
        fs_sig_clear(&sig);


        uint8_t chunk[RAW_CHUNK_SIZE];
        axolotl_decrypt(c1, c2, chunk, skey, params);
        memcpy(out_buf + *out_len, chunk, RAW_CHUNK_SIZE);
        *out_len += RAW_CHUNK_SIZE;
        (*limb_statuses)[l] = LIMB_OK;
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
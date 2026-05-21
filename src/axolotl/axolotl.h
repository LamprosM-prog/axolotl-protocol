#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "../elgamal/elgamal.h"

#define PACKET_SIZE       16
#define PACKETS_PER_LIMB  16
#define LIMB_SIZE         256
#define RAW_CHUNK_SIZE    32
#define CIPHERTEXT_SIZE   128

typedef struct {
    uint8_t data[PACKET_SIZE];
    bool    received;
} PacketSlot;

typedef struct {
    PacketSlot slots[PACKETS_PER_LIMB];
    uint8_t    limb_id;
    bool       complete;
} Limb;

typedef struct {
    Limb        *limbs;
    uint32_t     num_limbs;
    uint32_t     total_data_len;
} AxolotlSession;

typedef struct {
    uint32_t total_data_len;
    uint32_t num_limbs;
} SessionOpen;

// API
AxolotlSession *axolotl_init(uint8_t *data, uint32_t len, mpz_t pkey, ElGamalParam *params);
int axolotl_send(int sockfd, struct sockaddr_in *dest, AxolotlSession *sess);
int axolotl_recv(int sockfd, uint8_t *out_buf, uint32_t *out_len, mpz_t skey, ElGamalParam *params);
void axolotl_free(AxolotlSession *sess);
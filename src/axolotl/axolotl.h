#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <netinet/in.h>
#include "../elgamal/elgamal.h"
#include "../fss/fss.h"

#define RAW_CHUNK_SIZE   32
#define C1_SIZE          512
#define C2_SIZE          512
#define E_SIZE           32
#define S_SIZE           256
#define PADDING_SIZE     288
#define DATA_BYTES       1600   // 1000 symbols * 2 bytes
#define LIMB_SIZE        3600   // 1800 symbols * 2 bytes
#define PACKETS_PER_LIMB 36
#define PACKET_SIZE      100

typedef enum {
    LIMB_OK,
    LIMB_LOST,
    LIMB_TAMPERED,
} LimbStatus;

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
AxolotlSession *axolotl_init(uint8_t *data, uint32_t len, mpz_t pkey, ElGamalParam *params, FSParams *fss_params, FSSkey *fss_skey);
int axolotl_send(int sockfd, struct sockaddr_in *dest, AxolotlSession *sess);
int axolotl_recv(int sockfd, uint8_t *out_buf, uint32_t *out_len, mpz_t skey, ElGamalParam *params,FSParams *fss_params, FSPkey *fss_pkey, LimbStatus **limb_statuses);
void axolotl_free(AxolotlSession *sess);
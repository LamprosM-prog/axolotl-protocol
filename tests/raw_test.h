#pragma once
#include <stdint.h>

#define PACKET_SIZE 100
#define PACKETS_PER_LIMB 32
#define PORT 9001   // different from axolotl 9000

typedef struct {
    uint32_t total_data_len;
    uint32_t num_limbs;
} SessionOpen;
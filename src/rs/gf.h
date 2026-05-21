#ifndef GF_H
#define GF_H

#include <stdint.h>

void gf_init();
uint8_t gf_add(uint8_t a, uint8_t b);
uint8_t gf_multiply(uint8_t a, uint8_t b);
uint8_t gf_divide(uint8_t a, uint8_t b);
uint8_t gf_inverse(uint8_t a);
uint8_t gf_generator(int i);


#endif
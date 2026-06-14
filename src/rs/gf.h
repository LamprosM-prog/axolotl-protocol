#ifndef GF_H
#define GF_H

#include <stdint.h>

void gf_init();
uint16_t gf_add(uint16_t a, uint16_t b);
uint16_t gf_multiply(uint16_t a, uint16_t b);
uint16_t gf_divide(uint16_t a, uint16_t b);
uint16_t gf_inverse(uint16_t a);
uint16_t gf_generator(int i);


#endif
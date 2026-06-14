#ifndef  POLY_H
#define POLY_H

#include <stdint.h>
//highest degree first! a^x .. a0

uint16_t* poly_add(uint16_t* a, int a_len, uint16_t* b, int b_len, int* out_len);
uint16_t* poly_multiply(uint16_t* a, int a_len, uint16_t* b, int b_len, int* out_len);
uint16_t poly_evaluate(uint16_t* poly , int len, uint16_t x );
uint16_t* poly_derivate(uint16_t* poly, int len , int* out_len);
uint16_t* poly_divide_remainder(uint16_t* a, int a_len, uint16_t* b, int b_len, int* out_len);
uint16_t* poly_trim(uint16_t* poly, int len, int* out_len);



#endif
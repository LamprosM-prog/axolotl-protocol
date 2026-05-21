#ifndef  POLY_H
#define POLY_H

#include <stdint.h>
//highest degree first! a^x .. a0

uint8_t* poly_add(uint8_t* a, int a_len, uint8_t* b, int b_len, int* out_len);
uint8_t* poly_multiply(uint8_t* a, int a_len, uint8_t* b, int b_len, int* out_len);
uint8_t poly_evaluate(uint8_t* poly , int len, uint8_t x );
uint8_t* poly_derivate(uint8_t* poly, int len , int* out_len);
uint8_t* poly_divide_remainder(uint8_t* a, int a_len, uint8_t* b, int b_len, int* out_len);
uint8_t* poly_trim(uint8_t* poly, int len, int* out_len);



#endif
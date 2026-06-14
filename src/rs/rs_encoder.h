#ifndef RS_ENCODER
#define RS_ENCODER

#include <stdint.h>
//Builds the generator polynomial
//heap-allocated caller frees
uint16_t*  build(int eccLength, int* out_len);
//Appends eccLength codes in message using reedsolomon
uint16_t*  encode(uint16_t* message,uint16_t* gen_poly, int msg_len, int eccLength, int* out_len);
#endif  
#include "gf.h"
#include "poly.h"
#include "rs_encoder.h"
#include <stdlib.h>
#include <string.h>

uint8_t* build(int eccLength, int* out_len)
{
    uint8_t* g = malloc(1);
    g[0] = 1;
    int g_len = 1;

    for (int i = 1; i <= eccLength; i++)
    {
        uint8_t term[2];
        term[0] = 1;
        term[1] = gf_generator(i); 

        int new_len = 0;

        uint8_t* new_g = poly_multiply(g, g_len,term, 2,&new_len);

        free(g);
        g = new_g;
        g_len = new_len;
    }

    *out_len = g_len;
    return g;
}

uint8_t*  encode(uint8_t* message,uint8_t* gen_poly, int msg_len, int eccLength, int* out_len){  // Reed Solomon!
    *out_len = msg_len+eccLength;
    uint8_t* codeword = calloc(*out_len, 1);
    for(int i =0; i < msg_len; i++){
        codeword[i] = message[i];
    }
    int rem_len;
    uint8_t* remainder = poly_divide_remainder(codeword, *out_len, gen_poly, eccLength + 1, &rem_len);
    
    // append remainder as ECC symbols
    for(int i = 0; i < eccLength; i++) {
        codeword[msg_len + i] = remainder[i];
    }
    free(remainder);
    return codeword;
}
#include "../rs/gf.h" 
#include <stdio.h>

static uint8_t log_table[256];
static uint8_t exp_table[512];

void gf_init(){
    unsigned int value = 1;
    for(int i=0; i < 255; i++){
        exp_table[i] = (uint8_t)value;
        log_table[value] = (uint8_t)i;
        value <<= 1; 
        if(value >= 256){
            value ^= 0x11D; 
        }
    }
    for(int i = 255; i < 512; i++){
        exp_table[i] = exp_table[i - 255];
    }
}

uint8_t gf_add(uint8_t a, uint8_t b) {
    return a ^ b; 
}

uint8_t gf_multiply(uint8_t a, uint8_t b){
    if(a==0 || b==0){
        return 0;
    }
    return exp_table[log_table[a] + log_table[b]];
}

uint8_t gf_divide(uint8_t a, uint8_t b){
    // a/b  a = dividend , b = divisor
    if( b == 0){
        printf("Division by 0. Expect Unexpected Behavior\n"); 
        //I know this doesn't stop the division
        // but division by 0 won't really happen             
    }    
    if(a==0){
        return 0;
    }
    return exp_table[log_table[a] - log_table[b] + 255]; // I think thats supposed to be 255
}

uint8_t gf_inverse(uint8_t a){
    if(a==0){
        printf("No inverse for 0. Expect Unexpected Behavior\n");   
    }
    return exp_table[255 - log_table[a]];
}
uint8_t gf_generator(int i){
    return exp_table[i % 255];
}

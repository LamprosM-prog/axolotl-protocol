#include "gf.h" 
#include <stdio.h>

static uint16_t  log_table[65536];
static uint16_t  exp_table[131070];

void gf_init(){
    unsigned int value = 1;
    for(int i=0; i < 65535; i++){
        exp_table[i] = (uint16_t)value;
        log_table[value] = (uint16_t)i;
        value <<= 1; 
        if(value >= 65536){
            value ^= 0x1100B; 
        }
    }
    for(int i = 65535; i < 131070; i++){
        exp_table[i] = exp_table[i - 65535];
    }
}

uint16_t gf_add(uint16_t a, uint16_t b) {
    return a ^ b; 
}

uint16_t gf_multiply(uint16_t a, uint16_t b){
    if(a==0 || b==0){
        return 0;
    }
    return exp_table[log_table[a] + log_table[b]];
}

uint16_t gf_divide(uint16_t a, uint16_t b){
    // a/b  a = dividend , b = divisor
    if( b == 0){
        printf("Division by 0. Expect Unexpected Behavior\n"); 
        //I know this doesn't stop the division
        // but division by 0 won't really happen             
    }    
    if(a==0){
        return 0;
    }
    return exp_table[log_table[a] - log_table[b] + 65535]; // I think thats supposed to be 255
}

uint16_t gf_inverse(uint16_t a){
    if(a==0){
        printf("No inverse for 0. Expect Unexpected Behavior\n");   
    }
    return exp_table[65535 - log_table[a]];
}
uint16_t gf_generator(int i){
    return exp_table[i % 65535];
}

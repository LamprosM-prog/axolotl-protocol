    #include "poly.h"
    #include "gf.h"
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>

    uint16_t* poly_trim(uint16_t* poly, int len, int* out_len){
        int i = 0;  
        while(i < len && poly[i] == 0 ){
            i++; 
        }
        *out_len = len - i;
        uint16_t* result = malloc(*out_len *sizeof (uint16_t));
        memcpy (result, poly + i, *out_len *sizeof(uint16_t));
        return result;
    }

    uint16_t* poly_add(uint16_t* a, int a_len, uint16_t* b, int b_len , int* out_len){
        if(a_len >= b_len){
            *out_len = a_len;
        }else if (a_len < b_len)
        {
            *out_len = b_len;
        }
        uint16_t* result = malloc(*out_len *sizeof (uint16_t));
        int a_index = a_len - 1;
        int b_index = b_len - 1;
        int result_index = *out_len - 1;
        while(result_index >= 0){
            uint16_t av = (a_index >= 0 && a_index < a_len) ? a[a_index] : 0;
            uint16_t bv = (b_index >= 0 && b_index < b_len) ? b[b_index] : 0;
            result[result_index]= gf_add(av,bv);

            a_index--;
            b_index--;
            result_index--;
        }
        uint16_t* trimmed = poly_trim(result, *out_len, out_len);
        free(result);
        return trimmed;
    }

    uint16_t* poly_multiply(uint16_t* a, int a_len, uint16_t* b, int b_len ,int* out_len){
        *out_len = a_len + b_len - 1;
        int a_index = a_len - 1;
        int b_index = b_len - 1;
        uint16_t* result = calloc(*out_len, sizeof(uint16_t));
        for(int i= a_index; i >=0; i--){
            for(int j = b_index; j>=0; j--){
                int position = i+j;
                result[position] = gf_add(result[position], gf_multiply(a[i],b[j]));

            }
        }
        uint16_t* trimmed = poly_trim(result, *out_len, out_len);
        free(result);
        return trimmed;
    }
    uint16_t poly_evaluate(uint16_t* poly, int len, uint16_t x){  //Horner!
        uint16_t result = 0;
        for(int i = 0; i < len; i++){
            result = gf_multiply(result, x);
            result = gf_add(result, poly[i]);
        }
        return result;
    }

    uint16_t* poly_derivate(uint16_t* poly, int len , int* out_len){
        if(len <= 1){
            *out_len = 1;
            uint16_t* zero = calloc(1,sizeof(uint16_t)); 
            
            return zero;
        }
        *out_len = len - 1;
        uint16_t* result = calloc(*out_len, sizeof(uint16_t));
        int degree = len - 1;

        for(int i = 0 ; i < len-1; i++){
            int power = degree - i;
            if(power%2 == 1){
                result[i] = poly[i];
            }
        }
        uint16_t* trimmed = poly_trim(result, *out_len, out_len);
        free(result);
        return trimmed;
    }
    // a is the dividend  and b is the devisor (a/b)
    uint16_t* poly_divide_remainder(uint16_t* a, int a_len, uint16_t* b, int b_len, int* out_len) {
        int current_len = a_len;
        int start = 0;
        uint16_t* work = malloc(a_len * sizeof(uint16_t));
        memcpy(work, a, a_len * sizeof(uint16_t));

        while(current_len - 1 >= b_len - 1) {
            uint16_t factor = gf_multiply(work[start], gf_inverse(b[0])); //satanic black magic
                for(int i = 0; i < b_len; i++) {
                work[start + i] = gf_add(work[start + i], gf_multiply(b[i], factor));
            }
              
             // skip leading zeros
            while(current_len > 1 && work[start] == 0) {
                start++;
                current_len--;
            }
        }
        
        // copy remainder into clean result
        uint16_t* result = malloc( current_len * sizeof(uint16_t) );
        memcpy(result, work + start, current_len * sizeof(uint16_t));
        *out_len = current_len;
        free(work);
        return result;
    }
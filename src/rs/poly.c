    #include "../rs/poly.h"
    #include "../rs/gf.h"
    #include <stdio.h>
    #include <string.h>
    #include <stdlib.h>

    uint8_t* poly_trim(uint8_t* poly, int len, int* out_len){
        int i = 0;  
        while(i < len && poly[i] == 0 ){
            i++; 
        }
        *out_len = len - i;
        uint8_t* result = malloc(*out_len);
        memcpy (result, poly + i, *out_len);
        return result;
    }

    uint8_t* poly_add(uint8_t* a, int a_len, uint8_t* b, int b_len , int* out_len){
        if(a_len >= b_len){
            *out_len = a_len;
        }else if (a_len < b_len)
        {
            *out_len = b_len;
        }
        uint8_t* result = malloc(*out_len);
        int a_index = a_len - 1;
        int b_index = b_len - 1;
        int result_index = *out_len - 1;
        while(result_index >= 0){
            uint8_t av = (a_index >= 0 && a_index < a_len) ? a[a_index] : 0;
            uint8_t bv = (b_index >= 0 && b_index < b_len) ? b[b_index] : 0;
            result[result_index]= gf_add(av,bv);

            a_index--;
            b_index--;
            result_index--;
        }
        uint8_t* trimmed = poly_trim(result, *out_len, out_len);
        free(result);
        return trimmed;
    }

    uint8_t* poly_multiply(uint8_t* a, int a_len, uint8_t* b, int b_len ,int* out_len){
        *out_len = a_len + b_len - 1;
        int a_index = a_len - 1;
        int b_index = b_len - 1;
        uint8_t* result = calloc(*out_len, 1);
        for(int i= a_index; i >=0; i--){
            for(int j = b_index; j>=0; j--){
                int position = i+j;
                result[position] = gf_add(result[position], gf_multiply(a[i],b[j]));

            }
        }
        uint8_t* trimmed = poly_trim(result, *out_len, out_len);
        free(result);
        return trimmed;
    }
    uint8_t poly_evaluate(uint8_t* poly, int len, uint8_t x){  //Horner!
        uint8_t result = 0;
        for(int i = 0; i < len; i++){
            result = gf_multiply(result, x);
            result = gf_add(result, poly[i]);
        }
        return result;
    }

    uint8_t* poly_derivate(uint8_t* poly, int len , int* out_len){
        if(len <= 1){
            *out_len = 1;
            uint8_t* zero = calloc(1,1); 
            
            return zero;
        }
        *out_len = len - 1;
        uint8_t* result = calloc(*out_len, 1);
        int degree = len - 1;

        for(int i = 0 ; i < len-1; i++){
            int power = degree - i;
            if(power%2 == 1){
                result[i] = poly[i];
            }
        }
        uint8_t* trimmed = poly_trim(result, *out_len, out_len);
        free(result);
        return trimmed;
    }
    // a is the dividend  and b is the devisor (a/b)
    uint8_t* poly_divide_remainder(uint8_t* a, int a_len, uint8_t* b, int b_len, int* out_len) {
        int current_len = a_len;
        int start = 0;
        uint8_t* work = malloc(a_len);
        memcpy(work, a, a_len);

        while(current_len - 1 >= b_len - 1) {
            uint8_t factor = gf_multiply(work[start], gf_inverse(b[0])); //satanic black magic
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
        uint8_t* result = malloc(current_len);
        memcpy(result, work + start, current_len);
        *out_len = current_len;
        free(work);
        return result;
    }
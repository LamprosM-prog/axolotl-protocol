#ifndef DECODE_H
#define DECODE_H

#include <stdint.h>

// Computes 2t syndromes by evaluating the codeword at α^1 ... α^(2t)
// Returns heap-allocated array of length ecc_len, caller frees
uint16_t* compute_syndromes(uint16_t* codeword, int codeword_len, int ecc_len);

// Returns 1 if any syndrome is nonzero (errors present), 0 if clean
int check_errors(uint16_t* syndromes, int ecc_len);

// Berlekamp-Massey: returns error locator polynomial λ(x)
// highest-degree-first, caller frees, degree written to out_len
uint16_t* berlekamp_massey(uint16_t* syndromes, int ecc_len, int* out_len);

// Chien search: finds roots of λ(x), maps to error positions in codeword
// returns heap-allocated position list, count written to out_count
int* chien_search(uint16_t* lambda, int lambda_len, int codeword_len, int* out_count);

// Forney: computes error magnitudes and corrects codeword in place
// returns the corrected codeword (same pointer as input)
uint16_t* forney(uint16_t* lambda, int lambda_len,uint16_t* syndromes, int ecc_len,uint16_t* codeword, int codeword_len,int* error_positions, int error_count);

#endif
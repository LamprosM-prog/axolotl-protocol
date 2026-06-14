#include "rs_decode.h"
#include "gf.h"
#include "poly.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

uint16_t *compute_syndromes(uint16_t *codeword, int codeword_len, int ecc_len)
{
    uint16_t *syndromes = malloc(ecc_len * sizeof(uint16_t));
    for (int i = 0; i < ecc_len; i++)
    {
        uint16_t x = gf_generator(i + 1); // prime element
        uint16_t result = 0;
        for (int j = 0; j < codeword_len; j++)
        {
            result = gf_add(gf_multiply(result, x), codeword[j]);
        }
        syndromes[i] = result;
    }
    return syndromes;
}

int check_errors(uint16_t *syndromes, int ecc_len)
{
    for (int i = 0; i < ecc_len; i++)
    {
        if (syndromes[i] != 0)
            return 1;
    }
    return 0;
}

// lowest index = constant term
// this is because its how I did it in C#
uint16_t *berlekamp_massey(uint16_t *syndromes, int ecc_len, int *out_len)
{
    int size = ecc_len + 1;
    uint16_t *lambda = calloc(size, sizeof(uint16_t));
    uint16_t *bestL = calloc(size, sizeof(uint16_t));
    lambda[0] = 1;
    bestL[0] = 1;
    uint16_t b = 1;
    int degree = 0;
    int m = -1;
    for (int k = 0; k < ecc_len; k++)
    {
        // 1. compute delta (discrepancy)
        uint16_t delta = syndromes[k];
        for (int i = 1; i <= degree; i++)
        {
            delta = gf_add(delta, gf_multiply(lambda[i], syndromes[k - i]));
        }

        if (delta != 0)
        {
            //
            uint16_t *temp = malloc(size * sizeof(uint16_t));
            memcpy(temp, lambda, size);

            int shift = k - m;
            // λ = λ - (δ/b) * x^shift * B
            for (int j = 0; j < size; j++)
            {
                int target = j + shift;
                if (target < size)
                {
                    lambda[target] =
                        gf_add(lambda[target],
                               gf_multiply(gf_divide(delta, b), bestL[j]));
                }
            }

            if (2 * degree <= k)
            {
                degree = k + 1 - degree;
                free(bestL);
                bestL = temp;
                b = delta;
                m = k;
            }
            else
            {
                free(temp);
            }
        }
    }

    int last = size - 1;
    while (last > 0 && lambda[last] == 0)
        last--;

    // reverse to highest-first before returning
    uint16_t *result = malloc(last + 1 * sizeof(uint16_t));
    for (int i = 0; i <= last; i++)
    {
        result[i] = lambda[last - i];
    }
    *out_len = last + 1;

    free(lambda);
    free(bestL);
    return result;
}

int *chien_search(uint16_t *lambda, int lambda_len, int codeword_len,
                  int *out_count)
{
    int *positions =
        malloc((lambda_len - 1) * sizeof(int)); // at most degree(λ) errors
    *out_count = 0;
    for (int i = 0; i < 65535; i++)
    {
        uint16_t x = gf_generator(i);
        uint16_t eval = poly_evaluate(lambda, lambda_len, x);
        if (eval == 0)
        {
            int locator_exp = 65535 - i;
            int position = (codeword_len - 1) - locator_exp;
            if (position >= 0 && position < codeword_len)
            {
                positions[*out_count] = position;
                (*out_count)++;
            }
        }
    }

    return positions;
}
uint16_t *forney(uint16_t *lambda, int lambda_len, uint16_t *syndromes,
                int ecc_len, uint16_t *codeword, int codeword_len,
                int *error_positions, int error_count)
{
    uint16_t *syndromes_poly = malloc(ecc_len * sizeof(uint16_t));
    for (int i = 0; i < ecc_len; i++)
    {
        syndromes_poly[i] = syndromes[ecc_len - 1 - i];
    }
    int out_len_omega;
    int lambda_len_der;
    uint16_t *omega = poly_multiply(syndromes_poly, ecc_len, lambda, lambda_len,
                                   &out_len_omega); // heap-allocated
    uint16_t *lambda_der = poly_derivate(lambda, lambda_len, &lambda_len_der);
    uint16_t *omaga_scaled;
    int omega_scaled_len;
    if (out_len_omega > ecc_len)
    {
        omaga_scaled = malloc(ecc_len);
        omega_scaled_len = ecc_len;
        for (int i = 0; i < ecc_len; i++)
        {
            omaga_scaled[i] =
                omega[out_len_omega - ecc_len + i]; // take LAST ecc_len terms
        }
    }
    else
    {
        omaga_scaled = omega;
        omega_scaled_len = out_len_omega;
    }

    for (int i = 0; i < error_count; i++)
    {
        int locator_exp = (codeword_len - 1) - error_positions[i];
        uint16_t xk = gf_generator(locator_exp);
        uint16_t xk_inv = gf_inverse(xk);
        uint16_t num = poly_evaluate(omaga_scaled, omega_scaled_len, xk_inv);
        uint16_t denom = poly_evaluate(lambda_der, lambda_len_der, xk_inv);
        uint16_t error_mang = gf_divide(num, denom);
        codeword[error_positions[i]] =
            gf_add(codeword[error_positions[i]], error_mang);
    }
    if (omaga_scaled != omega)
        free(omaga_scaled);
    free(omega);
    free(lambda_der);
    free(syndromes_poly);
    return codeword;
}

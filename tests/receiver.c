#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include "axolotl.h"

#define PORT 9000

int main() {
    // ElGamal: receiver generates, gives pkey to sender 
    ElGamalParam params;
    mpz_init(params.p);
    mpz_init(params.q);
    mpz_init(params.g);
    generate_param(&params);

    mpz_t skey, pkey;
    mpz_init(skey);
    mpz_init(pkey);
    generate_skey(skey, params.q);
    generate_pkey(pkey, params.g, skey, params.p);

    printf("=== ElGamal pkey (paste into sender) ===\n");
    gmp_printf("%Zx\n\n", pkey);

    //FSS: sender generates, we wait for params+pkey here 
    FSParams fss_params;
    FSPkey   fss_pkey;
    fs_param_init(&fss_params);
    fs_pkey_init(&fss_pkey);

    printf("=== Paste sender's FSS params+pkey (p q g y, space-separated hex) ===\n");
    gmp_scanf("%Zx %Zx %Zx %Zx", fss_params.p, fss_params.q, fss_params.g, fss_pkey.y);

    // SOCKET 
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    printf("Listening on port %d...\n", PORT);

    uint8_t out_buf[4096] = {0};
    uint32_t out_len = 0;
    LimbStatus *statuses = NULL;

    axolotl_recv(sockfd, out_buf, &out_len, skey, &params,
                  &fss_params, &fss_pkey, &statuses);

    printf("Received %u bytes:\n%s\n", out_len, (char *)out_buf);

    uint32_t num_limbs = (out_len + RAW_CHUNK_SIZE - 1) / RAW_CHUNK_SIZE;
    printf("\n--- Limb statuses ---\n");
    for (uint32_t i = 0; i < num_limbs; i++) {
        const char *s = statuses[i] == LIMB_OK ? "OK" :
                         statuses[i] == LIMB_LOST ? "LOST" : "TAMPERED";
        printf("Limb %u: %s\n", i, s);
    }
    free(statuses);

    close(sockfd);
    mpz_clear(skey); mpz_clear(pkey);
    mpz_clear(params.p); mpz_clear(params.q); mpz_clear(params.g);
    fs_param_clear(&fss_params);
    fs_pkey_clear(&fss_pkey);
    return 0;
}
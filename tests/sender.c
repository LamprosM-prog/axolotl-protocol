#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include "axolotl.h"

#define PORT 9000

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: ./sender <receiver_elgamal_pkey_hex> <receiver_ip>\n");
        return 1;
    }

    // -- ElGamal: import receiver's pkey --
    ElGamalParam params;
    mpz_init(params.p);
    mpz_init(params.q);
    mpz_init(params.g);
    generate_param(&params);  // see note below

    mpz_t pkey;
    mpz_init(pkey);
    mpz_set_str(pkey, argv[1], 16);

    // -- FSS: sender generates, prints params+pkey for receiver --
    FSParams fss_params;
    FSSkey   fss_skey;
    FSPkey   fss_pkey;
    fs_param_init(&fss_params);
    fs_skey_init(&fss_skey);
    fs_pkey_init(&fss_pkey);
    fs_key_gen(&fss_params, &fss_skey, &fss_pkey);

    printf("=== Paste into receiver (FSS params+pkey: p q g y) ===\n");
    gmp_printf("%Zx %Zx %Zx %Zx\n\n", fss_params.p, fss_params.q, fss_params.g, fss_pkey.y);
    printf("Press Enter once pasted into receiver...\n");
    getchar();

    uint8_t *msg = (uint8_t *)"This is a 64 byte test message for axolotl protocol!!!!!";
    uint32_t len = 64;

    AxolotlSession *sess = axolotl_init(msg, len, pkey, &params, &fss_params, &fss_skey);
    if (!sess) { printf("axolotl_init failed\n"); return 1; }
    printf("Session initialized. Limbs: %u\n", sess->num_limbs);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest;
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(PORT);
    dest.sin_addr.s_addr = inet_addr(argv[2]);

    int ret = axolotl_send(sockfd, &dest, sess);
    if (ret < 0) printf("Send failed\n");
    else         printf("Sent successfully\n");

    axolotl_free(sess);
    close(sockfd);
    mpz_clear(pkey);
    mpz_clear(params.p); mpz_clear(params.q); mpz_clear(params.g);
    fs_param_clear(&fss_params);
    fs_skey_clear(&fss_skey);
    fs_pkey_clear(&fss_pkey);
    return 0;
}
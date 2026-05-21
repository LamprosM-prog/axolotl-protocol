#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "axolotl.h"
#include <stdlib.h> 

#define PORT 9000



int main() {
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

    // print pkey for sender to use
    char *pkey_str = mpz_get_str(NULL, 16, pkey);
    printf("Public key (paste into sender):\n%s\n\n", pkey_str);
    free(pkey_str);

    // -- SOCKET --
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    printf("Listening on port %d...\n", PORT);

    uint8_t out_buf[4096] = {0};
    uint32_t out_len = 0;
    axolotl_recv(sockfd, out_buf, &out_len, skey, &params);
    printf("Received %u bytes:\n%s\n", out_len, (char *)out_buf);

    close(sockfd);
    mpz_clear(skey); mpz_clear(pkey);
    mpz_clear(params.p); mpz_clear(params.q); mpz_clear(params.g);
    return 0;
}
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "axolotl.h"

#define RECEIVER_IP "YOUR_RECEIVER_IP_HERE"
#define PORT 9000


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./sender <receiver_pkey_hex>\n");
        return 1;
    }

    ElGamalParam params;
    mpz_init(params.p);
    mpz_init(params.q);
    mpz_init(params.g);
    generate_param(&params);

    // import receiver's pkey from argument
    mpz_t pkey;
    mpz_init(pkey);
    mpz_set_str(pkey, argv[1], 16);

    uint8_t *msg = (uint8_t *)"This is a 64 byte test message for axolotl protocol!!!!!";
    uint32_t len = 64;

    AxolotlSession *sess = axolotl_init(msg, len, pkey, &params);
    if (!sess) { printf("axolotl_init failed\n"); return 1; }
    printf("Session initialized. Limbs: %u\n", sess->num_limbs);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest;
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(PORT);
    dest.sin_addr.s_addr = inet_addr(RECEIVER_IP);

    int ret = axolotl_send(sockfd, &dest, sess);
    if (ret < 0) printf("Send failed\n");
    else         printf("Sent successfully\n");

    axolotl_free(sess);
    close(sockfd);
    mpz_clear(pkey);
    mpz_clear(params.p); mpz_clear(params.q); mpz_clear(params.g);
    return 0;
}
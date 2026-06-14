#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include "raw_test.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./sender_raw <receiver_ip> [num_limbs]\n");
        return 1;
    }

    uint32_t num_limbs = (argc >= 3) ? (uint32_t)atoi(argv[2]) : 100;

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in dest;
    dest.sin_family      = AF_INET;
    dest.sin_port        = htons(PORT);
    dest.sin_addr.s_addr = inet_addr(argv[1]);

    //HANDSHAKE
    SessionOpen open;
    open.total_data_len = num_limbs * PACKETS_PER_LIMB * PACKET_SIZE;
    open.num_limbs      = num_limbs;
    sendto(sockfd, &open, sizeof(open), 0, (struct sockaddr *)&dest, sizeof(dest));

    char ack[16];
    struct sockaddr_in from;
    socklen_t from_len = sizeof(from);
    struct timeval tv = {5, 0};
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int ret = recvfrom(sockfd, ack, sizeof(ack), 0, (struct sockaddr *)&from, &from_len);
    if (ret <= 0) {
        printf("No ACK received, aborting.\n");
        return 1;
    }

    uint32_t total_packets = num_limbs * PACKETS_PER_LIMB;
    printf("Handshake OK. Sending %u limbs (%u packets, %u bytes)...\n",
           num_limbs, total_packets, total_packets * PACKET_SIZE);

    //TRANSMISSION 
    uint8_t packet[PACKET_SIZE];
    for (uint32_t l = 0; l < num_limbs; l++) {
        for (uint32_t p = 0; p < PACKETS_PER_LIMB; p++) {
            memset(packet, 0, PACKET_SIZE);
            memcpy(packet, &l, sizeof(uint32_t));
            memcpy(packet + 4, &p, sizeof(uint32_t));
            memset(packet + 8, 0xAB, PACKET_SIZE - 8);

            sendto(sockfd, packet, PACKET_SIZE, 0, (struct sockaddr *)&dest, sizeof(dest));
        }
    }

    printf("Done sending.\n");
    close(sockfd);
    return 0;
}
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include "raw_test.h"

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    struct sockaddr_in addr;
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));
    printf("Listening on port %d...\n", PORT);

    // -- HANDSHAKE --
    SessionOpen open;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    struct timeval tv = {0, 0}; // block until handshake arrives
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    recvfrom(sockfd, &open, sizeof(open), 0, (struct sockaddr *)&client_addr, &client_len);
    sendto(sockfd, "ACK", 3, 0, (struct sockaddr *)&client_addr, client_len);

    uint32_t total_packets = open.num_limbs * PACKETS_PER_LIMB;
    printf("Handshake OK. Expecting %u limbs (%u packets)...\n",
           open.num_limbs, total_packets);

    // -- RECEPTION --
    tv.tv_sec  = 0;
    tv.tv_usec = 500000; // 500ms per-packet timeout
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    uint32_t received_packets = 0;
    uint32_t mismatched       = 0;

    uint8_t packet[PACKET_SIZE];

    for (uint32_t l = 0; l < open.num_limbs; l++) {
        for (uint32_t p = 0; p < PACKETS_PER_LIMB; p++) {
            int ret = recvfrom(sockfd, packet, PACKET_SIZE, 0,
                                (struct sockaddr *)&client_addr, &client_len);
            if (ret > 0) {
                received_packets++;

                uint32_t recv_limb_id, recv_packet_id;
                memcpy(&recv_limb_id,  packet,     sizeof(uint32_t));
                memcpy(&recv_packet_id, packet + 4, sizeof(uint32_t));

                if (recv_limb_id != l || recv_packet_id != p)
                    mismatched++;
            }
            // else: timed out -> lost
        }
    }

    printf("\n--- Results ---\n");
    printf("Expected packets:   %u\n", total_packets);
    printf("Received packets:   %u\n", received_packets);
    printf("Lost packets: %u (%.2f%%)\n",
           total_packets - received_packets,
           100.0 * (total_packets - received_packets) / total_packets);
    printf("Mismatched/reorder: %u\n", mismatched);

    close(sockfd);
    return 0;
}
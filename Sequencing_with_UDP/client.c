#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>

#define CHUNK_SIZE 5
#define MAX_CHUNKS 100
#define BUFFER_SIZE (CHUNK_SIZE * MAX_CHUNKS)
#define HOST "127.0.0.1"
#define SEND_PORT 8081
#define RECV_PORT 5000
#define TIMEOUT_MS 100

typedef struct {
    int seq_num;
    int total_chunks;
    char data[CHUNK_SIZE];
} DataChunk;

typedef struct {
    int seq_num;
} AckPacket;

int send_sock, recv_sock;
struct sockaddr_in dest_addr;

void send_chunk(DataChunk *chunk) {
    sendto(send_sock, chunk, sizeof(DataChunk), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
}

void *receive_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    fd_set readfds;
    struct timeval tv;
    DataChunk chunk;
    AckPacket ack;
    int count = 0;
    int curcount = 0;
    while (1) {
        FD_ZERO(&readfds);
        FD_SET(recv_sock, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 10000;  // 10ms

        int ready = select(recv_sock + 1, &readfds, NULL, NULL, &tv);
        if (ready > 0) {
            int received = recvfrom(recv_sock, &chunk, sizeof(DataChunk), 0, (struct sockaddr *)&sender_addr, &addr_len);
            if (received == sizeof(DataChunk)) {
                // Send ACK
                if(chunk.seq_num==0){
                    curcount=0;
                }
                count=chunk.total_chunks;
                ack.seq_num = chunk.seq_num;
                // printf("Received chunk %d\n", chunk.seq_num);
                // if(chunk.seq_num != 3){
                    sendto(recv_sock, &ack, sizeof(AckPacket), 0, (struct sockaddr *)&sender_addr, addr_len);
                    curcount++;
                    // printf("Sent ACK for chunk %d\n", chunk.seq_num);
                // }

                // Store chunk data
                memcpy(buffer + (chunk.seq_num * CHUNK_SIZE), chunk.data, CHUNK_SIZE);

                // Check if all chunks received
                if (count==curcount) {
                    buffer[chunk.total_chunks * CHUNK_SIZE] = '\0';
                    printf("\nReceived: %s\n", buffer);
                    printf("Enter message: ");
                    fflush(stdout);
                }
            }
        }
    }
    return NULL;
}

void *send_messages(void *arg) {
    char buffer[BUFFER_SIZE];
    DataChunk chunks[MAX_CHUNKS];
    int acks[MAX_CHUNKS] = {0};
    fd_set readfds;
    struct timeval tv;
    AckPacket ack;

    while (1) {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        int len = strlen(buffer);
        int total_chunks = (len + CHUNK_SIZE - 1) / CHUNK_SIZE;

        // Prepare chunks
        for (int i = 0; i < total_chunks; i++) {
            chunks[i].seq_num = i;
            chunks[i].total_chunks = total_chunks;
            memcpy(chunks[i].data, buffer + (i * CHUNK_SIZE), CHUNK_SIZE);
            acks[i] = 0;
        }

        // Send all chunks
        for (int i = 0; i < total_chunks; i++) {
            send_chunk(&chunks[i]);
            // printf("Sent chunk %d\n", i);
        }

        // Handle retransmissions
        while (1) {
            FD_ZERO(&readfds);
            FD_SET(send_sock, &readfds);
            tv.tv_sec = 0;
            tv.tv_usec = TIMEOUT_MS * 1000;

            int ready = select(send_sock + 1, &readfds, NULL, NULL, &tv);
            if (ready > 0) {
                int received = recvfrom(send_sock, &ack, sizeof(AckPacket), 0, NULL, NULL);
                if (received == sizeof(AckPacket)) {
                    acks[ack.seq_num] = 1;
                }
            } else {
                // Timeout occurred, resend unacknowledged chunks
                for (int i = 0; i < total_chunks; i++) {
                    if (!acks[i]) {
                        send_chunk(&chunks[i]);
                        // printf("Resent chunk %d\n", i);
                    }
                }
            }

            // Check if all chunks acknowledged
            int all_acked = 1;
            for (int i = 0; i < total_chunks; i++) {
                if (!acks[i]) {
                    all_acked = 0;
                    break;
                }
            }
            if (all_acked) break;
        }

        printf("Message sent successfully.\n");
    }
    return NULL;
}

int main() {
    struct sockaddr_in recv_addr;
    pthread_t recv_thread, send_thread;

    // Create UDP sockets
    send_sock = socket(AF_INET, SOCK_DGRAM, 0);
    recv_sock = socket(AF_INET, SOCK_DGRAM, 0);

    // Set non-blocking mode
    fcntl(send_sock, F_SETFL, O_NONBLOCK);
    fcntl(recv_sock, F_SETFL, O_NONBLOCK);

    // Prepare send address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SEND_PORT);
    inet_pton(AF_INET, HOST, &dest_addr.sin_addr);

    // Bind receive socket
    memset(&recv_addr, 0, sizeof(recv_addr));
    recv_addr.sin_family = AF_INET;
    recv_addr.sin_port = htons(RECV_PORT);
    recv_addr.sin_addr.s_addr = INADDR_ANY;
    bind(recv_sock, (struct sockaddr *)&recv_addr, sizeof(recv_addr));

    // Create threads
    pthread_create(&recv_thread, NULL, receive_messages, NULL);
    pthread_create(&send_thread, NULL, send_messages, NULL);

    // Wait for threads to complete
    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);

    // Close sockets
    close(send_sock);
    close(recv_sock);

    return 0;
}
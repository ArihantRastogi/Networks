#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 4096

void clearScreen() {
    printf("\033[2J");
    printf("\033[H");
}

int main(int argc, char *argv[]) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(serv_addr);
    char* ip_address;
    if(argc == 2){
        ip_address = argv[1];
    }
    else{
        ip_address = "127.0.0.1";
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, ip_address, &serv_addr.sin_addr);

    sendto(sock, "Ready", strlen("Ready"), 0, (struct sockaddr*)&serv_addr, addr_len);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        recvfrom(sock, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&serv_addr, &addr_len);

        if (strstr(buffer, "|")) {
            clearScreen();
        }
        printf("%s", buffer);

        if (strstr(buffer, "turn")) {
            char inputBuffer[BUFFER_SIZE];
            fgets(inputBuffer, BUFFER_SIZE, stdin);
            sendto(sock, inputBuffer, strlen(inputBuffer), 0, (struct sockaddr*)&serv_addr, addr_len);
        }

        if (strstr(buffer, "again")) {
            char inputBuffer[BUFFER_SIZE];
            fgets(inputBuffer, BUFFER_SIZE, stdin);
            sendto(sock, inputBuffer, 1, 0, (struct sockaddr*)&serv_addr, addr_len);
            if (inputBuffer[0] != 'y') {
                break;
            }
        }

        if (strstr(buffer, "Declined")) {
            break;
        }
    }

    close(sock);
    return 0;
}

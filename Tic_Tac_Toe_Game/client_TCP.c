#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

// Function to clear the screen
void clearScreen() {
    printf("\033[2J");  // Clear the screen
    printf("\033[H");   // Move the cursor to the top-left corner
}

#define PORT 8080
#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }
    char* ip_address;
    if(argc == 2){
        ip_address = argv[1];
    } else {
        ip_address = "127.0.0.1";
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip_address, &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    int flag = 0;
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        // Receive messages from server
        read(sock, buffer, BUFFER_SIZE);
        if(strstr(buffer, "|"))
            clearScreen(); // Clear the screen
        printf("%s", buffer); // Display the board or messages

        // If it's a player's turn, read their move
        if (strstr(buffer, "turn")) {
            char inputBuffer[BUFFER_SIZE];
            fgets(inputBuffer, BUFFER_SIZE, stdin);  // Get the input as a string
            send(sock, inputBuffer, strlen(inputBuffer), 0); // Send the input to the server
        }
        // If game is over, check if replay is possible
        if (strstr(buffer, "again")) {
            char inputBuffer[BUFFER_SIZE];
            fgets(inputBuffer, BUFFER_SIZE, stdin); // Use fgets to handle newlines
            write(sock, inputBuffer, 1); // Send only the first character as response to the server
            if (inputBuffer[0] != 'y') {
                break; // Exit if player doesn't want to play again
            }
        }
        // If other player rejected
        if(strstr(buffer, "Declined")){
            break;
        }
    }

    close(sock);
    return 0;
}

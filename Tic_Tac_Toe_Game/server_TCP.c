#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>

#define BUFFER_SIZE 4096

// Function to clear the screen
void clearScreen() {
    printf("\033[2J");  // Clear the screen
    printf("\033[H");   // Move the cursor to the top-left corner
}


#define PORT 8080
#define SIZE 3

char board[SIZE][SIZE];
int currentPlayer = 0; // 0 for Player 1 (X), 1 for Player 2 (O)
int clients[2];

void initBoard() {
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            board[i][j] = ' ';
        }
    }
}

void printBoard() {
    printf("\nCurrent board:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            printf(" %c ", board[i][j]);
            if (j < SIZE - 1) printf("|");
        }
        printf("\n");
        if (i < SIZE - 1) printf("---+---+---\n");
    }
    printf("\n");
}

void sendBoard(int client) {
    char message[100];
    sprintf(message, "Current board:\n");
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            sprintf(message + strlen(message), " %c ", board[i][j]);
            if (j < SIZE - 1) strcat(message, "|");
        }
        strcat(message, "\n");
        if (i < SIZE - 1) strcat(message, "---+---+---\n");
    }
    send(client, message, strlen(message), 0);
}

int checkWin() {
    for (int i = 0; i < SIZE; i++) {
        // Check rows and columns
        if ((board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ') ||
            (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' '))
            return 1;
    }
    // Check diagonals
    if ((board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') ||
        (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' '))
        return 1;

    // Check for draw
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (board[i][j] == ' ') return 0; // Game continues
        }
    }
    return 2; // Draw
}

void gameLoop() {
    int winner = 0;
    sendBoard(clients[0]);
    sendBoard(clients[1]);
    while (winner == 0) {
        int player = currentPlayer;
        char playerSymbol = (player == 0) ? 'X' : 'O';
        char message[400];
        sprintf(message, "Player %d's turn (symbol: %c). Enter row and column (1-3):\n", player + 1, playerSymbol);
        send(clients[player], message, strlen(message), 0);

        int row, col;
        char inputBuffer[BUFFER_SIZE];

        while (1) {
            clearScreen();
            memset(inputBuffer, 0, BUFFER_SIZE);  // Clear the buffer
            recv(clients[player], inputBuffer, BUFFER_SIZE, 0); // Receive input from client

            // Use sscanf to parse the input and check for invalid commands
            if (sscanf(inputBuffer, "%d %d", &row, &col) != 2 || row < 1 || row > 3 || col < 1 || col > 3) {
                sprintf(message, "Current board:\n");
                for (int i = 0; i < SIZE; i++) {
                    for (int j = 0; j < SIZE; j++) {
                        sprintf(message + strlen(message), " %c ", board[i][j]);
                        if (j < SIZE - 1) strcat(message, "|");
                    }
                    strcat(message, "\n");
                    if (i < SIZE - 1) strcat(message, "---+---+---\n");
                }
                sprintf(message, "%sInvalid Input. Your turn.\n", message);
                send(clients[player], message, strlen(message), 0); // Ask for input again
                continue; // Loop again until valid input is received
            }

            row--; col--; // Convert to 0-based index

            // Check if the move is valid (i.e., the cell is empty)
            if (board[row][col] != ' '){
                sprintf(message, "Current board:\n");
                for (int i = 0; i < SIZE; i++) {
                    for (int j = 0; j < SIZE; j++) {
                        sprintf(message + strlen(message), " %c ", board[i][j]);
                        if (j < SIZE - 1) strcat(message, "|");
                    }
                    strcat(message, "\n");
                    if (i < SIZE - 1) strcat(message, "---+---+---\n");
                }
                sprintf(message, "%sInvalid move, the cell is already occupied. Your turn.\n", message);
                send(clients[player], message, strlen(message), 0); // Ask for input again
                continue; // Loop again until valid input is received
            }

            break; // Valid input, break the loop
        }
        // Update the board with the valid move
        board[row][col] = playerSymbol;
        printBoard();

        // Send updated board to both clients
        for (int i = 0; i < 2; i++) {
            sendBoard(clients[i]);
        }

        // Check for a winner or a draw
        winner = checkWin();
        if (winner == 0) {
            currentPlayer = 1 - currentPlayer; // Switch player
        }
    }

    // Send result to both clients
    char result[100];
    if (winner == 1) {
        sprintf(result, "Player %d wins!\n", currentPlayer + 1);
    } else {
        sprintf(result, "It's a draw!\n");
    }
    send(clients[0], result, strlen(result), 0);
    send(clients[1], result, strlen(result), 0);
}


int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Creating socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 2) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for players to connect...\n");
    for (int i = 0; i < 2; i++) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        clients[i] = new_socket;
        char welcomeMessage[50];
        sprintf(welcomeMessage, "Player %d connected. You are '%c'.\n", i + 1, (i == 0) ? 'X' : 'O');
        send(new_socket, welcomeMessage, strlen(welcomeMessage), 0);
    }
    while (1) {
        clearScreen();

        // Ask if players want to play again
        char response[2];
        int allAgree = 1;
        int player_1_declined = 0;
        int player_2_declined = 0;
        for (int i = 0; i < 2; i++) {
            char playAgainMessage[50];
            sprintf(playAgainMessage, "Do you want to play again? (y/n): ");
            send(clients[i], playAgainMessage, strlen(playAgainMessage), 0);
        }
        for(int i=0; i<2; i++){
            char playAgainBuffer[BUFFER_SIZE];
            recv(clients[i], playAgainBuffer, sizeof(playAgainBuffer), 0);
            response[i] = playAgainBuffer[0]; // Only first character is needed
            if (response[i] != 'y') {
                if(i==0){
                    player_1_declined = 1;
                }
                else{
                    player_2_declined = 1;
                }
                allAgree = 0; // At least one player doesn't want to play again
                break;
            }
        }
        if(player_1_declined==1){
            player_1_declined = 0;
            char endMessage[50];
            sprintf(endMessage, "Player 1 Declined! Game Ends\n");
            send(clients[1], endMessage, strlen(endMessage), 0);
        }
        if(player_2_declined==1){
            player_2_declined = 0;
            char endMessage[50];
            sprintf(endMessage, "Player 2 Declined! Game Ends\n");
            send(clients[0], endMessage, strlen(endMessage), 0);
        }
        if(allAgree){
            char startMessage[50];
            sprintf(startMessage, "Game Restarted!\n");
            send(clients[0], startMessage, strlen(startMessage), 0);
            send(clients[1], startMessage, strlen(startMessage), 0);
        }
        if (!allAgree) {
            break; // Exit the loop if at least one player disagrees
        }

        initBoard();
        currentPlayer = 0; // Reset to Player 1
        printBoard();
        gameLoop();
    }

    // Clean up
    for (int i = 0; i < 2; i++) {
        close(clients[i]); // Close each client socket
    }
    close(server_fd); // Close server socket
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096
#define PORT 8080
#define SIZE 3

char board[SIZE][SIZE];
int currentPlayer = 0;
struct sockaddr_in clients[2];
int client_count = 0;
int server_fd;

void clearScreen() {
    printf("\033[2J");
    printf("\033[H");
}

void initBoard() {
    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            board[i][j] = ' ';
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

void sendBoard(int player_index) {
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
    sendto(server_fd, message, strlen(message), 0, (struct sockaddr*)&clients[player_index], sizeof(clients[player_index]));
}

int checkWin() {
    for (int i = 0; i < SIZE; i++) {
        if ((board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ') ||
            (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' '))
            return 1;
    }
    if ((board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ') ||
        (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' '))
        return 1;

    for (int i = 0; i < SIZE; i++)
        for (int j = 0; j < SIZE; j++)
            if (board[i][j] == ' ') return 0;
    return 2;
}

void gameLoop() {
    int winner = 0;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char inputBuffer[BUFFER_SIZE];
    sendBoard(0);
    sendBoard(1);

    while (winner == 0) {
        int player = currentPlayer;
        char playerSymbol = (player == 0) ? 'X' : 'O';
        char message[400];
        sprintf(message, "Player %d's turn (symbol: %c). Enter row and column (1-3):\n", player + 1, playerSymbol);
        sendto(server_fd, message, strlen(message), 0, (struct sockaddr*)&clients[player], sizeof(clients[player]));

        int row, col;
        while (1) {
            clearScreen();
            memset(inputBuffer, 0, BUFFER_SIZE);
            recvfrom(server_fd, inputBuffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);

            if (sscanf(inputBuffer, "%d %d", &row, &col) != 2 || row < 1 || row > 3 || col < 1 || col > 3) {
                sprintf(message, "Invalid Input. Your turn.\n");
                sendto(server_fd, message, strlen(message), 0, (struct sockaddr*)&clients[player], sizeof(clients[player]));
                continue;
            }
            row--; col--;
            if (board[row][col] != ' ') {
                sprintf(message, "Invalid move, the cell is already occupied. Your turn.\n");
                sendto(server_fd, message, strlen(message), 0, (struct sockaddr*)&clients[player], sizeof(clients[player]));
                continue;
            }
            break;
        }
        board[row][col] = playerSymbol;
        printBoard();

        for (int i = 0; i < 2; i++) {
            sendBoard(i);
        }

        winner = checkWin();
        if (winner == 0) {
            currentPlayer = 1 - currentPlayer;
        }
    }

    char result[100];
    if (winner == 1) {
        sprintf(result, "Player %d wins!\n", currentPlayer + 1);
    } else {
        sprintf(result, "It's a draw!\n");
    }
    for (int i = 0; i < 2; i++) {
        sendto(server_fd, result, strlen(result), 0, (struct sockaddr*)&clients[i], sizeof(clients[i]));
    }
}

int main() {
    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));

    printf("Waiting for players to connect...\n");
    for (int i = 0; i < 2; i++) {
        char buffer[BUFFER_SIZE];
        recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&clients[i], &addr_len);
        sprintf(buffer, "Player %d connected. You are '%c'.\n", i + 1, (i == 0) ? 'X' : 'O');
        sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr*)&clients[i], addr_len);
    }

    while (1) {
        clearScreen();

        char response[2];
        int allAgree = 1;
        for (int i = 0; i < 2; i++) {
            char playAgainMessage[50];
            sprintf(playAgainMessage, "Do you want to play again? (y/n): ");
            sendto(server_fd, playAgainMessage, strlen(playAgainMessage), 0, (struct sockaddr*)&clients[i], addr_len);
        // }
        // for (int i = 0; i < 2; i++) {
            char playAgainBuffer[BUFFER_SIZE];
            recvfrom(server_fd, playAgainBuffer, sizeof(playAgainBuffer), 0, (struct sockaddr*)&clients[i], &addr_len);
            response[i] = playAgainBuffer[0];
            if (response[i] != 'y') {
                allAgree = 0;
                char endMessage[50];
                sprintf(endMessage, "Player %d Declined! Game Ends\n", i + 1);
                sendto(server_fd, endMessage, strlen(endMessage), 0, (struct sockaddr*)&clients[1 - i], addr_len);
                break;
            }
        }
        if (allAgree) {
            char startMessage[50];
            sprintf(startMessage, "Game Restarted!\n");
            for (int i = 0; i < 2; i++) {
                sendto(server_fd, startMessage, strlen(startMessage), 0, (struct sockaddr*)&clients[i], addr_len);
            }
        } else {
            break;
        }

        initBoard();
        currentPlayer = 0;
        printBoard();
        gameLoop();
    }

    close(server_fd);
    return 0;
}

# Networking
## XOXO - The Tic-Tac-Toe game
- The code can handle at most two clients. If a third client tries to connect it, it will be in an infinite loop for itself, server and the connected clients will function as they are supposed to.
- The game will only exit/restart until a response is heard from both clients. Even if one declines, the other still has to respond and the socket will not be closed properly. This is because `recv` is a blocking call.
- The input that will be considered will be 
    ```
    (row) (col)
    ```
    Any input consisting of multiple integers (more than 2) will be correct with the first 2 integers being considered as the input the contains `%d %d` inside it will still be a valid input. (Look at sscanf used for parsing).
- The code only restarts if the input is `y` and exits on any other input.
- To run the code using UDP as the communication method,
    ```
    gcc -o server client_UDP.c
    gcc -o client client_UDP.c
    ./server
    ./client 192.168.1.222
    ```
    If the IP Address is not mentioned, the client will take the local host 127.0.0.1 as the local host IP Address.
- To run the code using TCP as the communication method,
    ```
    gcc -o server client_TCP.c
    gcc -o client client_TCP.c
    ./server
    ./client 192.168.1.222
    ```
    If the IP Address is not mentioned, the client will take the local host 127.0.0.1 as the local host IP Address.
### TCP
- The `Do you want to play again (y/n)` message is sent to both clients simultaneoulsy. Both need to respond to continue even in cases of declining.
### UDP
- The `Do you want to play again (y/n)` message is sent to player 1 first, waiting for the response the player 2 and waiting for the response. Both need to respond to continue even in cases of declining.
- The above condition is different in both to ensure that the clients do not switch between player 1 and player 2.
## Fake it till you make it
- The code implements TCP using UDP by dividing the data into smaller chunks. Each chunk is assigned a number which is sent along with the transmission (use structs). The sender should also communicate the total number of chunks being sent. After the receiver has data from all the chunks, it should aggregate them according to their sequence number and display the text.
- CHUNK_SIZE - 5 bytes
- TIMEOUT - 0.1 seconds
- BUFFER_SIZE - 500 bytes (max input length)
- No code is implemented for 4-way handshake during establishing connection and ending connection using FYN/RST flags that happens in real TCP sockets for connection management. Only the mentioned functionalities were implemented:
    - Data Sequencing
    - Retransmissions
- The client and server communicate with each other in an undefined manner. Meaning that the client can send messages to server in any order and vice-versa.
- If the user inputs something in the terminal while it is not their chance, it will remain in the `buffer` and will be transmitted as a mesasage after the other user sends something.
- The server needs to be up and running first to get the client to connect to it.
- The code utilises two ports for communication.
- To run the code
    ```
    gcc -o server client.c
    gcc -o client client.c
    ./server
    ./client 192.168.1.222
    ```
    If the IP Address is not mentioned, the client will take the local host 127.0.0.1 as the local host IP Address.

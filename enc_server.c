#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>


// Set up the address struct for the server socket
void setupAddressStructServer(struct sockaddr_in* address, int portNumber) {
    // Clear out the address struct
    memset((char*) address, '\0', sizeof(*address)); 

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char *argv[]){
    int connectionSocket, charsRead, keyRead, encRead, keyConnectionSocket;
    char buffer[256];
    char keyBuffer[256];
    char encBuffer[256];
    int bufferLength = 0;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);
    // Counter is for counting child processes; need to be able to have up to 5
    int counter = 0;

    // Check usage & args
    if (argc < 2) { 
      fprintf(stderr,"USAGE: %s port\n", argv[0]); 
      exit(1);
    } 
  
    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        fprintf(stderr, "enc_server: error opening socket\n");
        exit(1);
    }

    // Set up the address struct for the server socket
    setupAddressStructServer(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "enc_server: error binding socket to port\n");
        exit(1);
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5); 
  
    // Accept a connection, blocking if one is not available until one connects
    while(1) {
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
        if (connectionSocket < 0){
            fprintf(stderr, "enc_server: error on accept\n");
            exit(1);
        }
        counter++;                                                  // increment counter to indicate how many child processes are going  

        int child;
        pid_t spawnpid = fork();
        if (spawnpid == -1) {                                       // fork failed
            fprintf(stderr, "Failed to fork.\n");
            continue;
        } else if (spawnpid > 0) {                                  // parent process
            continue;
        } else if (spawnpid == 0) {                                 // child process
            // Get the validation code from the client ("enc")
            memset(buffer, '\0', 256);                              // always clear the buffer       
            charsRead = recv(connectionSocket, buffer, 3, 0);       // Read the client's sent item from the socket, should be 3 bytes ("enc")
            if (charsRead < 0) {
                counter--;                                          // undo counter, this didn't work
                fprintf(stderr, "enc_server: error reading from socket\n");
                exit(1);
            }
            if (strcmp(buffer, "enc") != 0) {                       // NOT A VALID CONNECTION
                counter--;                                          // undo counter
                fprintf(stderr, "Incorrect server connection");
                exit(2);
            }
            
            // Get the SIZE of the message to be encrypted from the client (in bytes)
            memset(buffer, '\0', 256);
            charsRead = recv(connectionSocket, buffer, 5, 0);       // client should send 5 bytes
            if (charsRead < 0){
                counter--;
                fprintf(stderr, "enc_server: error reading from socket\n");
                exit(1);
            }
            int sizeOfMsg = atoi(buffer);                           // necessary for next step: look for a message of this length

            // Get the message to be encrypted from the client
            memset(buffer, '\0', 256);
            // TODO retry n number of times ???   
            char *msgBuffer = calloc(sizeOfMsg, sizeof(char));      // Allocate buffer just for storing the rec'd msg
            int msgIndex = 0;
            int len = sizeOfMsg;
            while (len > 0) {
                if (len < 256) {                                    // If msg is < 256 chars, or that is all that is left
                    charsRead = recv(connectionSocket, &msgBuffer[msgIndex], len, 0);
                    if (charsRead != len) {
                        fprintf(stderr, "enc_server: message data is missing\n");
                        exit(1);
                    }
                    break;
                } else {                                            // Otherwise just grab 256 chars at a time
                    charsRead = recv(connectionSocket, &msgBuffer[msgIndex], 256, 0);
                    msgIndex += 256;                                // move index up
                    len = len - 256;                                // move size down
                    if (charsRead != 256) {
                        fprintf(stderr, "enc_server: message data is missing\n");
                        exit(1);
                    }
                }
            }

            // Get the SIZE of the key to use to encrypt from the client (in bytes)
            memset(buffer, '\0', 256);
            charsRead = recv(connectionSocket, buffer, 5, 0);       // client should send 5 bytes
            if (charsRead < 0){
                counter--;
                fprintf(stderr, "enc_server: error reading from socket\n");
                exit(1);
            }
            int sizeOfKey = atoi(buffer);                           // necessary for next step: look for a key of this length
            
            // Get the key to use to encrypt from the client
            memset(buffer, '\0', 256);
            // TODO retry n number of times ???
            char *keyBuffer = calloc(sizeOfKey, sizeof(char));      // Allocate buffer just for storing the rec'd msg
            int keyIndex = 0;
            int keyLen = sizeOfKey;
            while (keyLen > 0) {
                if (keyLen < 256) {                                    // If msg is < 256 chars, or that is all that is left
                    charsRead = recv(connectionSocket, &keyBuffer[keyIndex], keyLen, 0);
                    if (charsRead != keyLen) {
                        fprintf(stderr, "enc_server: message data is missing\n");
                        exit(1);
                    }
                    break;
                } else {                                            // Otherwise just grab 256 chars at a time
                    charsRead = recv(connectionSocket, &keyBuffer[keyIndex], 256, 0);
                    keyIndex += 256;                                // move index up
                    keyLen = keyLen - 256;                                // move size down
                    if (charsRead != 256) {
                        fprintf(stderr, "enc_server: message data is missing\n");
                        exit(1);
                    }
                }
            }

            // Create ciphertext by encrypting message with key
            char *encBuffer = calloc(sizeOfMsg, sizeof(char));
            int i = 0;
            for (i = 0; i< sizeOfMsg; i++) {
                char a, b;
                if (msgBuffer[i] == ' ') {
                    a = 26;
                } else {
                    a = msgBuffer[i] - 'A';        // convert to number, A = 0, B = 1, ... Z = 25
                }

                if (keyBuffer[i] == ' ') {
                    b = 26;
                } else {
                    b = keyBuffer[i] - 'A';
                }
              
                encBuffer[i] = (a + b) % 27;
                if (encBuffer[i] == 26) {
                    encBuffer[i] = ' ';
                } else {
                    encBuffer[i] = encBuffer[i] + 'A';
                }
            }

            // Tell client how big the encrypted message will be
            memset(buffer, '\0', sizeof(buffer));
            sprintf(buffer, "%5d", sizeOfMsg);
            int sentChars = send(connectionSocket, buffer, strlen(buffer), 0);
            
            // Send ciphertext
            memset(buffer, '\0', sizeof(buffer));
            strcpy(buffer, encBuffer);
            int encIndex = 0;
            int encLen = strlen(encBuffer);
            while (encLen > 0) {
                if (encLen < 256) {
                    sentChars = send(connectionSocket, &buffer[encIndex], encLen, 0);
                    if (sentChars != keyLen) {
                        fprintf(stderr, "enc_server: message data is missing\n");
                        exit(1);
                    }
                    break;
                } else {
                    sentChars = send(connectionSocket, &buffer[encIndex], 256, 0);
                    encIndex += 256;
                    encLen = encLen - 256;
                    if (sentChars != 256) {
                        fprintf(stderr, "enc_server: message data is missing\n");
                        exit(1);
                    }
                }
            }
            counter--;
            exit(0);        
        }

        close(connectionSocket); 
        close(keyConnectionSocket);
    }
    // Close the listening socket
    close(listenSocket); 
    return 0;
}

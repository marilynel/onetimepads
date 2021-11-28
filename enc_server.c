#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 


/**
 * QUESTIONS:
 * how can i listen for multiple things on here? do I fork?
 *      --> i am not able to listen for mesage then key, but i want both passed in and i want ot keep them separate
 **/

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
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

    // Check usage & args
    if (argc < 2) { 
      fprintf(stderr,"USAGE: %s port\n", argv[0]); 
      exit(1);
    } 
  
    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5); 
  
    // Accept a connection, blocking if one is not available until one connects
    while(1) {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
        if (connectionSocket < 0){
            error("ERROR on accept");
        }

        //printf("SERVER: Connected to client running at host %d port %d\n", ntohs(clientAddress.sin_addr.s_addr), ntohs(clientAddress.sin_port));

        // Get the message from the client and display it
        memset(buffer, '\0', 256);
        // Read the client's message from the socket
        charsRead = recv(connectionSocket, buffer, 255, 0); 
        if (charsRead < 0) {
            error("ERROR reading from socket");
        }
        printf("SERVER: I received this from the client: \"%s\"\n", buffer);


        // get key
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
        if (connectionSocket < 0){
            error("ERROR on accept");
        }
        
        memset(keyBuffer, '\0', 256);
        keyRead = recv(connectionSocket, keyBuffer, 255, 0);
        if (keyRead < 0) {
            error("ERROR reading key from socket");
        }
        printf("SERVER: I received this from the client: \"%s\"\n", keyBuffer);
        

        // encrypt message
        /*
        int i = 0;
        for (i = 0; i < sizeof(buffer); i++) {
          encBuffer[i] = buffer[i] + keyBuffer[i];
        }
        */

        // Send a Success message back to the client
        charsRead = send(connectionSocket, buffer, sizeof(buffer), 0); 
        if (charsRead < 0){
          error("ERROR writing to socket");
        }

        /*
        keyRead = send(connectionSocket, keyBuffer, sizeof(keyBuffer), 0);
        if (keyRead < 0) {
          error("ERROR writing key to socket");
        }
        */
        // Close the connection socket for this client
        close(connectionSocket); 
        close(keyConnectionSocket);
    }
    // Close the listening socket
    close(listenSocket); 
    return 0;
}

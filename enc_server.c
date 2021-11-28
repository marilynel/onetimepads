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
void setupAddressStructServer(struct sockaddr_in* address, 
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
    // for counting child processes; need to have up to 5
    int counter = 0;

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
    setupAddressStructServer(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5); 
  
    // Accept a connection, blocking if one is not available until one connects
    while(1) {
        // Accept the connection request which creates a connection socket
        if (counter >=5) {
            sleep(1000);
            continue;
        }
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo); 
        if (connectionSocket < 0){
            error("ERROR on accept");
        }
        counter++;

        int child;
        pid_t spawnpid = fork();
        if (spawnpid == -1) {                 // failed
            fprintf(stderr, "Failed to fork.\n");
            continue;
        } else if (spawnpid > 0) {            // parent
            continue;
        } else if (spawnpid == 0) {           // child
            // Get the message from the client and display it
            memset(buffer, '\0', 256);
            // Read the client's message from the socket
            charsRead = recv(connectionSocket, buffer, 3, 0); 
            if (charsRead < 0) {
                counter--;
                error("ERROR reading from socket");
            }
            if (strcmp(buffer, "enc") != 0) {
                counter--;
                fprintf(stderr, "Incorrect server connection");
                exit(2);
            }
            printf("SERVER: I received this from the client: \"%s\"\n", buffer);
            
            memset(buffer, '\0', 256);
            // get the size of the mexsage
            charsRead = recv(connectionSocket, buffer, 5, 0); 
            if (charsRead < 0){
                counter--;
                error("ERROR writing to socket");
            }
            printf("SERVER: I received this from the client: \"%s\"\n", buffer);
            int sizeOfMsg = atoi(buffer);

            // get message
            memset(buffer, '\0', 256);
            // TODO allocate a buffer later
            // TODO only read 256 bytes at a time
            // TODO retry n number of times ???   
            //charsRead = recv(connectionSocket, buffer, sizeOfMsg, 0);
            char *msgBuffer = calloc(sizeOfMsg, sizeof(char));
            int msgIndex = 0;
            int len = sizeOfMsg;
            while (len > 0) {
                if (len < 256) {
                    //sentChars = send(socketFD, &buffer[msgIndex], len, 0);
                    charsRead = recv(connectionSocket, &msgBuffer[msgIndex], len, 0);
                    if (charsRead != len) {
                        error("Data is missing");
                    }
                    break;
                } else {
                    charsRead = recv(connectionSocket, &msgBuffer[msgIndex], 256, 0);
                    msgIndex += 256;
                    len = len - 256;
                    if (charsRead != 256) {
                        error("Data is missing");
                    }
                }
            }

            // redo for key

            
            
            
            if (charsRead < 0){
                counter--;
                error("ERROR writing to socket");
            }
            printf("SERVER: I received this from the client: \"%s\"\n", buffer);
            //char *msgBuffer = calloc(sizeOfMsg, sizeof(char));
            // save message in buffer
            //strcpy(msgBuffer, buffer);

            // get key
            memset(buffer, '\0', 256);
            // get the size of the mexsage
            charsRead = recv(connectionSocket, buffer, 5, 0); 
            if (charsRead < 0){
                counter--;
                error("ERROR writing to socket");
            }
            printf("SERVER: I received this from the client: \"%s\"\n", buffer);
            int sizeOfKey = atoi(buffer);
            
            
            //printf("server: rec'd buffer size is %d\n", sizeOfMsg);
            // get message
            memset(buffer, '\0', 256);
            // TODO allocate a buffer later
            // TODO only read 256 bytes at a time
            // TODO retry n number of times ???
            charsRead = recv(connectionSocket, buffer, sizeOfKey, 0);
            if (charsRead < 0){
                counter--;
                error("ERROR writing to socket");
            }
            printf("SERVER: I received this from the client: \"%s\"\n", buffer);
            char *keyBuffer = calloc(sizeOfKey, sizeof(char));
            // save message in buffer
            strcpy(keyBuffer, buffer);


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

            //printf("Encryption: %s\n", encBuffer);

            // tell client how big the encrypted message will be
            memset(buffer, '\0', sizeof(buffer));
            sprintf(buffer, "%5d", sizeOfMsg);
            int sentChars = send(connectionSocket, buffer, strlen(buffer), 0);
            
            // send message
            memset(buffer, '\0', sizeof(buffer));
            strcpy(buffer, encBuffer);
            printf("SERVEr: sending \"%s\" \n", buffer);
            sentChars = send(connectionSocket, buffer, strlen(buffer), 0);
            
/*
            sentChars = send(connectionSocket, encBuffer, sizeof(encBuffer), 0);
            if (keyRead < 0) {
                error("ERROR writing key to socket");
            }
*/
            counter--;
            exit(0);
        } 


        // get key
        /*
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
        */

        

        // Send a Success message back to the client
        

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

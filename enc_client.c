#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
    perror(msg); 
    exit(1); 
} 

// Set up the address struct
void setupAddressStructClient(struct sockaddr_in* address, int portNumber, char* hostname) {
    // Clear out the address struct
    memset((char*) address, '\0', sizeof(*address)); 

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname(hostname); 
    if (hostInfo == NULL) { 
        fprintf(stderr, "CLIENT: ERROR, no such host\n"); 
        exit(1); 
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

// Usage: enc_client myplaintext mykey 57171
//        <prg name> <message>   <key> <port>

int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead, keyWritten, keyRead, nextSend;
    struct sockaddr_in serverAddress;
    char buffer[256];
    char keyBuffer[256];
    // Check usage & args
    if (argc < 3) { 
        fprintf(stderr,"USAGE: %s messageFile keyFile port\n", argv[0]);        
        // messageFile = argv[1], keyFile = argv[2], port = argv[3]
        exit(0); 
    } 

    // open and read message file
    FILE *messageFile = fopen(argv[1], "r");
    // move pointer to end of file
    fseek(messageFile, 0, SEEK_END);
    // find size of file
    int sizeOfMsg = ftell(messageFile);
    // move cursor back to beginning of file??
    fseek(messageFile, 0, SEEK_SET);
    char *msgBuffer = calloc(sizeOfMsg, sizeof(char));
    if (msgBuffer == NULL) {
        error("Unable to create buffer");
    }
    // read file into buffer
    size_t result = fread(msgBuffer, sizeof(char), sizeOfMsg, messageFile);
    if (result != sizeOfMsg) {
        error("Read from file error");
    }
    // remove trailing newline 
    msgBuffer[sizeOfMsg - 1] = '\0';

    // validate message input
    int i = 0;
    for (i = 0; i < sizeOfMsg-1; i++) {
        if (((msgBuffer[i] < 'A') || (msgBuffer[i] > 'Z'))) {
            if (msgBuffer[i] != ' ') {
                error("Invalid message");
            }
        } 
    }

    // open and read key file
    FILE *keyFile = fopen(argv[2], "r");
    fseek(keyFile, 0, SEEK_END);
    int sizeOfKey = ftell(keyFile);
    fseek(keyFile, 0, SEEK_SET);

    size_t keyresult = fread(keyBuffer, sizeof(char), sizeOfKey, keyFile);
    if (keyresult != sizeOfKey) {
        error("Read from key error");
    }
    keyBuffer[sizeOfKey - 1] = '\0';




    if (sizeOfKey < sizeOfMsg) {
        //error("Error: key ‘%s’ is too short", argv[2]);
        error("Error: key is too short");
    } 


    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        error("CLIENT: ERROR opening socket");
    }

    
    // Set up the server address struct
    setupAddressStructClient(&serverAddress, atoi(argv[3]), "localhost");
    
    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        printf("ok\n");
        error("CLIENT: ERROR connecting");
    }

    // check for valid client/server connection by sending "enc" to server
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, "enc");
    int sentChars = send(socketFD, buffer, strlen(buffer), 0);
    if (sentChars != strlen(buffer)) {
        fprintf(stderr, "Incorrect server connection");
        exit(2);
    }

    // tell server how big the message will be
    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%5d", sizeOfMsg-1);
    sentChars = send(socketFD, buffer, strlen(buffer), 0);

    // send message
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, msgBuffer);
    printf("CLIENT: sending \"%s\" \n", buffer);
    int msgIndex = 0;
    int len = strlen(msgBuffer);
    while (len > 0) {
        if (len < 256) {
            sentChars = send(socketFD, &buffer[msgIndex], len, 0);
            if (sentChars != len) {
                error("Data is missing");
            }
            break;
        } else {
            sentChars = send(socketFD, &buffer[msgIndex], 256, 0);
            msgIndex += 256;
            len = len - 256;
            if (sentChars != 256) {
                error("Data is missing");
            }
        }
    }

    // redo for key


    // tell server how big the key will be
    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%5d", sizeOfKey-1);
    sentChars = send(socketFD, buffer, strlen(buffer), 0);

    // send key
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, keyBuffer);
    printf("CLIENT: sending \"%s\" \n", buffer);
    sentChars = send(socketFD, buffer, strlen(buffer), 0);



    // Send message to server
    // Write to the server
    /*
    charsWritten = send(socketFD, buffer, strlen(buffer)+1, 0); 
    if (charsWritten < 0){
        error("CLIENT: ERROR writing to socket");
    }
    if (charsWritten < strlen(buffer)){
        printf("CLIENT: WARNING: Not all data written to socket!\n");
    }
    */

    /*
    keyWritten = send(socketFD, keyBuffer, strlen(keyBuffer)+1, 0); 
    //printf("Client: sending key\n");
    if (keyWritten < 0){
        error("CLIENT: ERROR writing key to socket");
    }
    if (keyWritten < strlen(keyBuffer)){
        printf("CLIENT: WARNING: Not all key data written to socket!\n");
    }
    */

    // Get size of encrypted message from server
    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));
    // get size of encrypted message
    charsRead = recv(socketFD, buffer, 5, 0); 
    if (charsRead < 0){
        error("CLIENT: ERROR reading from socket");
    }
    printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
    sizeOfMsg = atoi(buffer);
    
    // get encrypted message from server
    memset(buffer, '\0', 256);
    // get the size of the mexsage
    charsRead = recv(socketFD, buffer, sizeOfMsg, 0); 
    if (charsRead < 0){
        error("ERROR writing to socket");
    }
    printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
   



    /*
    memset(keyBuffer, '\0', sizeof(keyBuffer));
    // Read data from the socket, leaving \0 at end
    keyRead = recv(socketFD, keyBuffer, sizeof(keyBuffer) - 1, 0); 
    if (keyRead < 0){
        error("CLIENT: ERROR reading from socket");
    }
    printf("CLIENT: I received this from the server: \"%s\"\n", keyBuffer);
*/
    // Close the socket
    close(socketFD); 
    return 0;
}

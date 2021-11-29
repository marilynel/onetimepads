#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

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

int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead, keyWritten, keyRead, nextSend;
    struct sockaddr_in serverAddress;
    char buffer[256];
    // Check usage & args
    if (argc < 3) { 
        fprintf(stderr,"USAGE: %s messageFile keyFile port\n", argv[0]);        
        // messageFile = argv[1], keyFile = argv[2], port = argv[3]
        exit(0); 
    } 

    // Open and read message file into buffer
    FILE *messageFile = fopen(argv[1], "r");
    fseek(messageFile, 0, SEEK_END);
    int sizeOfMsg = ftell(messageFile);             // get size of file from the end
    fseek(messageFile, 0, SEEK_SET);                // move cursor back to beginning of file??
    char *msgBuffer = calloc(sizeOfMsg, sizeof(char));
    if (msgBuffer == NULL) {
        fprintf(stderr, "enc_client: error creating buffer\n");
        exit(1);
    }
    size_t result = fread(msgBuffer, sizeof(char), sizeOfMsg, messageFile);
    if (result != sizeOfMsg) {
        fprintf(stderr, "enc_client: error reading file\n");
        exit(1);
    }
    msgBuffer[sizeOfMsg - 1] = '\0';                // remove trailing newline 

    // Check message for valid characters only (A to Z, plus space)
    int i = 0;
    for (i = 0; i < sizeOfMsg-1; i++) {
        if (((msgBuffer[i] < 'A') || (msgBuffer[i] > 'Z'))) {
            if (msgBuffer[i] != ' ') {
                fprintf(stderr, "enc_client error: input contains bad characters\n");
                exit(1);
            }
        } 
    }

    // Open and read key file into buffer
    FILE *keyFile = fopen(argv[2], "r");
    fseek(keyFile, 0, SEEK_END);
    int sizeOfKey = ftell(keyFile);
    fseek(keyFile, 0, SEEK_SET);
    char *keyBuffer = calloc(sizeOfKey, sizeof(char));
    if (keyBuffer == NULL) {
        fprintf(stderr, "enc_client: error creating buffer\n");
        exit(1);
    }
    size_t keyresult = fread(keyBuffer, sizeof(char), sizeOfKey, keyFile);
    if (keyresult != sizeOfKey) {
        fprintf(stderr, "enc_client: error reading file\n");
        exit(1);
    }
    keyBuffer[sizeOfKey - 1] = '\0';                // remove trailing newline 



    // Check to make sure key is at least as large as the message
    if (sizeOfKey < sizeOfMsg) {
        fprintf(stderr, "Error: key ‘%s’ is too short\n", argv[2]);
        exit(1);
    } 


    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        fprintf(stderr, "enc_client: error opening socket\n");
        exit(1);
    }

    
    // Set up the server address struct
    setupAddressStructClient(&serverAddress, atoi(argv[3]), "localhost");
    
    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "enc_client: error connecting to server\n");
        exit(1);
    }

    // Check for valid client/server connection by sending "enc" to server
    memset(buffer, '\0', sizeof(buffer));
    strcpy(buffer, "enc");
    int sentChars = send(socketFD, buffer, strlen(buffer), 0);
    if (sentChars != strlen(buffer)) {
        fprintf(stderr, "Incorrect server connection");
        exit(2);
    }

    // Tell server how big the message will be
    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%5d", sizeOfMsg-1);
    sentChars = send(socketFD, buffer, strlen(buffer), 0);

    // Send message to be encrypted
    memset(buffer, '\0', sizeof(buffer));
    //strcpy(buffer, msgBuffer);
    int msgIndex = 0;
    int len = strlen(msgBuffer);
    while (len > 0) {
        if (len < 256) {
            sentChars = send(socketFD, &msgBuffer[msgIndex], len, 0);
            if (sentChars != len) {
                fprintf(stderr, "enc_client: message data is missing\n");
                exit(1);
            }
            break;
        } else {
            sentChars = send(socketFD, &msgBuffer[msgIndex], 256, 0);
            msgIndex += 256;
            len = len - 256;
            if (sentChars != 256) {
                fprintf(stderr, "enc_client: message data is missing\n");
                exit(1);
            }
        }
    }


    // Tell server how big the key will be
    memset(buffer, '\0', sizeof(buffer));
    sprintf(buffer, "%5d", sizeOfKey-1);
    sentChars = send(socketFD, buffer, strlen(buffer), 0);

    // Send key
    memset(buffer, '\0', sizeof(buffer));
    int keyIndex = 0;
    int keyLen = strlen(keyBuffer);
    while (keyLen > 0) {
        if (keyLen < 256) {
            sentChars = send(socketFD, &keyBuffer[keyIndex], keyLen, 0);
            if (sentChars != keyLen) {
                fprintf(stderr, "enc_client: message data is missing\n");
                exit(1);
            }
            break;
        } else {
            sentChars = send(socketFD, &keyBuffer[keyIndex], 256, 0);
            keyIndex += 256;
            keyLen = keyLen - 256;
            if (sentChars != 256) {
                fprintf(stderr, "enc_client: message data is missing\n");
                exit(1);
            }
        }
    }

    // Get size of encrypted message from server
    memset(buffer, '\0', 256);
    charsRead = recv(socketFD, buffer, 5, 0); 
    if (charsRead < 0){
        fprintf(stderr, "enc_client: error reading from socket\n");
        exit(1);
    }
    int sizeOfEnc = atoi(buffer);
    // TODO retry n number of times ???   
    char *encBuffer = calloc(sizeOfMsg, sizeof(char));      // Allocate buffer just for storing the rec'd msg
    int encIndex = 0;
    int encLen = sizeOfEnc;
    while (encLen > 0) {
        if (encLen < 256) {                                    // If msg is < 256 chars, or that is all that is left
            charsRead = recv(socketFD, &encBuffer[encIndex], encLen, 0);
            if (charsRead != encLen) {
                fprintf(stderr, "enc_client: message data is missing\n");
                exit(1);
            }
            break;
        } else {                                            // Otherwise just grab 256 chars at a time
            charsRead = recv(socketFD, &encBuffer[encIndex], 256, 0);
            encIndex += 256;                                // move index up
            encLen = encLen - 256;                                // move size down
            if (charsRead != 256) {
                fprintf(stderr, "enc_client: message data is missing\n");
                exit(1);
            }
        }
    }

    printf("%s\n", encBuffer);
 
    // Close the socket
    close(socketFD); 
    return 0;
}

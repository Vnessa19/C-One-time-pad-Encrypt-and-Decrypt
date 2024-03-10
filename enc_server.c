#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define BUF_SIZE 75000

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
}

// function to split the comma delimited data received from the client
// the data is: plaintext,key\n
void splitBuffer(const char *buf, char **input, char **key) {
    //lets find the positio of the comma and point to it
    char *commaPtr = strchr(buf, ',');
    if (commaPtr != NULL) {
        //calculate the lenth of the plain text input
        size_t inputLength = commaPtr - buf;
        //allocate memory up to the input length plus  one for null terminatr
        *input = malloc(inputLength + 1);
        if (*input == NULL) {
            perror("Failed to allocate memory for input");
            exit(EXIT_FAILURE);
        }
        //copy plain text part into the input buffer
        strncpy(*input, buf, inputLength);
        //add null terminator, need parethesis for correct order of precedense
        (*input)[inputLength] = '\0';

        //for the key part just set the pointer to after the position of the comma should work
        *key = commaPtr + 1; 
    } else {
        printf("No comma found in the buffer.\n");
        *input = NULL;
        *key = NULL;
    }
}

// Function to encrypt the data we got 
char encrypt(char input, char key) {
    /*
    int length = strlen(input);
    char* encrypted = malloc(length + 1);
    if (encrypted == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    for (int i = 0; i < length; i++) {
        int messageVal = (input[i] == ' ') ? 26 : input[i] - 'A';
        int keyVal = (key[i] == ' ') ? 26 : key[i] - 'A';
        // mod 27 to account for the space character
        int encryptedVal = (messageVal + keyVal) % 27;
        encrypted[i] = (encryptedVal == 26) ? ' ' : 'A' + encryptedVal;
    }
    //null terminate
    encrypted[length] = '\0';
    return encrypted;
*/
    char converted;
    int messageVal = (input == ' ') ? 26 : input - 'A';
    int keyVal = (key == ' ') ? 26 : key - 'A';
    // mod 27 to account for the space character
    int convertedVal = (messageVal + keyVal) % 27;
    converted = (convertedVal == 26) ? ' ' : 'A' + convertedVal;

    return converted;
}


void processRequest(int cfd) {

/*

  size_t totalReceived = 0;
  //create a buffer of the same size as the plain text file buffer, received data is likely the same size
  //but we can realocate more if needed later
  char* receiveBuffer = malloc(plainTextFileLength);
  if(!receiveBuffer){
    fprintf(stderr, "CLIENT: Failed to allocate memory for received data");
  }
 //keep track of chars received
  int keepLooping = 1;
  while (keepLooping == 1) {
    //get returned message from server
    charsRead = recv(socketFD, receiveBuffer + totalReceived, plainTextFileLength - totalReceived, 0);
    
    //end looping if there was an error or was connection closed?
    if (charsRead == -1 || charsRead == 0) {
      break;
    }

    totalReceived += charsRead;
    
    //double the buffer if full
    if (totalReceived == plainTextFileLength) {
      plainTextFileLength *= 2; // Double it
      receiveBuffer = realloc(receiveBuffer, plainTextFileLength);
      if (!receiveBuffer) {
        fprintf(stderr, "CLIENT: Failed to reallocate memory for receive buffer");
      }
    }

  }

*/


 /* Read data from the connection */
/*  
    char buf[BUF_SIZE];
    memset(buf, 0, BUF_SIZE);
    ssize_t nread;
    ssize_t totalRead = 0;
    while (totalRead < BUF_SIZE) {
      nread = read(cfd, buf + totalRead, BUF_SIZE - totalRead);
      if (nread == -1) {
        perror("read");
        close(cfd);
        exit(EXIT_FAILURE);
      }
      totalRead += nread;
        //are we done? check if null terminator was received whch means we are done
        //break out of the loop to avoid read from holding for more data to be received when there is no more
      if(buf[nread] == '\0') {
      //if(buf[strlen(buf) -1]  == '@') {
        break;
      }

    }
    //split the plain text and the key, they are delimited by a commaa
    char *input = NULL;
    char *key = NULL;
    splitBuffer(buf, &input, &key);
    char *encryptedMessage = encrypt(input, key);

    ssize_t encryptedMessageLength = strlen(encryptedMessage);

    // send the data back to the client
    ssize_t totalWritten = 0;
    ssize_t nwritten;
    while (totalWritten < encryptedMessageLength) {
      nwritten = write(cfd, encryptedMessage + totalWritten, encryptedMessageLength - totalWritten);
      if (nwritten == -1) {
        perror("write");
        close(cfd);
        free(encryptedMessage);
        exit(EXIT_FAILURE);
      }
      totalWritten += nwritten;
    }//end while write loop
    free(input);
    free(encryptedMessage);
 */
}

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
  int connectionSocket, charsRead;
  char buffer[256];
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
    error("Server ERROR opening socket");
  }

  // Set up the address struct for the server socket
  setupAddressStruct(&serverAddress, atoi(argv[1]));

  // Associate the socket to the port
  if (bind(listenSocket, 
          (struct sockaddr *)&serverAddress, 
          sizeof(serverAddress)) < 0){
    error("Server ERROR on binding");
  }

  // Start listening for connetions. Allow up to 5 connections to queue up
  listen(listenSocket, 5); 
  
  // Accept a connection, blocking if one is not available until one connects
  while(1){
    // Accept the connection request which creates a connection socket
    connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, 
                &sizeOfClientInfo); 
    if (connectionSocket < 0){
      error("Server ERROR on accept");
    }

    // Get the message from the client and display it
    memset(buffer, '\0', 256);
    // Read the client's message from the socket
    char plaintextChar;
    char keyChar;
    int pTxtDone = 0;
    int pCharsRead;
    int keyDone = 0;
    int keyCharsRead;
    int sentChars;
    char convertedChar;

    while(1) {
      if(pTxtDone == 0){
        pCharsRead = recv(connectionSocket, &plaintextChar, 1, 0);
        if (plaintextChar == '@'){
          pTxtDone = 1;
        }
        if (pCharsRead < 0){
          error("ERROR reading from socket");
        }
//fprintf(stderr, "%c", plaintextChar);
      }

      if(keyDone == 0){
        keyCharsRead = recv(connectionSocket, &keyChar, 1, 0);
        if (keyChar == '\n' || pTxtDone == 1){
          keyDone = 1;
        }
        if (keyCharsRead < 0){
          error("Server ERROR reading from socket");
        }
      }
      if (pTxtDone == 1 && keyDone == 1){
        break;
      } else {
        convertedChar = encrypt(plaintextChar, keyChar);
        sentChars = send(connectionSocket, &convertedChar, 1, 0);
        if (sentChars < 0) {
           error("Server error sending from socket");
        }
        //fprintf(stderr, "%c", convertedChar);
      }
    }
/*
    // Send a Success message back to the client
    charsRead = send(connectionSocket, 
                    "I am the server, and I got your message", 39, 0); 
    if (charsRead < 0){
      error("ERROR writing to socket");
    }
*/
    // Close the connection socket for this client
    close(connectionSocket); 
  }
  // Close the listening socket
  close(listenSocket); 
  return 0;
}


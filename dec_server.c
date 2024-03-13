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
/*void splitBuffer(const char *buf, char **input, char **key) {
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
}*/

// Function to encrypt the data we got 
char decrypt(char input, char key) {

    char converted;
    int messageVal = (input == ' ') ? 26 : input - 'A';
    int keyVal = (key == ' ') ? 26 : key - 'A';
    // mod 27 to account for the space character
    int convertedVal = (messageVal - keyVal + 27) % 27;
    converted = (convertedVal == 26) ? ' ' : 'A' + convertedVal;

    return converted;
}

int handshake(int socketFD){
    char buffer[3];
    int n;
     // sending "E#" to say I am Encryption Client
    n = write(socketFD, "D#", 2);
    if (n < 0){
      return 0;
      fprintf(stderr, "CLIENT: ERROR writing to socket during handshake");
    }
    n=0;
    memset(buffer, 0, 3);
    while(n < 2) {
      n = read(socketFD, buffer, 2);
      if (n < 0) {
        fprintf(stderr, "CLIENT: ERROR reading from socket during handshake");
        return 0;
      }

      if (strlen(buffer) == 2 && strcmp(buffer, "D#") == 0) {
        return 1;
      } else if (strlen(buffer) > 0 ){
        break;
      }
    }  
    return 0;
}


void processRequest(int connectionSocket) {
    char plaintextChar;
    char keyChar;
    char receivedData[2];
    int pTxtDone = 0;
    int pCharsRead;
    int keyDone = 0;
    int keyCharsRead;
    int sentChars;
    char convertedChar;

    while(1) {
      pCharsRead = recv(connectionSocket, &receivedData, 2, 0);
      if (pCharsRead < 0){
          error("ERROR reading from socket");
          break;
        } else if (pCharsRead < sizeof(receivedData)) {
             continue;
        }

      plaintextChar = receivedData[0];
      keyChar = receivedData[1];
      convertedChar = decrypt(plaintextChar, keyChar);
      sentChars = send(connectionSocket, &convertedChar, 1, 0);
      if (sentChars < 0) {
        error("Server error sending from socket");
      }
}


    /*while(1) {
      if(pTxtDone == 0){
        pCharsRead = recv(connectionSocket, &plaintextChar, 1, 0);
        if (plaintextChar == '@'){
          pTxtDone = 1;
        }
        if (pCharsRead < 0){
          error("ERROR reading from socket");
        }
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
      }
    }*/


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
    if(handshake(connectionSocket) == 0){
      close(connectionSocket); 
      continue;
    }

    switch(fork()){
        case -1:
          perror("Error Creating new process with fork()");
          close(connectionSocket);
          break;
        
        case 0:
          //child process
          close(listenSocket);
          processRequest(connectionSocket);

          //terminate child process
          exit(0);

        default:
          //parent process
          close(connectionSocket);
          break;
    }//end switch



   
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


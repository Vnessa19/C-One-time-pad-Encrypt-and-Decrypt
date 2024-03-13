#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()
#include <ctype.h>

/**
* Client code
* 1. Create a socket and connect to the server specified in the command arugments.
* 2. Prompt the user for input and send that input as a message to the server.
* 3. Print the message received from the server and exit the program.
*/

// Error function used for reporting issues
void error(const char *msg) { 
  perror(msg); 
  exit(2); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
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
    exit(0); 
  }
  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, 
        hostInfo->h_addr_list[0],
        hostInfo->h_length);
}

//offer a handshake to server
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

//function to check for invalid chars 
int isInvalidChar(char inputChar) {
    //if the curent character is not an uppercase letter and not a space and not a new line
    //then we got an unexpected value 
    if (!isupper(inputChar) && inputChar != ' ' && inputChar != '\n') {
        return 1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
  //argv[1] this is the plain text input filename 
  //argv[2] this is the key filenam
  //argv[3] this is the port
 
  int socketFD, portNumber, charsWritten;
  struct sockaddr_in serverAddress;
  
  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr,"USAGE: %s plaintext_filename key_filename port\n", argv[0]);
    exit(1);
  }
  //open the input file and the Key file
  FILE* inputFile = fopen(argv[1], "rb");
  FILE* keyFile = fopen(argv[2], "rb");

  if (!inputFile || !keyFile) {
    error("CLIENT: Failed to open file");
  }

  // get the lenght of the files
  fseek(inputFile, 0, SEEK_END);
  size_t plainTextFileLength = ftell(inputFile);
  fseek(inputFile, 0, SEEK_SET);

  fseek(keyFile, 0, SEEK_END);
  size_t keyLength = ftell(keyFile);
  fseek(keyFile, 0, SEEK_SET);
    
  //check if key is smaller than input
  if(keyLength < plainTextFileLength) {
    fprintf(stderr, "CLIENT: Key file is shorter than plaintext.");
    exit(1);
  }
  
  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0); 
  if (socketFD < 0){
    error("CLIENT: ERROR opening socket");
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
    error("CLIENT: ERROR connecting");
  }
 
  if(handshake(socketFD) == 0) {
    close(socketFD); 
    error("CLIENT: Handshake rejected by server.");  
  }

  char charFromInputFile;
  char charFromKeyFile;
  char sendBuff[2];
  int doneSend1 = 0;
  int doneSend2 = 0;
  int pCharsSent;
  int keyCharsSent;
  int charsRead;
  char receivedChar;

  while(1) {
    //read sigle character from each file
    if (fread(&charFromInputFile, 1, sizeof charFromInputFile, inputFile) < 1 || fread(&charFromKeyFile, 1, sizeof charFromKeyFile, keyFile) < 1) {
      error("CLIENT: Failed to read file");
      fclose(keyFile);
      fclose(inputFile);
      exit(1);
    }
    //check for invalid characters
    if (isInvalidChar(charFromInputFile) == 1 || isInvalidChar(charFromKeyFile) == 1) {
      fprintf(stderr, "CLIENT: Invalid charater found\n");
      exit(1);
    }
    //replace with end of transmission indicator if this is the new line character found at the last character in the file
    if (charFromInputFile == '\n') charFromInputFile = '@';

    sendBuff[0] = charFromInputFile;
    sendBuff[1] = charFromKeyFile;

    if(doneSend1 == 0){
      pCharsSent =  send(socketFD, sendBuff, 2, 0); 
      if (pCharsSent < 0){
        error("ERROR reading from socket");
      }
      if (charFromInputFile == '@') {
        doneSend1 = 1;
      }
    }

    if (doneSend1 == 1) {
      printf("\n");
      fclose(keyFile);
      fclose(inputFile);
      break;
    } else {
      charsRead = recv(socketFD, &receivedChar, 1, 0);
      if (charsRead < 0){
        error("Server ERROR reading from socket");
      }
      printf("%c",receivedChar);
      fflush(stdout);
    }
    // start streaming message to server sending one char at a time from each file
    /*if(doneSend1 == 0) {
      pCharsSent =  send(socketFD, &charFromInputFile, 1, 0); 
      if (pCharsSent < 0){
        error("ERROR reading from socket");
      }
      if (charFromInputFile == '@') {
        doneSend1 = 1;
      }
    }

    if(doneSend2 == 0){
      keyCharsSent = send(socketFD, &charFromKeyFile, 1, 0);
      if (keyCharsSent < 0){
        error("ERROR reading from socket");
      }
      if (charFromKeyFile == '\n' || doneSend1 == 1) {
        doneSend2 = 1;
      }
    }

    if (doneSend1 == 1 && doneSend2 == 1){
      printf("\n");
      fclose(keyFile);
      fclose(inputFile);
      break;
    } else {
      charsRead = recv(socketFD, &receivedChar, 1, 0);
      if (charsRead < 0){
        error("Server ERROR reading from socket");
      }
      printf("%c",receivedChar);
      fflush(stdout);
    }*/
   


  }

  // Close the socket and clean up
  close(socketFD); 
  return 0;
}

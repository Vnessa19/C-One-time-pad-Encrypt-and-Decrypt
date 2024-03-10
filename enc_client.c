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

/*
 * Reeusabel function to load contents of files is more convenient
 * param filename is the name of the file
 * param length is a reference to a var in memory will be set to the length of the contents of the file
 *             Useful to compare size of key and plantext
 * returns the contents of the files
 */
char* readFileIntoBuffer(const char* filename, size_t* length) {
  FILE* file = fopen(filename, "rb");
  if (!file) {
    error("CLIENT: Failed to open file");
    exit(1);
  }
  //lets get the lenght of the contents using fseek and ftell
  fseek(file, 0, SEEK_END);
  *length = ftell(file);
  fseek(file, 0, SEEK_SET);

  char* buffer = malloc(*length);
  if (!buffer) {
    error("CLIENT: Memory allocation failed");
    fclose(file);
    exit(1);
  }

  if (fread(buffer, 1, *length, file) != *length) {
    error("CLIENT: Failed to read file");
    free(buffer);
    fclose(file);
    exit(1);
  }
  fclose(file);
  return buffer;
}

/*
char* combineBuffersCommaDemilimted(const char* buffer1, size_t length1, const char* buffer2, size_t length2) {
    // the total length for the combined buffer 
    // plus 3 for the comma, an end of transmission indicator @ for null termnator 
    size_t totalLength = length1 + length2 + 3;
    char* combinedBuffer = (char*)malloc(totalLength);
    if (!combinedBuffer) {
        fprintf(stderr, "Failed to allocate memory for combined buffer\n");
        exit(1);
    }

    // the first buffer first
    memcpy(combinedBuffer, buffer1, length1);
    // then add the comma
    combinedBuffer[length1] = ',';
    // and then the second buffer
    memcpy(combinedBuffer + length1 + 1, buffer2, length2);
    // null terminate the combined buffer
    combinedBuffer[totalLength - 2] = '@';

    // null terminate the combined buffer
    combinedBuffer[totalLength - 1] = '\0';

    //check for invalid characters in the cobbined buffer
    for (int i = 0; combinedBuffer[i] != '\0'; ++i) {
        //if the curent character is not an uppercase letter, not a space and not a comma
        //then we got an error
        if (!isupper(combinedBuffer[i]) && combinedBuffer[i] != ' ' && combinedBuffer[i] != ',' && combinedBuffer[i] != '\n' && combinedBuffer[i] != '@') {
          fprintf(stderr, "CLIENT: Invalid charater found\n");
          exit(1);
        }
    }

    return combinedBuffer;
}
*/
//offer a handshake to server
int handshake(int socketFD){
    char buffer[3];
    int n;
     // sending "E#" to say I am Encryption Client
    n = write(socketFD, "E#", 2);
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

      if (strlen(buffer) == 2 && strcmp(buffer, "E#") == 0) {
        return 1;
      } else if (strlen(buffer) > 0 ){
        break;
      }
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
 // load the file contents file into filebuffer
  size_t plainTextFileLength;
  char* fileBuffer = readFileIntoBuffer(argv[1], &plainTextFileLength);
  //insert poison pill at the new line
  if(fileBuffer[plainTextFileLength - 1] =='\n') {
    fileBuffer[plainTextFileLength - 1] = '@';
    //plainTextFileLength--;
  }

  // now load key file into another buffer
  size_t keyLength;
  char* keyBuffer = readFileIntoBuffer(argv[2], &keyLength);

  if(keyLength < plainTextFileLength) {
    fprintf(stderr, "CLIENT: Key file is shorter than plaintext.");
    exit(1);
  }

  //now lets combine both buffers separated by a coma so that we can send 
  //the whole thing in one message to the server
 // char* combinedBuffer = combineBuffersCommaDemilimted(fileBuffer, plainTextFileLength, keyBuffer, keyLength);
  //size_t combinedBufferLength = strlen(combinedBuffer);

  
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

  
//  if(handshake(socketFD) == 0) {
//    error("CLIENT: Handshake rejected by server.");  
//  }

// Clear out the buffer array
 // char buffer[500];
  //memset(buffer, '\0', sizeof(buffer));
  // Send message to server
  
    int doneSend1 = 0;
    int doneSend2 = 0;
    int sendCount1 = 0;
    int sendCount2 = 0;
    int pCharsSent;
    int keyCharsSent;
    int charsRead;
    char receivedChar;

    while(1) {
      if(doneSend1 == 0) {
        pCharsSent =  send(socketFD, &fileBuffer[sendCount1], 1, 0); 
        if (pCharsSent < 0){
          error("ERROR reading from socket");
        }
        if (fileBuffer[sendCount1] == '@') {
          doneSend1 = 1;
        }
        sendCount1++;
      }

      if(doneSend2 == 0){
        keyCharsSent = send(socketFD, &keyBuffer[sendCount2], 1, 0);
        if (keyCharsSent < 0){
          error("ERROR reading from socket");
        }
        if (keyBuffer[sendCount2] == '\n' || doneSend1 == 1) {
          doneSend2 = 1;
        }

        sendCount2++;
      }

      if (doneSend1 == 1 && doneSend2 == 1){
        printf("\n");
        break;
      } else {
        charsRead = recv(socketFD, &receivedChar, 1, 0);
        if (charsRead < 0){
          error("Server ERROR reading from socket");
        }
        printf("%c", receivedChar);
      }

    }

  //charsWritten = send(socketFD, buffer, strlen(buffer), 0); 
  //if (charsWritten < 0){
  //  error("CLIENT: ERROR writing to socket");
  //}
  //if (charsWritten < strlen(buffer)){
  //  printf("CLIENT: WARNING: Not all data written to socket!\n");
 // }

  // Get return message from server
  // Clear out the buffer again for reuse
/* 
  memset(buffer, '\0', sizeof(buffer));
  // Read data from the socket, leaving \0 at end
  charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }
  printf("CLIENT: I received this from the server: \"%s\"\n", buffer);
*/







  //print encrypted message received
  //printf("%s\n", receiveBuffer);

  // Close the socket and clean up the buffer bonanza
  close(socketFD); 
  free(fileBuffer);
  free(keyBuffer);
  //free(receiveBuffer);
  //free(combinedBuffer);
  return 0;
}

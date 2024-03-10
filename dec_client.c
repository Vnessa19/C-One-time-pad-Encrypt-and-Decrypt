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
 * function to combine the plain text file and the key file separated by a comma
 * this is necessary becasue the whole thing has to be sent to the server in one message
 */
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

//offer a handshake to server
int handshake(int socketFD){
    char buffer[3];
    int n;
     // sending "E#" to say I am Decryption Client
    n = write(socketFD, "D#", 2);
    if (n < 0){
      return 0;
      fprintf(stderr, "DEC CLIENT: ERROR writing to socket during handshake");
    }
    n=0;
    memset(buffer, 0, 3);
    while(n < 2) {
      n = read(socketFD, buffer, 2);
      if (n < 0) {
        fprintf(stderr, "DEC CLIENT: ERROR reading from socket during handshake");
        return 0;
      }

      if (strlen(buffer) == 2 && strcmp(buffer, "D#") == 0) {
        return 1;
      } else if (strlen(buffer) > 0 ){
        break;
      }
    }  
    return 0;

   /* 
   char buffer[256];
    int n, result;
     // sending "DC" to say I am Deryption Client
    n = write(socketFD, "DC", 2);
    if (n < 0){
      return 0;
      fprintf(stderr, "CLIENT: ERROR writing to socket during handshake");
    }

    memset(buffer, 0, 256);
    n = read(socketFD, buffer, 255);
    if (n < 0) {
       fprintf(stderr, "CLIENT: ERROR reading from socket during handshake");
       return 0;
    }

    if (strcmp(buffer, "ok") == 0) {
      result = 1;
    } else {
      result = 0;
    }
    return result;
    */
}



int main(int argc, char *argv[]) {
  //argv[1] this is the plain text input filename 
  //argv[2] this is the key filenam
  //argv[3] this is the port
 
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;
  
  // Check usage & args
  if (argc < 4) { 
    fprintf(stderr,"USAGE: %s plaintext_filename key_filename port\n", argv[0]);
    exit(1);
  } 

  // load the file contents file into filebuffer
  size_t plainTextFileLength;
  char* fileBuffer = readFileIntoBuffer(argv[1], &plainTextFileLength);
  //remove new line at the end if present
  if(fileBuffer[plainTextFileLength - 1] =='\n') {
    fileBuffer[plainTextFileLength - 1] = '\0';
    plainTextFileLength--;
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
  char* combinedBuffer = combineBuffersCommaDemilimted(fileBuffer, plainTextFileLength, keyBuffer, keyLength);
  size_t combinedBufferLength = strlen(combinedBuffer);

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
     error("CLIENT: Handshake rejected by server.");  
  }

/*  
  // load the file contents file into filebuffer
  size_t plainTextFileLength;
  char* fileBuffer = readFileIntoBuffer(argv[1], &plainTextFileLength);
  //remove new line at the end if present
  if(fileBuffer[plainTextFileLength - 1] =='\n') {
    fileBuffer[plainTextFileLength - 1] = '\0';
    plainTextFileLength--;
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
  char* combinedBuffer = combineBuffersCommaDemilimted(fileBuffer, plainTextFileLength, keyBuffer, keyLength);
  size_t combinedBufferLength = strlen(combinedBuffer);
*/

  // keep track of totalchars written
  int totalCharsWritten = 0;

  // Send to the server in a loop until all bytes are sent
  while (totalCharsWritten < combinedBufferLength) {
    charsWritten = send(socketFD, combinedBuffer + totalCharsWritten, combinedBufferLength - totalCharsWritten, 0); 
    if (charsWritten < 0){
      error("CLIENT: ERROR writing to socket");
    }
    totalCharsWritten += charsWritten;
  }

  if (totalCharsWritten < combinedBufferLength){
    fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
  }


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


  //charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
  if (charsRead < 0){
    error("CLIENT: ERROR reading from socket");
  }

  //print encrypted message received
  printf("%s\n", receiveBuffer);

  // Close the socket and clean up the buffer bonanza
  close(socketFD); 
  free(fileBuffer);
  free(keyBuffer);
  free(receiveBuffer);
  free(combinedBuffer);
  return 0;
}
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define BUF_SIZE 100000

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
char* encrypt(const char* input, const char* key) {
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
}

// Function to decrypt the input
char* decrypt(const char* input, const char* key) {
	int length = strlen(input);
    char* decrypted = malloc(length + 1);
    if (decrypted == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    for (int i = 0; i < length; i++) {
        int encryptedVal = (input[i] == ' ') ? 26 : input[i] - 'A';
        int keyVal = (key[i] == ' ') ? 26 : key[i] - 'A';
        int decryptedVal = (encryptedVal - keyVal + 27) % 27;
        decrypted[i] = (decryptedVal == 26) ? ' ' : 'A' + decryptedVal;
    }
    decrypted[length] = '\0'; // Null-terminate the decrypted string
    return decrypted;
}

void processRequest(int cfd) {
 /* Read data from the connection */
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
          char *encryptedMessage = decrypt(input, key);

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
}
//from the book: SIGCHILD handler to reap dead child processes
static void grimReaper(int sig) {
    int savedErrno;
   savedErrno = errno;
   while(waitpid(-1, NULL, WNOHANG) > 0) {
       continue;
   }
   errno = savedErrno;
}

//do i want to shake hands with client?
int handshake(int socketFD){
    char buffer[3];
    int n;
     // sending "D#" to say I am Decryption Server
    n = write(socketFD, "D#", 2);
    if (n < 0){
      return 0;
      fprintf(stderr, "DEC SERVER: ERROR writing to socket during handshake");
    }
    n=0;
    memset(buffer, 0, 3);
    while(n < 2) {
      n = read(socketFD, buffer, 2);
      if (n < 0) {
        fprintf(stderr, "DEC SERVER: ERROR reading from socket during handshake");
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
    
    memset(buffer, 0, 256);
    n = read(socketFD, buffer, 255);
    if (n < 0) {
       perror("SERVER ERROR reading from socket during handshake");
       return 0;
    }
    
    //expecting DC from Decryption client
    if (strcmp(buffer, "DC") == 0) {
        n = write(socketFD, "ok", 2);
        result = 1;
    } else {
        n = write(socketFD, "false", 5);
        result = 0;
    }

    if (n < 0){ 
      perror("SERVER ERROR writing to socket during handshake");
      result = 0;
    }
    
    return result;
  */
}



int main(int argc, char *argv[])
{
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sfd, s;
    int cfd; // Socket file descriptor for the client
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;
    //ssize_t nread;
   // char buf[BUF_SIZE];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
/*
    //from the book: sig action handler
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);  
    sa.sa_flags = SA_RESTART;
    sa.sa_handler = grimReaper;

    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
*/

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket (TCP) */
    hints.ai_flags = AI_PASSIVE;    /* For wildcard IP address */
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;

    s = getaddrinfo(NULL, argv[1], &hints, &result);
    if (s != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
        sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sfd == -1)
            continue;

        if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0)
            break; /* Success */

        close(sfd);
    }

    if (rp == NULL) { /* No address succeeded */
        fprintf(stderr, "Could not bind\n");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result); /* No longer needed */

    if (listen(sfd, 5) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

//    for(;;){
        /* Accept a connection. Note: this is a blocking call. */
        peer_addr_len = sizeof(struct sockaddr_storage);
        cfd = accept(sfd, (struct sockaddr *)&peer_addr, &peer_addr_len);
        if (cfd == -1) {
          perror("Failure accepting connection");
          exit(EXIT_FAILURE);
        }
/*
        switch(fork()){
          case -1:
            perror("Error Creating new process with fork()");
            close(cfd);
            break;
          
          case 0:
            //child process
            close(sfd);
*/
//           if(handshake(cfd) == 1){
              processRequest(cfd);
//           } else {
              //close(cfd);
//           }

/*
            //terminate child process
            exit(0);

          default:
            //parent process
            close(cfd);
            break;
      }//end switch
*/ 
//    }//end infinity for

    close(cfd); // Close the client socket
    close(sfd); // Close the server socket
    return 0;
}


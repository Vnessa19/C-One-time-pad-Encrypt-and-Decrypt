This project contains five small programs developed on the Linux platform using Vim as the text editor. These programs will utilize multiprocessing and socket-based communication to encrypt and decrypt data akin to a one-time pad system (https://en.wikipedia.org/wiki/One-time_pad). The program will encrypt and decrypt plaintext into ciphertext, using a key, using modulo 27 operations: 26 capital letters, and the space character. These programs use the API for network IPC: socket, connect, bind, listen, & accept to establish connections; send, recv to send and receive sequences of bytes for the purposes of encryption and decryption by the appropriate servers.They should be operable from the command line, supporting standard Unix functionalities such as input/output redirection and job control. A bash shell script called compileall that creates 5 executable programs from the files is included. 

Brief pecifications of the five programs:
enc_server: This program is the encryption server and will run in the background as a daemon. Usage syntax: end_server <listening_port>

enc_client: This program connects to enc_server, and asks it to perform a one-time pad style encryption as detailed above. enc_client doesn’t do the encryption - enc_server does. Usage syntax: enc_client <plaintest> <key> <port>

dec_server: this program performs exactly like enc_server, in syntax and usage. dec_server will decrypt ciphertext that is given, using the passed-in ciphertext and key, and it returns plaintext again to dec_client.

dec_cilent: similarly, this program will connect to dec_server and will ask it to decrypt ciphertext using a passed-in ciphertext and key. It performs exactly like enc_client. dec_client should not be able to connect to enc_server, if it tries to connect on the correct port, it will get rejected. 

keygen: This program creates a key file of specified length. The characters in the file generated will be any of the 27 allowed characters, generated using the standard Unix randomization methods. The last character keygen outputs will be a newline. Any error text will output to stderr. Usage syntax: keygen <keylength>


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main( int argc, char *argv[] )  {
  
  char key_source_chars[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

  if( argc > 2 ) {
    fprintf(stderr, "Too many arguments supplied.\n");
    return 1;
  }
  int keyLen = atoi(argv[1]);
  //Make sure a positive number was entered
  if (keyLen <= 0) {
    fprintf(stderr, "Enter number greater than zero for the key length.\n");
    return 1;
  }

  //make room in memory to store the key 
  char *key = malloc(keyLen + 1); 

  //seed the rand function to make sure we don't kenerate the same random sequesnce
  srand(time(NULL));
  for (int i = 0; i < keyLen; i++) {
    int randIdx = rand() % 27;
    key[i] = key_source_chars[randIdx]; 
    //fprintf(stderr, "%s", &key_source_chars[0][randIdx]);
  }

  key[keyLen] = '\0';
  printf("%s\n", key);

  free(key);
  return 0;
}


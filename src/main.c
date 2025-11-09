#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "input.h"

int main() {
  // Initialise the value we will input 
  char *str = NULL;

  while(1) {
    // Checks whether if str contains a string.
    // If that is the case, it frees the previous prompt 
    // for the next prompt the user will enter.
    if(str != NULL) { free(str); }
  
    // Prints the prompt immediately into the output stream
    if(write(STDOUT_FILENO, "$ ", 3) == -1) {
      perror("failed to write prompt");
      exit(EXIT_FAILURE);
    }
    
    str = getl();
    
    printf("%s\n", str);

    if(strncmp(str,"exit", 4) == 0) {
      fprintf(stdout, "Exited oyster shell\n");
      free(str);
      break;
    }
  }
  return 0;
}

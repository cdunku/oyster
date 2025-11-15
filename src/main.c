#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "input.h"
#include "tokenizer.h"
#include "exec.h"
#include "helper.h"

int main() {
  // Initialise the value we will input 
  char *str = NULL;
  char **argv = NULL;

  while(1) {
    // Checks whether if str contains a string.
    // If that is the case, it frees the previous prompt 
    // for the next prompt the user will enter.
    // Same goes when tokenizing the prompt with argv too.
    if(str != NULL) { free(str); }
    if(argv != NULL) { vector_free(argv, 0); }

    // Prints the prompt immediately into the output stream
    if(write(STDOUT_FILENO, "$ ", 2) == -1) {
      perror("failed to write prompt");
      exit(EXIT_FAILURE);
    }
    
    str = getl();
    
    // printf("%s\n", str);

    argv = tokenizer(str);

    /*
    size_t i = 0;
    while(argv[i] != NULL) {
      printf("%s, ", argv[i]);
      i++;
    }
    printf("\n");
    */  
      
    handle_exec(argv, NULL);
    if(strncmp(str,"quit", 4) == 0) {

      fprintf(stdout, "Exited oyster shell\n");

      vector_free(argv, 0);
      free(str);
      break;
    }
  }
  return 0;
}

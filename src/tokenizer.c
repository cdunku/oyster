#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tokenizer.h"
#include "helper.h"

#define BUFFER_SIZE 64

char **tokenizer(char *str) {
  size_t argv_size = BUFFER_SIZE; 
  size_t token_size = BUFFER_SIZE;

  char *token = malloc(token_size * sizeof(char));
  char **argv = malloc(argv_size * sizeof(char *));

  if(token == NULL || argv == NULL) {
    fprintf(stderr, "Fatal: failed to allocate memory for tokenization\n");
    exit(EXIT_FAILURE);
  }

  size_t i = 0, j = 0, k = 0;

  bool in_quotes = false;
  char quote = '\0';

  char c;

  while((c = str[i]) != '\0') {

    if(!in_quotes) {
      switch (c) { 
        case ' ':
        case '\n':
        case '\t':
        case '\a':
          
          if(k > 0) {
            token[k] = '\0';
            char *tokendup = strdup(token);
            
            if(tokendup == NULL) {
              vector_free(argv, j);
              free(token);
              fprintf(stderr, "Fatal: failed to allocate memory to prompt\n");
              exit(EXIT_FAILURE);
            }
            argv[j++] = tokendup;
            k = 0;
          }
          i++;

          continue;

        case '"':
        case '\'':
          in_quotes = true;
          quote = c;
          i++;

          continue;
      }
    }
    else {
      if(in_quotes && c == quote) {
        in_quotes = false;
        i++;

        continue;
      }
    }

    token[k++] = c;
    i++;
          
    if(k == token_size - 1) {
      token_size *= 2;
      char *_token = realloc(token, token_size * sizeof(char));

      if(_token == NULL) {
        vector_free(argv, j);
        free(token);
        return NULL;
      }

      token = _token;
    } 

    if(j == argv_size - 1) {
      argv_size *= 2;
      char **_argv = realloc(argv, argv_size * sizeof(char *));
      
      if(_argv == NULL) {
        vector_free(argv, j);
        free(token);
        return NULL;
      
      }
      argv = _argv;
    }
  }

  // If the last character ends with a delimiter that this function hasn't included,
  // then the last character can be deleted and replaced by '\0'. To prevent this we want 
  // to check whether if the last token was tokenized or not.
  
  if(k > 0) {
    token[k] = '\0';
    char *tokendup = strdup(token);
    
    if(tokendup == NULL) {
      vector_free(argv, 0);
      free(token);
      fprintf(stderr, "Fatal: failed to allocate memory to last character\n");
      exit(EXIT_FAILURE);
    }
    argv[j++] = tokendup;
  }

  if(in_quotes) {
  fprintf(stderr, "Error: missing %c quote\n", quote);
  vector_free(argv, 0);
  free(token);
  return NULL;

}

  argv[j] = NULL;
  free(token);
  
  return argv;
}

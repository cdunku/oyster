#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tokenizer.h"
#include "helper.h"

#define BUFFER_SIZE 64

Tokenizer tokenizer(char *str) {
  // Declare the initial buffer size for both the arguments vector and token size.
  size_t argv_size = BUFFER_SIZE; 
  size_t token_size = BUFFER_SIZE;

  // Declare the struct for tokenization and initialise everything needed.
  Tokenizer t;

  char *token = malloc(token_size * sizeof(char));
  t.argv = malloc(argv_size * sizeof(char *));
  t.argv_size = 0;

  if(token == NULL || t.argv == NULL) {
    fprintf(stderr, "Fatal: failed to allocate memory for tokenization\n");
    exit(EXIT_FAILURE);
  }

  size_t i = 0, j = 0;

  bool double_quotes = true;
  bool in_quotes = false;
  bool single_quotes;
  char quote = '\0';

  char c;

  while((c = str[i]) != '\0') {

    if(!in_quotes) {
      switch (c) { 
        case ' ':
        case '\n':
        case '\t':
        case '\a':
          // Checks whether if there are any characters inputted to the token variable,
          // if so and if we hit a delimiter, parse the token and input it into argv.
          if(j > 0) {
            token[j] = '\0';
            char *tokendup = strdup(token);
            
            if(tokendup == NULL) {
              vector_free(t.argv, t.argv_size);
              free(token);
              fprintf(stderr, "Fatal: failed to allocate memory to prompt\n");
              exit(EXIT_FAILURE);
            }
            t.argv[t.argv_size++] = tokendup;
            j = 0;
          }
          i++;

          continue;

        case '"':
          double_quotes = true;
        case '\'':
          // If we are inside " " or ' ' special rules apply.
          in_quotes = true;
          quote = c;
          i++;

          continue;
      }
    }
    else {
      if(double_quotes && str[i] == '\\') {

        double_quotes = false; 

        char ch = str[i + 1];
        char esc_ch;

        switch (ch) {
          case 'a':  esc_ch = '\a'; break;
          case 'b':  esc_ch = '\b'; break;
          case 'f':  esc_ch = '\f'; break;
          case 'n':  esc_ch = '\n'; break;
          case 'r':  esc_ch = '\r'; break;
          case 't':  esc_ch = '\t'; break;
          case 'v':  esc_ch = '\v'; break;
          case '?':  esc_ch = '\?'; break;
          case '\\': esc_ch = '\\'; break;
          case '\"': esc_ch = '\"'; break;
          case '\'': esc_ch = '\''; break;
          case '\0': esc_ch = '\0'; break;
            break;
          case 'x':
            // Convert to hex
            break;
          // Implement for octal numbers

          default:
            esc_ch = ch;
            break;
        }

        token[j++] = esc_ch;
        i += 2;
        continue;
      } 
      // If we are inside the quotes and the current character is equal to the initial quote,
      // get out of the string.
      if(in_quotes && c == quote) {
        in_quotes = false;
        i++;

        continue;
      }
    }

    // Write the character to the token and advance to the next character from the inputted string.
    token[j++] = c;
    i++;
         
    // Some boundary checks and reallocation so we do not overflow the buffers.
    if(j == token_size - 1) {
      token_size *= 2;
      char *_token = realloc(token, token_size * sizeof(char));

      if(_token == NULL) {
        vector_free(t.argv, t.argv_size);
        free(token);

        t.argv = NULL;
        t.argv_size = 0;
        return t;

      }

      token = _token;
    } 

    if(t.argv_size == argv_size - 1) {
      argv_size *= 2;
      char **_argv = realloc(t.argv, argv_size * sizeof(char *));
      
      if(_argv == NULL) {
        vector_free(t.argv, t.argv_size);
        free(token);

        t.argv = NULL;
        t.argv_size = 0;
        return t;
      
      }
      t.argv = _argv;
    }
  }

  // If the last character ends with a delimiter that this function hasn't included,
  // then the last character can be deleted and replaced by '\0'. To prevent this we want 
  // to check whether if the last token was tokenized or not.
  
  if(j > 0) {
    token[j] = '\0';
    char *tokendup = strdup(token);
    
    if(tokendup == NULL) {
      vector_free(t.argv, t.argv_size);
      free(token);

      fprintf(stderr, "Fatal: failed to allocate memory to last character\n");
      exit(EXIT_FAILURE);

    }
    t.argv[t.argv_size++] = tokendup;
  }

  // If we are inside the quotes and we haven't ended the string,
  // do not execute the command and display an error.
  
  if(in_quotes) {
  
    fprintf(stdout, "Error: missing %c quote\n", quote);
  
    vector_free(t.argv, t.argv_size);
    free(token);
    t.argv = NULL;

    t.argv_size = 0;
    return t;
  }

  t.argv[t.argv_size] = NULL;

  free(token);
  return t;
}

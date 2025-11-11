#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "input.h"

#define BUFFER_SIZE 64 // Starting buffer size

char *getl() {
  
  // We first initialise the buffer amount
  size_t buffer_size = BUFFER_SIZE;
  char *buffer = malloc(buffer_size * sizeof(char));

  char ch;
  ssize_t byte_read;

  size_t i = 0;

  while(1) {
    // Since ch is 1 byte long, it will read a maximum of 1 byte.
    byte_read = read(STDIN_FILENO, &ch, sizeof(ch));

    if(byte_read == -1) {
      perror("failed to read byte");
      exit(EXIT_FAILURE);
    }

    // If it is end of the new line (\n) or if the code reached EOF (0) then exit.
    if(ch == '\n' || byte_read == 0) {
      // If there was no input to begin with, free the buffer.
      if(byte_read == 0 && i == 0) {
        free(buffer);
        return NULL;
      }
      break; 
    } 

    // If the amount of the bytes read into the buffer is greater than the buffer size,
    // then reallocate new chunks of memory into the buffer.
    //
    // buffer_size - 1 < i checks whether if there is enough space to write to buffer[].
    // If we were to write buffer_size < i, that could overwrite the \0.
    // 
    // buffer_size -1 -> buffer size without the NULL character.
    // buffer_size -> includes the \0 character.
    if(i >= buffer_size - 1) { 
      buffer_size *= 2;
      char *_buffer = realloc(buffer, buffer_size);

      if(_buffer == NULL) {
        // Error: if the buffer cannot be expanded, the prompt fails.
        // Prevents buffer overflow.
        free(buffer);
        return NULL;
      }
      buffer = _buffer;
    }

    // Copy a character into the buffer.
    buffer[i++] = ch;
  }

  // The buffer ends here.
  buffer[i] = '\0';

  return buffer;
}

void tokenizer_free(char **argv, char* token, size_t size) {
  for(size_t i = 0; i < size; i++) {
    free(argv[i]);
  } 
  free(argv);
  free(token);
  return;
}

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

  while(str[i] != '\0') {
    switch (str[i]) { 
      case ' ':
      case '\n':
      case '\t':
      case '\a':
        if(k > 0) {
          token[k] = '\0';
          char *tokendup = strdup(token);
          if(tokendup == NULL) {
            tokenizer_free(argv, token, j);

            fprintf(stderr, "Fatal: failed to allocate memory to prompt\n");
            exit(EXIT_FAILURE);
          }
          argv[j++] = tokendup;
          k = 0; 
        }
        break;
      default:
        token[k++] = str[i];
        break;
    }
    if(k >= token_size - 1) {
      token_size *= 2;
      char *_token = realloc(token, token_size * sizeof(char));
      if(_token == NULL) {
        tokenizer_free(argv, token, j);
        return NULL;
      }
      token = _token;
    } 
    if(j >= argv_size - 1) {
      argv_size *= 2;
      char **_argv = realloc(argv, argv_size * sizeof(char *));
      if(_argv == NULL) {
        tokenizer_free(argv, token, j);
        return NULL;
      }
      argv = _argv;
    }
    i++;
  }

  // If the last character ends with a delimiter that this function hasn't included,
  // then the last character can be deleted and replaced by '\0'. To prevent this we want 
  // to check whether if the last token was tokenized or not.
  if(k > 0) {
    token[k] = '\0';
    char *tokendup = strdup(token);
    if(tokendup == NULL) {
      tokenizer_free(argv, token, j);

      fprintf(stderr, "Fatal: failed to allocate memory to last character\n");
      exit(EXIT_FAILURE);
    }
    argv[j++] = tokendup;
  }

  argv[j] = NULL;

  free(token);
  return argv;
} 

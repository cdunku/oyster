#include <stdio.h>
#include <stdlib.h>
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


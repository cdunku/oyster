#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "input.h"
#include "tokenizer.h"
#include "exec.h"
#include "helper.h"
#include "error.h"

bool has_unclosed_quotes(const char *str) {
  size_t i = 0;
  char save_quote = 0;
  size_t len = strlen(str);

  for(size_t i = 0; i < len; i++){
    // Checks for quotes.
    if(str[i] == '\'' || str[i] == '"') {
      // If we reached out first quote save it.
      if(save_quote == 0) {
        save_quote = str[i];
      }
      // If we reached the quote saved in save_quote initially, end the string.
      else if(save_quote == str[i]) {
        save_quote = 0;
      }
    }
  }
  return save_quote != 0;
}

char *join_unclosed_strings(char *str, char* unclosed_string) {
  size_t first_str_len = strlen(str);
  size_t second_str_len = strlen(unclosed_string);

  // We add 2 at the end because we count in the index where we need to end the string 
  // with either a '\n' and/or '\0'.
  char *new_str = malloc((first_str_len + second_str_len + 2) * sizeof(char));
  if(new_str == NULL) {
    fprintf(stderr, "Error: could not allocate memory to unclosed string\n");
    return NULL;
  }

  // We copy the contents of memory from the intial string to the new string
  memcpy(new_str, str, first_str_len);
  // We set the \n so we can differentiate between the first and second strings
  new_str[first_str_len] = '\n';
  // This is very important, the offset new_str + first_str_len + 1 signals that 
  // we are done copying the first string and + 1 to end the string.
  // Hence we create an offset of the first copied string and copy the next string.
  memcpy(new_str + first_str_len + 1, unclosed_string, second_str_len);
  // We end the string.
  new_str[first_str_len + second_str_len + 1] = '\0';

  free(str);
  free(unclosed_string);
  return new_str;
}

int main(void) {
  char *str = NULL;
  size_t total_cmds = 0;

  while (1) {
    if (write(STDOUT_FILENO, "$ -> ", 5) == -1) {
      perror("write");
      exit(EXIT_FAILURE);
    }

    str = getl();            
    if (str == NULL) continue;    
    // Check whether if we have an unclosed string
    while(has_unclosed_quotes(str)) {
      printf("quote> ");
      fflush(stdout);
      char *old_str = str;
      char *unclosed_string = getl();
      if(old_str == NULL) {
        fprintf(stderr, "Error: unexpected EOF when trying to reach end of quote\n");
        free(str);
        continue;
      }
      if(unclosed_string == NULL) {
        fprintf(stderr, "Error: unexpected EOF when trying to reach end of quote\n");
        free(old_str);
        continue;
      }
      str = join_unclosed_strings(old_str, unclosed_string);
      if(str == NULL) {
        fprintf(stderr, "Error: could not close the quotes\n");
        continue;
      }
    }

    Token *tokens = tokenizer(str);
    if (tokens == NULL) {
      free(str);
      continue;
    }

    ExecutionUnit *units = handle_parsed_units(tokens, &total_cmds);
    if(units == NULL) {
      token_list_free(tokens);
      free(str);
      continue;
    }
    handle_exec_units(units, total_cmds);

    token_list_free(tokens);   
    free_units(units, total_cmds);         
    free(str);
    str = NULL;
  }

  return 0;
}

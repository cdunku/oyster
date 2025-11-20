#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include "tokenizer.h"
#include "helper.h"

#define BUFFER_SIZE 64

// For simplicity sake, we will only retrieve 2 hex digits and 3 octal digits.
char parse_hex_escape(const char *str, size_t start, int *hex_digits) {
  int val = 0;
  int count = 0;

  // Read up to two hex digits
  for (int k = 0; k < 2; k++) {
    char c = str[start + k];
    int digit;

    if (c >= '0' && c <= '9') { digit = c - '0'; }
    else if (c >= 'a' && c <= 'f') { digit = c - 'a' + 10; }
    else if (c >= 'A' && c <= 'F') { digit = c - 'A' + 10; } 
    else { break; }

    val = (val * 16) + digit;
    count++;
  }

  *hex_digits = count;
  return (char)val;
}

char parse_octal_escape(const char *str, size_t start, int *oct_digits) {
  int val = 0;
  int count = 0;

  // Read up to three octal digits
  while (count < 3) {
    char c = str[start + count];
    if (c < '0' || c > '7') { break; }

    val = (val * 8) + (c - '0');
    count++;
  }

  *oct_digits = count;
  return (char)val;
}

Pipeline tokenizer(const char *str) {
  // Declare the initial buffer size for both the arguments vector and token size.
  size_t argv_size = BUFFER_SIZE; 
  size_t token_size = BUFFER_SIZE;

  // Declare the struct for tokenization and initialise everything needed.
  Pipeline t;
  t.cmd = malloc(sizeof(Command_Info));
  if(t.cmd == NULL) {
    fprintf(stderr, "Fatal: failed to allocate command\n");
    exit(EXIT_FAILURE);
  }

  char *token = malloc(token_size * sizeof(char));
  t.cmd->argv = malloc(argv_size * sizeof(char *));
  t.argv_size = 0;

  if(token == NULL || t.cmd->argv  == NULL) {
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
        case '\a': { 
          // Checks whether if there are any characters inputted to the token variable,
          // if so and if we hit a delimiter, parse the token and input it into argv.
          if(j > 0) {
            token[j] = '\0';
            char *tokendup = strdup(token);
            
            if(tokendup == NULL) { error_flush_token(&t, token, "Fatal: failed to allocate memory to prompt"); }
            t.cmd->argv[t.argv_size++] = tokendup;
            j = 0;
          }
          i++;
        }

          continue;

        case '"': double_quotes = true;
        case '\'': {
          // If we are inside " " or ' ' special rules apply.
          in_quotes = true;
          quote = c;
          i++;
          continue;
        }

        case '|':
        case '<': {
          if(j == 0) { fprintf(stderr, "Error: unable to redirect program flow\n"); break; }
          
          char *tokendup = strdup(c == '|' ? "|" : "<");
          if(tokendup == NULL) { error_flush_token(&t, token, "Fatal: failed to allocate memory to prompt"); }

          t.cmd->argv[t.argv_size++] = tokendup;
          i++;
          continue;
        }
        case '>':
        case '2':
        case '&': {
          continue;
        }
      }
    }
    else {
      // Checks whether if inside the double quotes string we encountered a '\'.
      if(double_quotes && str[i] == '\\') {

        char ch = str[i + 1];
        char esc_ch;

        switch (ch) {
          case 'a':  esc_ch = '\a'; i += 2; break;
          case 'b':  esc_ch = '\b'; i += 2; break;
          case 'f':  esc_ch = '\f'; i += 2; break;
          case 'n':  esc_ch = '\n'; i += 2; break;
          case 'r':  esc_ch = '\r'; i += 2; break;
          case 't':  esc_ch = '\t'; i += 2; break;
          case 'v':  esc_ch = '\v'; i += 2; break;
          case '?':  esc_ch = '\?'; i += 2; break;
          case '\\': esc_ch = '\\'; i += 2; break;
          case '\"': esc_ch = '\"'; i += 2; break;
          case '\'': esc_ch = '\''; i += 2; break;
          // Hex format: \xhh -> E.g. \xFF or \xA9
          case 'x': {
            int hex_count = 0;
            esc_ch = parse_hex_escape(str, i + 2, &hex_count);

            if(hex_count > 0) { i += 2 + hex_count; }
            else { esc_ch = 'x'; i += 2; }

            break;
          }
          // Octal format: \ooo -> E.g. \077 or \123
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7': { 
            int oct_count = 0;
            esc_ch = parse_octal_escape(str, i + 1, &oct_count);
            i += 1 + oct_count;
            break;
          }

          default:
            esc_ch = ch;
            i += 2;
            break;
        }

        token[j++] = esc_ch;
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
        vector_free(t.cmd->argv, t.argv_size);
        free(token);

        t.cmd->argv  = NULL;
        t.argv_size = 0;
        return t;

      }

      token = _token;
    } 

    if(t.argv_size == argv_size - 1) {
      argv_size *= 2;
      char **_argv = realloc(t.cmd->argv, argv_size * sizeof(char *));
      
      if(_argv == NULL) {
        vector_free(t.cmd->argv, t.argv_size);
        free(token);

        t.cmd->argv  = NULL;
        t.argv_size = 0;
        return t;
      
      }
      t.cmd->argv  = _argv;
    }
  }

  // If the last character ends with a delimiter that this function hasn't included,
  // then the last character can be deleted and replaced by '\0'. To prevent this we want 
  // to check whether if the last token was tokenized or not.
  
  if(j > 0) {
    token[j] = '\0';
    char *tokendup = strdup(token);
    
    if(tokendup == NULL) { error_flush_token(&t, token, "Fatal: failed to allocate memory to last character"); }
    t.cmd->argv[t.argv_size++] = tokendup;
  }

  // If we are inside the quotes and we haven't ended the string,
  // do not execute the command and display an error.
  
  if(in_quotes) {
  
    fprintf(stdout, "Error: missing %c quote\n", quote);
  
    error_flush_token(&t, token, "");
    t.cmd->argv  = NULL;

    t.argv_size = 0;
    return t;
  }

  t.cmd->argv[t.argv_size] = NULL;

  free(token);
  return t;
}

// Parse the argument vector into multiple parts 

Pipeline parse_cmd(Pipeline t) {

  size_t cmd_amount = 1;
  size_t i = 0;

  while(t.cmd->argv[i] != NULL) {
    if(strcmp(t.cmd->argv[i], "|") == 0) { cmd_amount++; }
    i++;
  }

  Pipeline pipeline;
  pipeline.cmd_count = cmd_amount;
  // We pass in the sizeof Command_Info (since it is the exact amount of memory we need),
  // together with how many different commands will be parsed.
  pipeline.cmd = calloc(pipeline.cmd_count, sizeof(Command_Info));
  if(pipeline.cmd == NULL) {
    error_flush_token(&pipeline, NULL, "Fatal: failed to calloc to parser");
  }

  // Initialise the members inside the Command_Info for the pipeline.cmd array.
  // Allocation will be created in the next arrays if and when needed.
  for(size_t i = 0; i < pipeline.cmd_count; i++) {
    pipeline.cmd[i].argv = NULL;
    pipeline.cmd[i].input = NULL;
    pipeline.cmd[i].output = NULL;
    pipeline.cmd[i].append = 0;
  }
  
  size_t ai = 0; // Index of one array  
  size_t ci = 0; // cmd[] index
  i = 0;

  size_t cmd_capacity = 4;

  pipeline.cmd[ci].argv = malloc(cmd_capacity * sizeof(char *));
  if(pipeline.cmd[ci].argv == NULL) {
    error_flush_token(&pipeline, NULL, "Fatal: failed to allocate memory to vector parsed");
  }

  while(t.cmd->argv[i] != NULL) {
    if(strcmp(t.cmd->argv[i], "|") == 0) {
      pipeline.cmd[ci].argv[ai] = NULL;
      
      ai = 0;
      cmd_capacity = 4;
      ci++;
      i++;

      pipeline.cmd[ci].argv = malloc(cmd_capacity * sizeof(char *));
      if(pipeline.cmd[ci].argv == NULL) {
        error_flush_token(&pipeline, NULL, "Fatal: failed to allocate memory to vector parsed");
      } 
      continue;
    }

    // If ai + 1 is equal to or greater than the cmd_capacity 
    // (which in our case is the \0 in the array), reallocate new memory into the existing array.
    if(ai + 1 >= cmd_capacity) {
      cmd_capacity *= 2;
      char **argv_ = realloc(pipeline.cmd[ci].argv, cmd_capacity * sizeof(char*));
      
      if(argv_ == NULL) {
        error_flush_token(&pipeline, NULL, "Fatal: failed to reallocate memory to parsed prompt");
      }

      pipeline.cmd[ci].argv = argv_;
    }

    pipeline.cmd[ci].argv[ai] = strdup(t.cmd->argv[i]);
    if(pipeline.cmd[ci].argv[ai] == NULL) {
      error_flush_token(&pipeline, NULL, "Fatal: failed to allocate memory to parsed token");
    }
    ai++;
    i++;
  }

  pipeline.cmd[ci].argv[ai] = NULL;

  return pipeline;
}

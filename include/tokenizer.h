#pragma once

#include <stdbool.h>

// STRING - Command or an argument(s); ls -lah.
// OPERATOR - |, <, >, &&, ||.
typedef enum { STRING, OPERATOR } TokenType;

// Types of operators supported
typedef enum {

  OP_PIPE,
  OP_REDIRECT_IN,
  OP_REDIRECT_OUT,
  OP_REDIRECT_OUT_APPEND,
  OP_REDIRECT_ERR,
  OP_REDIRECT_ERR_APPEND,
  OP_REDIRECT_ALL,
  OP_REDIRECT_ALL_APPEND,
  OP_CONDITIONAL_OR,
  OP_CONDITIONAL_AND,
  OP_NONE,

} OperatorType;

// Handle a single token.
typedef struct Token {

  TokenType token_type;
  char *content;
  // Point to the next token.
  struct Token *next;

} Token;

typedef struct RedirectionStream {

  // Redirecting input and output.
  char *file_in;
  char *file_out;
  char *file_err;

  bool stdio_append;
  bool stderr_append;

} RedirectionStream;

// Information about the command being ran.
typedef struct Command {

  char **argv;
  size_t argc;

  RedirectionStream stream;

} Command;

// Handling 1 or more commands using a Pipeline struct, where commands can communicate with each other.
typedef struct Pipeline {
  
  Command *cmds;
  size_t cmds_count;

} Pipeline;

// The main struct that handles the entire execution of a command or commands.
typedef struct ExecutionUnit {

  Pipeline pl;
  OperatorType op_type;

} ExecutionUnit;

Token *tokenizer(const char *str);
Command *parse_cmds(Token *t, size_t *total_cmd);

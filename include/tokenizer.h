#pragma once

#include <stdbool.h>

typedef enum { STRING, OPERATOR } TokenType;

typedef struct Token {

  TokenType type;
  char *content;
  struct Token *next;

} Token;

typedef struct Command {

  char **argv;
  size_t argc;

  // Redirecting input and output
  char *file_in;
  char *file_out;
  bool append;

  // For the pipeline
  struct Command *stdin_cmd;
  struct Command *stdout_cmd;

} Command;

Token *tokenizer(const char *str);
Command *parse_cmds(Token *t);

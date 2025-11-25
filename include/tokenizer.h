#pragma once

typedef enum { STRING, OPERATOR } TokenType;

typedef struct Token {

  TokenType type;
  char *content;
  struct Token *next;

} Token;

typedef struct Command {

  char **argv;
  size_t argc;

  struct Command *stdin_cmd;
  struct Command *stdout_cmd;

} Command;

Token *tokenizer(const char *str);
Command *parse_cmds(Token *t);

#pragma once

typedef struct {

  char **argv;
  char *input;
  char *output;
  int append;

} Command_Info;

typedef struct {

  Command_Info cmd;
  size_t argv_size;

} Pipeline;

Pipeline tokenizer(const char *str);

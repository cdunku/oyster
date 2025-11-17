#pragma once

typedef struct {

  char **argv;
  size_t argv_size;

} Tokenizer;

Tokenizer tokenizer(const char *str);

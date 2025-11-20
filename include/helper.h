#pragma once

#include "tokenizer.h"

void vector_free(char **argv, size_t i);
void error_flush_token(Pipeline* const p, char* token, const char *err_msg);

#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

void vector_free(char **argv, size_t size) {
  for(size_t i = 0; i < size; i++) {
    free(argv[i]);
  }
  free(argv);
  return;
}

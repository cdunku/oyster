#include <stdio.h>
#include <stdlib.h>

#include "helper.h"

void vector_free(char **argv, size_t i) {
  while(argv[i] != NULL) {
    free(argv[i]);
    i++;
  }
  free(argv);
  return;
}

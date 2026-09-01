#include "bfi.h"

void errors_print(const Errors *errors)
{
     for (size_t i = 0; i < errors->count; i++) {
          fprintf(stderr, "error: %s: %s in line %zu, column %zu\n", errors->items[i].filepath, errors->items[i].reason, errors->items[i].line, errors->items[i].col);
     }
}

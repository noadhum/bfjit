#include "bfi.h"

void interpret(Tokens *tokens, unsigned char *memory)
{
     unsigned char *ptr = memory;

     for (size_t i = 0; i < tokens->count; i++) {
          Token current = tokens->items[i];
          switch (current.kind) {
          case TOK_RIGHT:
               ptr += current.operand;
               break;

          case TOK_LEFT:
               ptr -= current.operand;
               break;

          case TOK_ADD:
               *ptr += current.operand;
               break;

          case TOK_SUB:
               *ptr -= current.operand;
               break;

          case TOK_PUT:
               putchar(*ptr);
               break;

          case TOK_GET:
               *ptr = getchar();
               break;

          case TOK_JZ:
               if (!*ptr) i = current.operand;
               break;

          case TOK_JNZ:
               if (*ptr) i = current.operand;
               break;

          case TOK_ZERO:
               *ptr = 0;
               break;
          }
     }
}

#define BASIC_IMPLEMENTATION
#include "bfjit.h"

void usage(const char *prog)
{
     fprintf(stderr, "usage: %s <input.bf>\n", prog);
}

int main(int argc, char **argv)
{
     int result = 0;

     String_Builder source = {0};
     Lexer lexer = {0};
     unsigned char *memory = NULL;

     const char *prog = *basic_shift_args(&argc, &argv);
     if (argc != 1) {
          usage(prog);
          basic_return_defer(1);
     }

     const char *path = *basic_shift_args(&argc, &argv);
     if (!basic_load_file(path, &source)) {
          fprintf(stderr, "error: %s unable to load file\n", path);
          basic_return_defer(1);
     }

     lexer.filepath = path;
     lexer.source = &source;
     lexer_tokenize(&lexer);

     if ((memory = malloc(30000)) == NULL) {
          fprintf(stderr, "error: %s unable to allocate memory\n", path);
          basic_return_defer(1);
     }

     if (!errs_empty(&lexer.errors)) {
          errs_print(&lexer.errors);
          basic_return_defer(1);
     }

     for (size_t i = 0; i < lexer.tokens.count; i++) {
          printf("%4zu: Token('%c', %zu);\n", i, lexer.tokens.items[i].kind, lexer.tokens.items[i].operand);
     }

defer:
     if (source.items) sb_free(&source);
     if (memory) free(memory);
     return result;
}

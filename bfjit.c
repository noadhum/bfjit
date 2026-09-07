#include <time.h>

#define BFJIT_IMPLEMENTATION
#include "bfjit.h"

#ifndef JIT_MEMORY_SIZE
#define JIT_MEMORY_SIZE 30000
#endif // JIT_MEMORY_SIZE

void usage(const char *prog)
{
     fprintf(stderr, "usage: %s <input.bf>\n", prog);
}

int main(int argc, char **argv)
{
     int result = 0;

     String_Builder source = {0};
     Lexer lexer = {0};
     void *memory = NULL;
     String_Builder code = {0};

     const char *prog = *basic_shift_args(&argc, &argv);
     if (argc != 1) {
          usage(prog);
          basic_return_defer(1);
     }

     const char *path = *basic_shift_args(&argc, &argv);
     if (!basic_load_file(path, &source)) {
          fprintf(stderr, "error: %s: unable to load file\n", path);
          basic_return_defer(1);
     }

     lexer.filepath = path;
     lexer.source = &source;
     lexer_tokenize(&lexer);

     if ((memory = malloc(JIT_MEMORY_SIZE)) == NULL) {
          fprintf(stderr, "error: %s: unable to allocate memory\n", path);
          basic_return_defer(1);
     }

     if (!stack_empty(&lexer.errors)) {
          errs_print(&lexer.errors);
          basic_return_defer(1);
     }

     clock_t compile_start = clock();
     generate_code(&lexer.tokens, &code);
     clock_t compile_end = clock();

     clock_t execution_start = clock();
     if (!run_code(&code, memory)) {
          fprintf(stderr, "error: unable to run code\n", path);
          basic_return_defer(1);
     };
     clock_t execution_end = clock();

     putchar('\n');
     printf("Compilation Time:     %f\n", (double)(compile_end - compile_start) / CLOCKS_PER_SEC);
     printf("Execution Time:       %f\n", (double)(execution_end - execution_start) / CLOCKS_PER_SEC);
     printf("Total:                %f\n", ((double)(compile_end - compile_start) + (double)(execution_end - execution_start)) / CLOCKS_PER_SEC);

defer:
     if (source.items) sb_free(&source);
     if (memory) free(memory);
     if (code.items) sb_free(&code);
     return result;
}

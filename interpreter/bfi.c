#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define SB_IMPLEMENTATION
#include "../modules/sb.h"

#define SV_IMPLEMENTATION
#include "../modules/sv.h"

#define BFI_ASSERT(expr, message) do {if (!(expr)) {fprintf(stderr, "bfi: %s\n", message); abort();}} while(0)

#define BFI_PANIC(message) BFI_ASSERT(false, message)

#define DA_INIT_CAP 128

#define da_reserve(da, new_cap)                                         \
     do {                                                               \
          if ((da)->capacity < (new_cap)) {                             \
               if ((da)->capacity == 0) (da)->capacity = DA_INIT_CAP;   \
               while ((da)->capacity < (new_cap)) {                     \
                    (da)->capacity *= 2;                                \
               }                                                        \
               (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
               BFI_ASSERT((da)->items != NULL, "unable to allocate memory"); \
          }                                                             \
     } while(0)

#define da_append(da, item) do {da_reserve((da), (da)->count + 1); (da)->items[(da)->count++] = item;} while(0)
#define da_free(da) do {free((void *)(da)->items); (da)->items = NULL;} while(0)

#define stack_push(stack, item) da_append(stack, item)
#define stack_pop(stack) ((stack)->items[--(stack)->count])
#define stack_peek(stack) ((stack)->items[(stack)->count - 1])
#define stack_empty(stack) ((stack)->count == 0)

#define return_defer(value) do {result = (value); goto defer;} while(0)

typedef struct {
     String_View path;
     String_Builder contents;
} File;

bool file_read(const char *path, File *file)
{
     bool result = true;
     char *data = NULL;


     FILE *fp = fopen(path, "rb");
     if (fp == NULL) return_defer(false);
     if (fseek(fp, 0, SEEK_END) != 0) return_defer(false);

     long long fs = ftell(fp);
     if (fs == -1L) return_defer(false);
     if (fseek(fp, 0, SEEK_SET) != 0) return_defer(false);
     if ((data = malloc(fs)) == NULL) return_defer(false);

     fread(data, fs, 1, fp);
     if (ferror(fp)) return_defer(false);

     file->path = sv_from_cstr(path);
     file->contents = sb_from_parts(data, fs);

defer:
     if (fp) fclose(fp);
     if (!result) fprintf(stderr, "bfi: %s: unable to read file\n", path);
     return result;
}

void file_free(File *file)
{
     sb_free(&file->contents);
}

typedef struct {
     File *file;
     uint64_t pos;
     uint32_t line, col;
} Lexer;

typedef enum {
     INST_RIGHT     = '>',
     INST_LEFT      = '<',
     INST_ADD       = '+',
     INST_SUB       = '-',
     INST_OUTPUT    = '.',
     INST_INPUT     = ',',
     INST_JZ        = '[',
     INST_JNZ       = ']',
     INST_ZERO,     // [-] or [+]
} Inst_Kind;

typedef struct {
     Inst_Kind kind;
     uint64_t operand, pos;
     uint32_t line, col;
} Inst;

bool is_bf_cmd(char c)
{
     return strrchr("><+-.,[]", c) != NULL;
}

bool is_bf_cmd_mergeable(char c)
{
     return strrchr("><+-", c) != NULL;
}

#define inst_peek(lexer, inst) inst_peekn(lexer, 0, inst)

bool inst_peekn(Lexer *lexer, uint64_t n, Inst *inst)
{
     bool result = true;
     uint64_t pos = lexer->pos;
     uint32_t line = lexer->line;
     uint32_t col = lexer->col;

     if (line == 0) line++;
     if (col == 0)  col++;

     for (uint64_t i = 0; i <= n; i++) {
          while (pos < lexer->file->contents.count && !is_bf_cmd(lexer->file->contents.data[pos])) {
               if (lexer->file->contents.data[pos] == '\n') {
                    line++;
                    col = 1;
               } else {
                    col++;
               }
               pos++;
          }

          if (pos >= lexer->file->contents.count) return_defer(false);

          inst->kind = lexer->file->contents.data[pos];
          inst->operand = 1;
          inst->pos = pos;
          inst->line = line;
          inst->col = col;

          pos++;
          col++;

          if (is_bf_cmd_mergeable(inst->kind)) {
               while (pos < lexer->file->contents.count) {
                    if (!is_bf_cmd(lexer->file->contents.data[pos])) {
                         if (lexer->file->contents.data[pos] == '\n') {
                              line++;
                              col = 1;
                         } else {
                              col++;
                         }
                         pos++;
                         continue;
                    }

                    if (lexer->file->contents.data[pos] == inst->kind) {
                         pos++;
                         col++;
                         inst->operand++;
                    } else {
                         break;
                    }
               }
          }
     }

defer:
     return result;
}

bool inst_next(Lexer *lexer, Inst *inst)
{
     bool result = true;

     if (lexer->line == 0) lexer->line++;
     if (lexer->col == 0)  lexer->col++;

     while (lexer->pos < lexer->file->contents.count && !is_bf_cmd(lexer->file->contents.data[lexer->pos])) {
          if (lexer->file->contents.data[lexer->pos] == '\n') {
               lexer->line++;
               lexer->col = 1;
          } else {
               lexer->col++;
          }
          lexer->pos++;
     }

     if (lexer->pos >= lexer->file->contents.count) return_defer(false);

     inst->kind = lexer->file->contents.data[lexer->pos];
     inst->operand = 1;
     inst->pos = lexer->pos;
     inst->line = lexer->line;
     inst->col = lexer->col;

     lexer->pos++;
     lexer->col++;

     if (is_bf_cmd_mergeable(inst->kind)) {
          while (lexer->pos < lexer->file->contents.count) {
               if (!is_bf_cmd(lexer->file->contents.data[lexer->pos])) {
                    if (lexer->file->contents.data[lexer->pos] == '\n') {
                         lexer->line++;
                         lexer->col = 1;
                    } else {
                         lexer->col++;
                    }
                    lexer->pos++;
                    continue;
               }

               if (lexer->file->contents.data[lexer->pos] == inst->kind) {
                    lexer->pos++;
                    lexer->col++;
                    inst->operand++;
               } else {
                    break;
               }
          }
     } else if (inst->kind == INST_JZ) {
          Inst next, next_next;

          if (inst_peekn(lexer, 0, &next) && inst_peekn(lexer, 1, &next_next)) {
               if ((next.kind == INST_ADD || next.kind == INST_SUB) && next.operand == 1 && (next_next.kind == INST_JNZ)) {
                    lexer->pos = next_next.pos + 1;
                    lexer->line = next_next.line;
                    lexer->col = next_next.col + 1;

                    inst->kind = INST_ZERO;
                    inst->operand = 1;
               }
          }
     }

defer:
     return result;
}

typedef struct {
     Inst *items;
     size_t count;
     size_t capacity;
     // Error handling
     const char *path;
} Insts;

typedef struct {
     size_t *items;
     size_t count;
     size_t capacity;
} Bracket_Pos;

void tokenize(Lexer *lexer, Insts *insts)
{
     insts->path = lexer->file->path.data;
     Bracket_Pos bp = {0};
     Inst inst;

     for (size_t i = 0; inst_next(lexer, &inst); i++) {
          if (inst.kind == INST_JZ) {
               stack_push(&bp, i);
          } else if (inst.kind == INST_JNZ) {
               if (!stack_empty(&bp)) {
                    inst.operand = stack_peek(&bp);
                    insts->items[stack_pop(&bp)].operand = i;
               }
          }
          da_append(insts, inst);
     }
}

typedef struct {
     Inst *items;
     size_t count;
     size_t capacity;
} Bracket_Stack;

bool parse(Insts *insts)
{
     bool result = true;
     Bracket_Stack bs = {0};

     for (size_t i = 0; i < insts->count; i++) {
          if (insts->items[i].kind == INST_JZ) {
               stack_push(&bs, insts->items[i]);
          } else if (insts->items[i].kind == INST_JNZ) {
               if (!stack_empty(&bs)) {
                    stack_pop(&bs);
               } else {
                    fprintf(stderr, "%s:%u:%u: unexpected closing bracket ']'\n", insts->path, insts->items[i].line, insts->items[i].col);
                    result = false;
               }
          }
     }

     for (size_t i = 0; i < bs.count; i++) {
          fprintf(stderr, "%s:%u:%u: unclosed bracket '['\n", insts->path, insts->items[i].line, insts->items[i].col);
          result = false;
     }

     return result;
}

void interpret(Insts *insts, uint8_t *tape)
{
     uint8_t *ptr = tape;

     for (size_t i = 0; i < insts->count; i++) {
          Inst current = insts->items[i];
          switch (current.kind) {
          case INST_RIGHT:
               ptr += current.operand;
               break;

          case INST_LEFT:
               ptr -= current.operand;
               break;

          case INST_ADD:
               *ptr += current.operand;
               break;

          case INST_SUB:
               *ptr -= current.operand;
               break;

          case INST_OUTPUT:
               putchar(*ptr);
               break;

          case INST_INPUT:
               *ptr = getchar();
               break;

          case INST_JZ:
               if (*ptr == 0) i = insts->items[i].operand;
               break;

          case INST_JNZ:
               if (*ptr != 0) i = insts->items[i].operand;
               break;

          case INST_ZERO:
               *ptr = 0;
               break;

          default: break;
          }
     }
}

#define TAPE_SIZE 30000

int main(int argc, char **argv)
{
     int result = 0;

     File file = {0};
     Insts insts = {0};

     uint8_t *tape = NULL;

     if (argc != 2) {
          fprintf(stderr, "usage: %s <input.bf>", argv[0]);
          return_defer(1);
     }

     argc--, argv++;
     if (!file_read(argv[0], &file)) return_defer(1);

     if (!sv_ends_with(file.path, ".b") && !sv_ends_with(file.path, ".bf")) {
          fprintf(stderr, "bfi: %s: file format not recognized\n", file.path.data);
          return_defer(1);
     }

     Lexer lexer = {
         .file = &file
     };

     tokenize(&lexer, &insts);
     if (!parse(&insts)) return_defer(1);

     tape = malloc(TAPE_SIZE * sizeof(uint8_t));
     BFI_ASSERT(tape != NULL, "unable to allocate tape");
     memset(tape, 0, TAPE_SIZE);

     interpret(&insts, tape);

defer:
     if (file.contents.data) sb_free(&file.contents);
     if (insts.items) da_free(&insts);
     if (tape) free(tape);
     return result;
}

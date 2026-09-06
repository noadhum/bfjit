#define BASIC_IMPLEMENTATION
#include "basic.h"

typedef enum {
     TOK_RIGHT      = '>',
     TOK_LEFT       = '<',
     TOK_ADD        = '+',
     TOK_SUB        = '-',
     TOK_PUT        = '.',
     TOK_GET        = ',',
     TOK_JZ         = '[',
     TOK_JNZ        = ']',
     TOK_ZERO,      // [-] or [+]
} Token_Kind;

typedef struct {
     Token_Kind kind;
     size_t operand;
} Token;

typedef struct {
     Token *items;
     size_t count;
     size_t capacity;
} Tokens;

typedef struct {
     size_t *items;
     size_t count;
     size_t capacity;
} Bracket_Pos;

#define bp_push stack_push
#define bp_pop stack_pop
#define bp_peek_last stack_peek_last
#define bp_empty stack_empty

typedef struct {
     const char *filepath;
     size_t line;
     size_t col;
     const char *reason;
} Error;

typedef struct {
     Error *items;
     size_t count;
     size_t capacity;
} Errors;

#define errs_push stack_push
#define errs_empty stack_empty

void errs_print(const Errors *errors)
{
     for (size_t i = 0; i < errors->count; i++) {
          fprintf(stderr, "error: %s: %s in line %zu, column %zu\n",
                  errors->items[i].filepath,
                  errors->items[i].reason,
                  errors->items[i].line,
                  errors->items[i].col);
     }
}

typedef struct {
     const char *filepath;
     String_Builder *source;
     size_t pos;
     size_t line;
     size_t col;
     Tokens tokens;
     Errors errors;
} Lexer;

#define is_bf_cmd(c) (strchr("><+-.,[]", (c)) != NULL)
#define is_bf_cmd_mergeable(c) (strchr("><+-", (c)) != NULL)

void lexer_push(Lexer *lexer, Token token)
{
     da_append(&lexer->tokens, token);
}

void lexer_skip_non_bf(Lexer *lexer)
{
     if (lexer->line == 0) lexer->line++;
     if (lexer->col == 0)  lexer->col++;

     while (lexer->pos < lexer->source->count && !is_bf_cmd(lexer->source->items[lexer->pos])) {
          char current = lexer->source->items[lexer->pos];
          if (current == '\n') {
               lexer->line++;
               lexer->col = 1;
          } else {
               lexer->col++;
          }
          lexer->pos++;
     }
}

bool lexer_peekn(Lexer lexer, size_t n, Token *token)
{
     if (lexer.line == 0) lexer.line++;
     if (lexer.col == 0)  lexer.col++;

     for (size_t i = 0; i <= n; i++) {
          lexer_skip_non_bf(&lexer);

          if (lexer.pos >= lexer.source->count) return false;

          token->kind = lexer.source->items[lexer.pos];
          token->operand = 1;

          lexer.pos++;
          lexer.col++;

          if (is_bf_cmd_mergeable(token->kind)) {
               while (lexer.pos < lexer.source->count) {
                    if (!is_bf_cmd(lexer.source->items[lexer.pos])) {
                         if (lexer.source->items[lexer.pos] == '\n') {
                              lexer.line++;
                              lexer.col = 1;
                         } else {
                              lexer.col++;
                         }
                         lexer.pos++;
                         continue;
                    }

                    if (lexer.source->items[lexer.pos] == token->kind) {
                         lexer.pos++;
                         lexer.col++;
                         token->operand++;
                    } else {
                         break;
                    }
               }
          }
     }

     return true;
}

bool lexer_peek(Lexer lexer, Token *token)
{
     return lexer_peekn(lexer, 0, token);
}

bool lexer_peekn_loc(Lexer lexer, size_t n, Token *token, size_t *pos, size_t *line, size_t *col)
{
     if (lexer.line == 0) lexer.line++;
     if (lexer.col == 0)  lexer.col++;

     for (size_t i = 0; i <= n; i++) {
          lexer_skip_non_bf(&lexer);

          if (lexer.pos >= lexer.source->count) return false;

          token->kind = lexer.source->items[lexer.pos];
          token->operand = 1;

          *pos = lexer.pos;
          *line = lexer.line;
          *col = lexer.col;

          lexer.pos++;
          lexer.col++;

          if (is_bf_cmd_mergeable(token->kind)) {
               while (lexer.pos < lexer.source->count) {
                    if (!is_bf_cmd(lexer.source->items[lexer.pos])) {
                         if (lexer.source->items[lexer.pos] == '\n') {
                              lexer.line++;
                              lexer.col = 1;
                         } else {
                              lexer.col++;
                         }
                         lexer.pos++;
                         continue;
                    }

                    if (lexer.source->items[lexer.pos] == token->kind) {
                         lexer.pos++;
                         lexer.col++;
                         token->operand++;
                    } else {
                         break;
                    }
               }
          }
     }

     return true;
}

bool lexer_next(Lexer *lexer, Token *token)
{
     if (lexer->line == 0) lexer->line++;
     if (lexer->col == 0)  lexer->col++;

     lexer_skip_non_bf(lexer);

     if (lexer->pos >= lexer->source->count) return false;

     token->kind = lexer->source->items[lexer->pos];
     token->operand = 1;

     lexer->pos++;
     lexer->col++;

     if (is_bf_cmd_mergeable(token->kind)) {
          while (lexer->pos < lexer->source->count) {
               if (!is_bf_cmd(lexer->source->items[lexer->pos])) {
                    if (lexer->source->items[lexer->pos] == '\n') {
                         lexer->line++;
                         lexer->col = 1;
                    } else {
                         lexer->col++;
                    }
                    lexer->pos++;
                    continue;
               }

               if (lexer->source->items[lexer->pos] == token->kind) {
                    lexer->pos++;
                    lexer->col++;
                    token->operand++;
               } else {
                    break;
               }
          }
     } else if (token->kind == TOK_JZ) {
          Token next, next_next;
          size_t next_next_pos, next_next_line, next_next_col;

          if (lexer_peekn(*lexer, 0, &next) && lexer_peekn_loc(*lexer, 1, &next_next, &next_next_pos, &next_next_line, &next_next_col)) {
               if ((next.kind == TOK_ADD || next.kind == TOK_SUB) && next.operand == 1 && (next_next.kind == TOK_JNZ)) {
                    lexer->pos = next_next_pos + 1;
                    lexer->line = next_next_line;
                    lexer->col = next_next_col + 1;

                    token->kind = TOK_ZERO;
                    token->operand = 1;
               }
          }
     }

     return true;
}

void lexer_tokenize(Lexer *lexer)
{
     Bracket_Pos bp = {0};
     Token token;

     for (size_t i = 0; lexer_next(lexer, &token); i++) {
          if (token.kind == TOK_JZ) {
               bp_push(&bp, i);
          } else if (token.kind == TOK_JNZ) {
               if (!bp_empty(&bp)) {
                    token.operand = bp_peek_last(&bp);
                    lexer->tokens.items[bp_pop(&bp)].operand = i;
               } else {
                    errs_push(&lexer->errors, ((Error){lexer->filepath, lexer->line, lexer->col - 1, "unexpected closing bracket ']'"}));
               }
          }
          lexer_push(lexer, token);
     }

     for (size_t i = 0; i < bp.count; i++) {
          errs_push(&lexer->errors, ((Error){lexer->filepath, lexer->line, lexer->col - 1, "unclosed bracket '['"}));
     }
}

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

     interpret(&lexer.tokens, memory);

defer:
     if (source.items) sb_free(&source);
     if (memory) free(memory);
     return result;
}

#ifndef BFJIT_H_
#define BFJIT_H_

#ifdef BFJIT_IMPLEMENTATION
#    define BASIC_IMPLEMENTATION
#endif // BFJIT_IMPLEMENTATION
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
     TOK_ZERO       // [+] or [-]
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

void errs_print(const Errors *errors);

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
#define is_bf_cmd_mergeable(c) (strchr("><+-.,", (c)) != NULL)

typedef struct {
     size_t *items;
     size_t count;
     size_t capacity;
} Bracket_Pos;

void lexer_push(Lexer *lexer, Token token);
void lexer_skip_non_bf(Lexer *lexer);
bool lexer_peekn(Lexer lexer, size_t n, Token *token);
bool lexer_peek(Lexer lexer, Token *token);
bool lexer_peekn_loc(Lexer lexer, size_t n, Token *token, size_t *pos, size_t *line, size_t *col);
bool lexer_next(Lexer *lexer, Token *token);
void lexer_tokenize(Lexer *lexer);

#define BFJIT_CPU_X86_64      0
#define BFJIT_CPU_ARM64       1

#if defined(__x86_64__) || defined(_M_X64)
#    define BFJIT_CPU BFJIT_CPU_X86_64
#elif defined(__aarch64__) || defined(_M_ARM64)
#    define BFJIT_CPU BFJIT_CPU_ARM64
#else
#    error "bfjit doesn't support this architecture yet"
#endif

#if BFJIT_CPU == BFJIT_CPU_X86_64
#    define X86_64_IMPLEMENTATION
#    include "bfjit_x86_64.h"

#    define generate_code x86_64_generate_code

void x86_64_generate_code(Tokens *tokens, String_Builder *sb);

#elif BFJIT_CPU_ARM64
#    error "bfjit doesn't support ARM64 yet"
#endif

#ifdef _WIN32
#    include <memoryapi.h>

#    define JIT_MAP_FAILED NULL

#    define JIT_MAP_CREATE(count) VirtualAlloc(NULL, (count), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)
#    define JIT_MAP_FREE(map, count) VirtualFree((map), (count), MEM_RELEASE)
#else
#    include <sys/mman.h>

#    define JIT_MAP_FAILED MAP_FAILED

#    define JIT_MAP_CREATE(count) mmap(NULL, (count), PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
#    define JIT_MAP_FREE(map, count) munmap((map), (count))
#endif

#endif // BFJIT_H_

#ifdef BFJIT_IMPLEMENTATION

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
               stack_push(&bp, i);
          } else if (token.kind == TOK_JNZ) {
               if (!stack_empty(&bp)) {
                    size_t jz_idx = stack_pop(&bp);
                    token.operand = jz_idx;
                    lexer->tokens.items[jz_idx].operand = i;
               } else {
                    stack_push(&lexer->errors, ((Error){lexer->filepath, lexer->line, lexer->col - 1, "unexpected closing bracket ']'"}));
               }
          }
          lexer_push(lexer, token);
     }

     for (size_t i = 0; i < bp.count; i++) {
          stack_push(&lexer->errors, ((Error){lexer->filepath, lexer->line, lexer->col - 1, "unclosed bracket '['"}));
     }
}

typedef struct {
     size_t operand_addr;
     int32_t operand;
} Bracket_Jump;

typedef struct {
     Bracket_Jump *items;
     size_t count;
     size_t capacity;
} Bracket_Jumps;

#if BFJIT_CPU == BFJIT_CPU_X86_64
void x86_64_generate_code(Tokens *tokens, String_Builder *sb)
{
     Bracket_Pos bp = {0};

     for (size_t i = 0; i < tokens->count; i++) {
          Token current = tokens->items[i];
          switch (current.kind) {
          case TOK_RIGHT: {
#              ifdef _WIN32
                    x86_64_emit_add_val(sb, REG_RCX, current.operand);     // add rcx, current.operand
#              else
                    x86_64_emit_add_val(sb, REG_RDI, current.operand);     // add rdi, current.operand
#              endif
          } break;

          case TOK_LEFT: {
#              ifdef _WIN32
                    x86_64_emit_sub_val(sb, REG_RCX, current.operand);     // sub rcx, current.operand
#              else
                    x86_64_emit_sub_val(sb, REG_RDI, current.operand);     // sub rdi, current.operand
#              endif
          } break;

          case TOK_ADD: {
#              ifdef _WIN32
                    x86_64_emit_add_byte_val(sb, REG_RCX, current.operand);     // add byte[rcx], current.operand
#              else
                    x86_64_emit_add_byte_val(sb, REG_RDI, current.operand);     // add byte[rdi], current.operand
#              endif
          } break;

          case TOK_SUB: {
#              ifdef _WIN32
                    x86_64_emit_sub_byte_val(sb, REG_RCX, current.operand);     // sub byte[rcx], current.operand
#              else
                    x86_64_emit_sub_byte_val(sb, REG_RDI, current.operand);     // sub byte[rdi], current.operand
#              endif
          } break;

          case TOK_PUT: {
               for (size_t i = 0; i < current.operand; i++) {
#                   ifdef _WIN32
#                        error "add TOK_PUT for Windows"
#                   else
                         x86_64_emit_push_r64(sb, REG_RDI);                     // push rdi
                         x86_64_emit_mov_val(sb, REG_RAX, 1);                   // mov rax, 1
                         x86_64_emit_mov_r64(sb, REG_RSI, REG_RDI);             // mov rsi, rdi
                         x86_64_emit_mov_val(sb, REG_RDI, 1);                   // mov rdi, 1
                         x86_64_emit_mov_val(sb, REG_RDX, 1);                   // mov rdx, 1
                         x86_64_emit_syscall(sb);                               // syscall
                         x86_64_emit_pop_r64(sb, REG_RDI);                      // pop rdi
#              endif
               }
          } break;

          case TOK_GET: {
               for (size_t i = 0; i < current.operand; i++) {
#                   ifdef _WIN32
#                        error "add TOK_GET for Windows"
#                   else
                         x86_64_emit_push_r64(sb, REG_RDI);                // push rdi
                         x86_64_emit_xor_r64(sb, REG_RAX, REG_RAX);        // xor rax, rax
                         x86_64_emit_mov_r64(sb, REG_RSI, REG_RDI);        // mov rsi, rdi
                         x86_64_emit_xor_r64(sb, REG_RDI, REG_RDI);        // xor rdi, rdi
                         x86_64_emit_mov_val(sb, REG_RDX, 1);              // mov rdx, 1
                         x86_64_emit_syscall(sb);                          // syscall
                         x86_64_emit_pop_r64(sb, REG_RDI);                 // pop rdi
#                   endif
               }
          } break;

          case TOK_JZ: {
                    x86_64_emit_cmp_byte_val(sb, REG_RDI, 0);         // cmp byte[rdi], 0
                    x86_64_emit_byte_many(sb, "\x0F\x84", 2);         // jz
                    stack_push(&bp, sb->count);
                    x86_64_emit_byte_many(sb, "\x00\x00\x00\x00", 4);
          } break;

          case TOK_JNZ: {
                    x86_64_emit_cmp_byte_val(sb, REG_RDI, 0);         // cmp byte[rdi], 0
                    x86_64_emit_byte_many(sb, "\x0F\x85", 2);         // jnz

                    int32_t jz_operand_addr = stack_pop(&bp);
                    int32_t jz_operand = sb->count - jz_operand_addr;
                    memcpy(sb->items + jz_operand_addr,  &jz_operand, sizeof(jz_operand));

                    int32_t jnz_operand = jz_operand_addr - sb->count;
                    sb_reserve(sb, sb->count + 4);
                    memcpy(sb->items + sb->count, &jnz_operand, sizeof(jnz_operand));
                    sb->count += 4;
          } break;

          case TOK_ZERO: {
#              ifdef _WIN32
                    x86_64_emit_mov_byte_val(sb, REG_RCX, 0);         // mov byte[rcx], 0
#              else
                    x86_64_emit_mov_byte_val(sb, REG_RDI, 0);         // mov byte[rdi], 0
#              endif
          } break;

          }
     }

     x86_64_emit_byte(sb, '\xC3');                                    // ret
}

#elif BFJIT_CPU == BFJIT_CPU_ARM64
#    error "bfjit doesn't have ARM64 codegen yet"
#endif

bool run_code(String_Builder *sb, void *memory)
{
     bool result = true;

     void *map = JIT_MAP_CREATE(sb->count);
     if (map == JIT_MAP_FAILED) basic_return_defer(false);

     memcpy(map, sb->items, sb->count);
     void (*prog)(void*) = map;
     prog(memory);

defer:
     JIT_MAP_FREE(map, sb->count);
     return result;
}

#endif // BFJIT_IMPLEMENTATION

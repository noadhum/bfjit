/*
  basic.h - (https://www.github.com/noadhum/basic.h)
  Personal single-header library in C
 */

#ifndef BASIC_H_
#define BASIC_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BASICDEF
#define BASICDEF
#endif // BASICDEF

#ifndef BASIC_NO_PREFIX
#    define da_reserve basic_da_reserve
#    define da_append basic_da_append
#    define da_append_many basic_da_append_many
#    define da_free basic_da_free
#    define da_push basic_da_push
#    define da_pop basic_da_pop
#    define da_peek_first basic_da_peek_first
#    define da_peek_last basic_da_peek_last

#    define stack_push basic_stack_push
#    define stack_pop basic_stack_pop
#    define stack_peek_first basic_stack_peek_first
#    define stack_peek_last basic_stack_peek_last
#    define stack_empty basic_stack_empty

#    define String_Builder Basic_String_Builder
#    define SB BASIC_SB
#    define sb_from_parts basic_sb_from_parts
#    define sb_from_cstr basic_sb_from_cstr
#    define sb_append_cstr basic_sb_append_cstr
#    define sb_append_char basic_sb_append_char
#    define sb_append_sb basic_sb_append_sb
#    define sb_reserve basic_sb_reserve
#    define sb_free basic_sb_free
#    define sb_eq_cstr basic_sb_eq_cstr
#    define sb_eq_cstr_ignorecase basic_sb_eq_cstr_ignorecase
#    define sb_eq_sb basic_sb_eq_sb
#    define sb_eq_sb_ignorecase basic_sb_eq_sb_ignorecase

#    define String_View Basic_String_View
#    define SV BASIC_SV
#    define sv_from_parts basic_sv_from_parts
#    define sv_from_cstr basic_sv_from_cstr
#    define sv_from_sb basic_sv_from_sb
#    define sv_substr basic_sv_substr
#    define sv_find basic_sv_find
#    define sv_rfind basic_sv_rfind
#    define sv_eq_cstr basic_sv_eq_cstr
#    define sv_eq_sv basic_sv_eq_sv
#    define sv_starts_with basic_sv_starts_with
#    define sv_ends_with basic_sv_ends_with

#    define load_file basic_load_file
#endif // BASIC_NO_PREFIX

#ifdef BASIC_INCLUDE_STDINT
#include <inttypes.h>
#include <stdint.h>

#    define i8 int8_t
#    define s8 int8_t
#    define u8 uint8_t
#    define i16 int16_t
#    define s16 int16_t
#    define u16 uint16_t
#    define i32 int32_t
#    define s32 int32_t
#    define u32 uint32_t
#    define i64 int64_t
#    define s64 int64_t
#    define u64 uint64_t
#endif // BASIC_INCLUDE_STDINT

#ifndef BASIC_REALLOC
#define BASIC_REALLOC realloc
#endif // BASIC_REALLOC

#ifndef BASIC_FREE
#define BASIC_FREE free
#endif // BASIC_FREE

#define BASIC_PANIC(message) basic__panic(__FILE__, __func__, __LINE__, (message))
void basic__panic(const char *file, const char *func, size_t line, const char *message);

#define BASIC_ASSERT(expr, message) basic__assert(__FILE__, __func__, __LINE__, (expr), (message))
void basic__assert(const char *file, const char *func, size_t line, bool expr, const char *message);

#define basic_return_defer(value) do {result = (value); goto defer;} while (0)

// Inspired by tsoding/nob.h's nob_shift()
#define basic_shift(xs, sz) (BASIC_ASSERT(sz > 0, "unable to shift anymore"), (sz)--, (xs)++)
#define basic_shift_args(argc, argv) basic_shift(*argv, *argc)

// Initial capacity of dynamic array
#ifndef BASIC_DA_INIT_CAP
#define BASIC_DA_INIT_CAP 128
#endif // BASIC_DA_INIT_CAP

#define basic_da_reserve(da, new_cap)                                   \
     do {                                                               \
          if ((da)->capacity < (new_cap)) {                             \
               if ((da)->capacity == 0) (da)->capacity = BASIC_DA_INIT_CAP; \
               while ((da)->capacity < (new_cap)) {                     \
                    (da)->capacity *= 2;                                \
               }                                                        \
               (da)->items = BASIC_REALLOC((da)->items, (da)->capacity * sizeof(*(da)->items)); \
               BASIC_ASSERT((da)->items != NULL, "unable to allocate memory"); \
          }                                                             \
     } while (0)

// Append a single item into dynamic array
#define basic_da_append(da, item)                       \
     do {                                               \
          basic_da_reserve((da), (da)->count + 1);      \
          (da)->items[(da)->count++] = (item);          \
     } while (0)

// Append multiple items into dynamic array
#define basic_da_append_many(da, new_items, items_count)                \
     do {                                                               \
          basic_da_reserve((da), (da)->count + (items_count));          \
          memcpy((da)->items + (da)->count, (new_items), (items_count) * sizeof(*(da)->items)); \
          (da)->count += (items_count);                                 \
     } while (0)

#define basic_da_free(da) BASIC_FREE((da)->items)

// Push an item into dynamic array
#define basic_da_push basic_da_append
// Pop an item from dynamic array
#define basic_da_pop(da) (BASIC_ASSERT((da)->count > 0, "unable to pop an empty dynamic array"), (da)->items[--(da)->count])
// Peek the first pushed item in dynamic array
#define basic_da_peek_first(da) (BASIC_ASSERT((da)->count > 0, "unable to peek an empty dynamic array"), (da)->items[0])
// Peek the last pushed item in dynamic array
#define basic_da_peek_last(da) (BASIC_ASSERT((da)->count > 0, "unable to peek an empty dynamic array"), (da)->items[(da)->count - 1])

// Stack operations
#define basic_stack_push basic_da_push
#define basic_stack_pop basic_da_pop
#define basic_stack_peek_first basic_da_peek_first
#define basic_stack_peek_last basic_da_peek_last
#define basic_stack_empty(stack) ((stack)->count == 0)

typedef struct {
     char *items;
     size_t count;
     size_t capacity;
} Basic_String_Builder;

#define BASIC_SB(cstr_lit) basic_sb_from_parts(cstr_lit, sizeof(cstr_lit) - 1)

BASICDEF Basic_String_Builder basic_sb_from_parts(const char *data, size_t count);
BASICDEF Basic_String_Builder basic_sb_from_cstr(const char *cstr);
BASICDEF void basic_sb_append_cstr(Basic_String_Builder *sb, const char *cstr);
BASICDEF void basic_sb_append_char(Basic_String_Builder *sb, char c);
BASICDEF void basic_sb_append_sb(Basic_String_Builder *dst, const Basic_String_Builder *src);
BASICDEF void basic_sb_reserve(Basic_String_Builder *sb, size_t new_cap);
BASICDEF void basic_sb_free(Basic_String_Builder *sb);
BASICDEF bool basic_sb_eq_cstr(const Basic_String_Builder *sb, const char *cstr);
BASICDEF bool basic_sb_eq_cstr_ignorecase(const Basic_String_Builder *sb, const char *cstr);
BASICDEF bool basic_sb_eq_sb(const Basic_String_Builder *a, const Basic_String_Builder *b);
BASICDEF bool basic_sb_eq_sb_ignorecase(const Basic_String_Builder *a, const Basic_String_Builder *b);

typedef struct {
     const char *data;
     size_t count;
} Basic_String_View;

#define BASIC_SV(cstr_lit) basic_sv_from_parts(cstr_lit, sizeof(cstr_lit) - 1)

BASICDEF Basic_String_View basic_sv_from_parts(const char *data, size_t count);
BASICDEF Basic_String_View basic_sv_from_cstr(const char *cstr);
BASICDEF Basic_String_View basic_sv_from_sb(const Basic_String_Builder *sb);
BASICDEF Basic_String_View basic_sv_substr(Basic_String_View sv, size_t pos, size_t count);
BASICDEF bool basic_sv_find(Basic_String_View sv, char c, size_t *idx);
BASICDEF bool basic_sv_rfind(Basic_String_View sv, char c, size_t *idx);
BASICDEF bool basic_sv_eq_cstr(Basic_String_View sv, const char *cstr);
BASICDEF bool basic_sv_eq_sv(Basic_String_View a, Basic_String_View b);
BASICDEF bool basic_sv_starts_with(Basic_String_View sv, const char *prefix);
BASICDEF bool basic_sv_ends_with(Basic_String_View sv, const char *suffix);

BASICDEF bool basic_load_file(const char *path, Basic_String_Builder *sb);

#endif // BASIC_H_

#ifdef BASIC_IMPLEMENTATION
#ifndef BASIC_IMPLEMENTED
#define BASIC_IMPLEMENTED

#include <ctype.h>

BASICDEF void basic__panic(const char *file, const char *func, size_t line, const char *message) {
     fprintf(stderr, "%s:%s:%zu: %s\n", file, func, line, message);
     abort();
}

BASICDEF void basic__assert(const char *file, const char *func, size_t line, bool expr, const char *message) {
     if (!expr) {
          fprintf(stderr, "%s:%s:%zu: %s\n", file, func, line, message); abort();
     }
}

BASICDEF Basic_String_Builder basic_sb_from_parts(const char *data, size_t count)
{
     Basic_String_Builder sb = {0};

     if (count != 0) {
          basic_da_reserve(&sb, count + 1);
          basic_da_append_many(&sb, data, count);
     }

     return sb;
}

BASICDEF Basic_String_Builder basic_sb_from_cstr(const char *cstr)
{
     return basic_sb_from_parts(cstr, strlen(cstr));
}

BASICDEF void basic_sb_append_cstr(Basic_String_Builder *sb, const char *cstr)
{
     size_t cstr_count = strlen(cstr);
     basic_da_append_many(sb, cstr, cstr_count);
}

BASICDEF void basic_sb_append_char(Basic_String_Builder *sb, char c)
{
     basic_da_append(sb, c);
}

BASICDEF void basic_sb_append_sb(Basic_String_Builder *dst, const Basic_String_Builder *src)
{
     basic_da_append_many(dst, src->items, src->count);
}

BASICDEF void basic_sb_reserve(Basic_String_Builder *sb, size_t new_cap)
{
     basic_da_reserve(sb, new_cap);
}

BASICDEF void basic_sb_free(Basic_String_Builder *sb)
{
     basic_da_free(sb);
}

BASICDEF bool basic_sb_eq_cstr(const Basic_String_Builder *sb, const char *cstr)
{
     size_t cstr_count = strlen(cstr);
     if (sb->count != cstr_count) return false;
     return memcmp(sb->items, cstr, cstr_count) == 0;
}

BASICDEF bool basic_sb_eq_cstr_ignorecase(const Basic_String_Builder *sb, const char *cstr)
{
     size_t cstr_count = strlen(cstr);
     if (sb->count != cstr_count) return false;

     for (size_t i = 0; i < sb->count; i++) {
          if (tolower((unsigned char)sb->items[i]) != tolower((unsigned char)cstr[i])) {
               return false;
          }
     }

     return true;
}

BASICDEF bool basic_sb_eq_sb(const Basic_String_Builder *a, const Basic_String_Builder *b)
{
     if (a->count != b->count) return false;
     return memcmp(a->items, b->items, a->count) == 0;
}

BASICDEF bool basic_sb_eq_sb_ignorecase(const Basic_String_Builder *a, const Basic_String_Builder *b)
{
     if (a->count != b->count) return false;

     for (size_t i = 0; i < a->count; i++) {
          if (tolower((unsigned char)a->items[i]) != tolower((unsigned char)b->items[i])) {
               return false;
          }
     }

     return true;
}

BASICDEF Basic_String_View basic_sv_from_parts(const char *data, size_t count)
{
     return (Basic_String_View) {
          .data = data,
          .count = count
     };
}

BASICDEF Basic_String_View basic_sv_from_cstr(const char *cstr)
{
     return basic_sv_from_parts(cstr, strlen(cstr));
}

BASICDEF Basic_String_View basic_sv_from_sb(const Basic_String_Builder *sb)
{
     return basic_sv_from_parts(sb->items, sb->count);
}

BASICDEF Basic_String_View basic_sv_substr(Basic_String_View sv, size_t pos, size_t count)
{
     return basic_sv_from_parts(&sv.data[pos], count);
}

BASICDEF bool basic_sv_find(Basic_String_View sv, char c, size_t *idx)
{
     for (size_t i = 0; i < sv.count; i++) {
          if (sv.data[i] == c) {
               *idx = i;
               return true;
          }
     }

     return false;
}

BASICDEF bool basic_sv_rfind(Basic_String_View sv, char c, size_t *idx)
{
     bool found = false;

     for (size_t i = 0; i < sv.count; i++) {
          if (sv.data[i] == c) {
               *idx = i;
               found = true;
          }
     }

     return found;
}

BASICDEF bool basic_sv_eq_cstr(Basic_String_View sv, const char *cstr)
{
     size_t cstr_count = strlen(cstr);
     if (sv.count != cstr_count) return false;
     return memcmp(sv.data, cstr, sv.count) == 0;
}

BASICDEF bool basic_sv_eq_sv(Basic_String_View a, Basic_String_View b)
{
     if (a.count != b.count) return false;
     return memcmp(a.data, b.data, a.count) == 0;
}

BASICDEF bool basic_sv_starts_with(Basic_String_View sv, const char *prefix)
{
     size_t prefix_count = strlen(prefix);
     if (sv.count < prefix_count) return false;
     Basic_String_View expected_prefix = basic_sv_from_parts(prefix, prefix_count);
     Basic_String_View actual_prefix = basic_sv_substr(sv, 0, prefix_count);
     return basic_sv_eq_sv(expected_prefix, actual_prefix);
}

BASICDEF bool basic_sv_ends_with(Basic_String_View sv, const char *suffix)
{
     size_t suffix_count = strlen(suffix);
     if (sv.count < suffix_count) return false;
     Basic_String_View expected_suffix = basic_sv_from_parts(suffix, suffix_count);
     Basic_String_View actual_suffix = basic_sv_substr(sv, sv.count - expected_suffix.count, expected_suffix.count);
     return basic_sv_eq_sv(expected_suffix, actual_suffix);
}

BASICDEF bool basic_load_file(const char *path, Basic_String_Builder *sb)
{
     bool result = true;

     FILE *file = fopen(path, "rb");
     if (!file) basic_return_defer(false);
     if (fseek(file, 0, SEEK_END) != 0) basic_return_defer(false);

#ifdef _WIN32
     long long file_count = _ftelli64(file);
#else
     long long file_count = ftello(file);
#endif
     if (file_count == -1L) basic_return_defer(false);
     if (fseek(file, 0, SEEK_SET) != 0) basic_return_defer(false);

     basic_da_reserve(sb, file_count);
     fread(sb->items, file_count, 1, file);
     if (ferror(file)) basic_return_defer(false);
     sb->count += file_count;

defer:
     if (file) fclose(file);
     return result;
}

#endif // BASIC_IMPLEMENTED
#endif // BASIC_IMPLEMENTATION

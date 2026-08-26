/*
 * sb - v2.1.0 (https://github.com/noadhum/sb)
 * Simple StringBuilder implementation in C
 */

#ifndef SB_H_
#define SB_H_

#include <stdbool.h>
#include <stddef.h>

typedef struct String_Builder String_Builder;

#ifndef SBDEF
#define SBDEF
#endif // SBDEF

#define SB SB_FROM_CSTR_LIT
#define SB_FROM_CSTR_LIT(cstr_lit) sb_from_parts(cstr_lit, sizeof(cstr_lit) - 1)

#ifndef SB_NO_ALIAS
SBDEF void sb_append(String_Builder *sb, const char *cstr);                     // Alias for `sb_append_cstr()`
SBDEF bool sb_eq(const String_Builder *a, const String_Builder *b);             // Alias for `sb_eq_sb()`
SBDEF bool sb_eq_ignorecase(const String_Builder *a, const String_Builder *b);  // Alias for `sb_eq_sb_ignorecase()`
#endif // SB_NO_ALIAS

SBDEF void sb_reserve(String_Builder *sb, size_t new_cap);
SBDEF void sb_free(String_Builder *sb);
SBDEF String_Builder sb_from_parts(const char *cstr, size_t count);
SBDEF String_Builder sb_from_cstr(const char *cstr);
SBDEF void sb_append_cstr(String_Builder *sb, const char *cstr);
SBDEF void sb_append_char(String_Builder *sb, char c);
SBDEF void sb_append_sb(String_Builder *a, const String_Builder *b);
SBDEF void sb_clear(String_Builder *sb);
SBDEF bool sb_empty(const String_Builder *sb);
SBDEF bool sb_eq_sb(const String_Builder *a, const String_Builder *b);
SBDEF bool sb_eq_sb_ignorecase(const String_Builder *a, const String_Builder *b);
SBDEF bool sb_eq_cstr(const String_Builder *sb, const char *cstr);
SBDEF bool sb_eq_cstr_ignorecase(const String_Builder *sb, const char *cstr);
SBDEF bool sb_starts_with(const String_Builder *sb, const char *prefix);
SBDEF bool sb_ends_with(const String_Builder *sb, const char *suffix);

#endif // SB_H_

#ifdef SB_IMPLEMENTATION

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#ifndef SB_NO_STDLIB
#include <stdlib.h>
#define SB_ABORT abort
#define SB_REALLOC realloc
#define SB_FREE free
#endif // SB_NO_STDLIB

#if !defined(SB_ABORT) || !defined(SB_REALLOC) || !defined(SB_FREE)
#error "SB_NO_STDLIB is defined, but SB_ABORT or SB_REALLOC or SB_FREE macro isn't"
#endif

#define SB_ASSERT(expr, message)                        \
     do {                                               \
          if (!(expr)) {                                \
               fprintf(stderr, "sb: %s\n", message);    \
               SB_ABORT();                              \
          }                                             \
     } while(0)

#ifndef SB_INIT_CAP
#define SB_INIT_CAP 64
#endif // SB_INIT_CAP

struct String_Builder {
     char *data;
     size_t count;
     size_t capacity;
};

#ifndef SB_NO_ALIAS
SBDEF void sb_append(String_Builder *sb, const char *cstr)
{
     sb_append_cstr(sb, cstr);
}

SBDEF bool sb_eq(const String_Builder *a, const String_Builder *b)
{
     return sb_eq_sb(a, b);
}

SBDEF bool sb_eq_ignorecase(const String_Builder *a, const String_Builder *b)
{
     return sb_eq_sb_ignorecase(a, b);
}
#endif // SB_NO_ALIAS

SBDEF void sb_reserve(String_Builder *sb, size_t new_cap)
{
     if (new_cap > sb->capacity) {
          if (sb->capacity == 0) sb->capacity = SB_INIT_CAP;
          while (new_cap > sb->capacity) {
               sb->capacity *= 2;
          }
          sb->data = SB_REALLOC(sb->data, sb->capacity*sizeof(char));
          SB_ASSERT(sb->data != NULL, "Unable to allocate memory");
     }
}

SBDEF void sb_free(String_Builder *sb)
{
     SB_FREE(sb->data);
     sb->data = NULL;
     sb->count = 0;
     sb->capacity = 0;
}

SBDEF String_Builder sb_from_parts(const char *cstr, size_t count)
{
     String_Builder sb = {0};
     if (count != 0) {
          sb_reserve(&sb, count + 1);
          memcpy(sb.data, cstr, count + 1);
          sb.count += count;
          sb.data[sb.count] = '\0';
     }

     return sb;
}

SBDEF String_Builder sb_from_cstr(const char *cstr)
{
     return sb_from_parts(cstr, strlen(cstr));
}

SBDEF void sb_append_cstr(String_Builder *sb, const char *cstr)
{
     size_t count = strlen(cstr);
     if (count == 0) return;

     sb_reserve(sb, sb->count + count + 1);
     memcpy(sb->data + sb->count, cstr, count);
     sb->count += count;
     sb->data[sb->count] = '\0';
}

SBDEF void sb_append_char(String_Builder *sb, char c)
{
     sb_reserve(sb, sb->count + 2);
     sb->data[sb->count++] = c;
     sb->data[sb->count] = '\0';
}

SBDEF void sb_append_sb(String_Builder *a, const String_Builder *b)
{
     if (b->data == NULL || sb_empty(b)) return;

     sb_reserve(a, a->count + b->count + 1);
     memcpy(a->data + a->count, b->data, b->count);
     a->count += b->count;
     a->data[a->count] = '\0';
}

SBDEF void sb_clear(String_Builder *sb)
{
     if (sb->data != NULL) {
          sb->data[0] = '\0';
     }

     sb->count = 0;
}

SBDEF bool sb_empty(const String_Builder *sb)
{
     return sb->count == 0;
}

SBDEF bool sb_eq_sb(const String_Builder *a, const String_Builder *b)
{
     if (a->count != b->count) {
          return false;
     }
     return memcmp(a->data, b->data, a->count) == 0;
}

SBDEF bool sb_eq_sb_ignorecase(const String_Builder *a, const String_Builder *b)
{
     if (a->count != b->count) {
          return false;
     }

     for (size_t i = 0; i < a->count; i++) {
          if (tolower((unsigned char)a->data[i]) != tolower((unsigned char)b->data[i])) {
               return false;
          }
     }

     return true;
}

SBDEF bool sb_eq_cstr(const String_Builder *sb, const char *cstr)
{
     size_t count = strlen(cstr);
     if (sb->count != count) {
          return false;
     }
     return memcmp(sb->data, cstr, count) == 0;
}

SBDEF bool sb_eq_cstr_ignorecase(const String_Builder *sb, const char *cstr)
{
     size_t count = strlen(cstr);
     if (sb->count != count) {
          return false;
     }

     for (size_t i = 0; i < sb->count; i++) {
          if (tolower((unsigned char)sb->data[i]) != tolower((unsigned char)cstr[i])) {
               return false;
          }
     }

     return true;
}

SBDEF bool sb_starts_with(const String_Builder *sb, const char *prefix)
{
     size_t count = strlen(prefix);
     if (sb->count < count) return false;
     return memcmp(sb->data, prefix, count) == 0;
}

SBDEF bool sb_ends_with(const String_Builder *sb, const char *suffix)
{
     size_t count = strlen(suffix);
     if (sb->count < count) return false;
     return memcmp(sb->data + (sb->count - count), suffix, count) == 0;
}
#endif // SB_IMPLEMENTATION

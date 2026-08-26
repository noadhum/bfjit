/*
 * sv - v1.0.0 (https://github.com/noadhum/sv)
 * Basic string_view implementation in C
 */

#ifndef SV_H_
#define SV_H_

typedef struct String_View String_View;

#ifndef SVDEF
#define SVDEF
#endif // SVDEF

#define SV SV_FROM_CSTR_LIT
#define SV_FROM_CSTR_LIT(cstr_lit) sv_from_parts(cstr_lit, sizeof(cstr_lit) - 1)

#ifndef SV_NO_ALIAS
SVDEF bool sv_eq(String_View a, String_View b);              // Alias for `sv_eq_sv()`
SVDEF bool sv_eq_ignorecase(String_View a, String_View b);   // Alias for `sv_eq_sv_ignorecase()`
#endif // SV_NO_ALIAS

SVDEF String_View sv_from_parts(const char *cstr, size_t count);
SVDEF String_View sv_from_cstr(const char *cstr);
SVDEF String_View sv_substr(String_View sv, size_t pos, size_t count);
SVDEF size_t sv_find(String_View sv, char c);
SVDEF size_t sv_rfind(String_View sv, char c);
SVDEF bool sv_empty(String_View sv);
SVDEF bool sv_eq_sv(String_View a, String_View b);
SVDEF bool sv_eq_sv_ignorecase(String_View a, String_View b);
SVDEF bool sv_starts_with(String_View sv, const char *prefix);
SVDEF bool sv_ends_with(String_View sv, const char *suffix);

#endif // SV_H_

#ifdef SV_IMPLEMENTATION

#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

struct String_View {
     const char *data;
     size_t count;
};

#ifndef SV_NO_ALIAS
SVDEF bool sv_eq(String_View a, String_View b)
{
     return sv_eq_sv(a, b);
}

SVDEF bool sv_eq_ignorecase(String_View a, String_View b)
{
     return sv_eq_sv_ignorecase(a, b);
}
#endif // SV_NO_ALIAS

SVDEF String_View sv_from_parts(const char *cstr, size_t count)
{
     return (String_View) {
          .data = cstr,
          .count = count
     };
}

SVDEF String_View sv_from_cstr(const char *cstr)
{
     return sv_from_parts(cstr, strlen(cstr));
}

SVDEF String_View sv_substr(String_View sv, size_t pos, size_t count)
{
     return sv_from_parts(&sv.data[pos], count);
}

SVDEF size_t sv_find(String_View sv, char c)
{
     for (size_t i = 0; i < sv.count; i++) {
          if (sv.data[i] == c) {
               return i;
          }
     }

     return SIZE_MAX;
}

SVDEF size_t sv_rfind(String_View sv, char c)
{
     size_t idx = SIZE_MAX;
     for (size_t i = 0; i < sv.count; i++) {
          if (sv.data[i] == c) {
               idx = i;
          }
     }

     return idx;
}

SVDEF bool sv_empty(String_View sv)
{
     return sv.count == 0;
}

SVDEF bool sv_eq_sv(String_View a, String_View b)
{
     if (a.count != b.count) {
          return false;
     }
     return memcmp(a.data, b.data, a.count) == 0;
}

SVDEF bool sv_eq_sv_ignorecase(String_View a, String_View b)
{
     if (a.count != b.count) {
          return false;
     }

     for (size_t i = 0; i < a.count; i++) {
          if (tolower((unsigned char)a.data[i]) != tolower((unsigned char)b.data[i])) {
               return false;
          }
     }

     return true;
}

SVDEF bool sv_starts_with(String_View sv, const char *prefix)
{
     size_t count = strlen(prefix);
     if (sv.count < count) return false;
     String_View expected_prefix = sv_from_parts(prefix, count);
     String_View actual_prefix = sv_substr(sv, 0, expected_prefix.count);
     return sv_eq_sv(expected_prefix, actual_prefix);
}

SVDEF bool sv_ends_with(String_View sv, const char *suffix)
{
     size_t count = strlen(suffix);
     if (sv.count < count) return false;
     String_View expected_suffix = sv_from_parts(suffix, count);
     String_View actual_suffix = sv_substr(sv, sv.count - expected_suffix.count, expected_suffix.count);
     return sv_eq_sv(expected_suffix, actual_suffix);
}

#endif // SV_IMPLEMENTATION

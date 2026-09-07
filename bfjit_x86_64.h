#ifndef X86_64_H_
#define X86_64_H_

#include <stdint.h>
#include "basic.h"

typedef enum {
     REG_RAX = 0,
     REG_RCX,
     REG_RDX,
     REG_RBX,
     REG_RSP,
     REG_RBP,
     REG_RSI,
     REG_RDI,
} x86_64_R64;

typedef enum {
     REG_AL = 0,
     REG_CL,
     REG_DL,
     REG_BL,
} x86_64_R8;

// https://en.wikipedia.org/wiki/ModR/M
typedef enum {
     MOD_MEM = 0,        // 00
     MOD_DISP8,          // 01
     MOD_DISP32,         // 10
     MOD_REG,            // 11
} ModRM_Mod;

uint8_t x86_64_generate_modrm(ModRM_Mod mod, x86_64_R64 reg, uint8_t reg_or_mem);

#define x86_64_emit_byte sb_append_char
#define x86_64_emit_byte_many da_append_many
void x86_64_emit_i32(String_Builder *sb, int32_t val);
void x86_64_emit_i64(String_Builder *sb, int64_t val);

void x86_64_emit_add_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b);            // add a, b
void x86_64_emit_add_val(String_Builder *sb, x86_64_R64 reg, int val);               // add reg, val
void x86_64_emit_add_byte_val(String_Builder *sb, x86_64_R64 reg, int val);          // add byte[reg], val

void x86_64_emit_sub_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b);            // sub a, b
void x86_64_emit_sub_val(String_Builder *sb, x86_64_R64 reg, int val);               // sub reg, val
void x86_64_emit_sub_byte_val(String_Builder *sb, x86_64_R64 reg, int val);          // sub byte[reg], val

void x86_64_emit_cmp_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b);            // cmp a, b
void x86_64_emit_cmp_val(String_Builder *sb, x86_64_R64 reg, int val);               // cmp reg, val
void x86_64_emit_cmp_byte_val(String_Builder *sb, x86_64_R64 reg, int val);          // cmp byte[reg], val

void x86_64_emit_mov_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b);            // mov a, b
void x86_64_emit_mov_val(String_Builder *sb, x86_64_R64 reg, int64_t val);           // mov reg, val
void x86_64_emit_mov_byte_val(String_Builder *sb, x86_64_R64 reg, int val);          // mov byte[reg], val
void x86_64_emit_mov_byte_r8(String_Builder *sb, x86_64_R64 r64, x86_64_R8 r8);      // mov byte[r64], r8

void x86_64_emit_movzx_r64_r8(String_Builder *sb, x86_64_R64 r64, x86_64_R8 r8);     // movzx r64, r8

void x86_64_emit_xor_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b);            // xor a, b

void x86_64_emit_push_r64(String_Builder *sb, x86_64_R64 reg);                       // push reg
void x86_64_emit_pop_r64(String_Builder *sb, x86_64_R64 reg);                        // pop reg

void x86_64_emit_syscall(String_Builder *sb);                                        // syscall

#endif // X86_64_H_

#ifdef X86_64_IMPLEMENTATION

void x86_64_emit_i32(String_Builder *sb, int32_t val)
{
     x86_64_emit_byte(sb, (uint8_t)(val >> 0));
     x86_64_emit_byte(sb, (uint8_t)(val >> 8));
     x86_64_emit_byte(sb, (uint8_t)(val >> 16));
     x86_64_emit_byte(sb, (uint8_t)(val >> 24));
}

void x86_64_emit_i64(String_Builder *sb, int64_t val)
{
     x86_64_emit_byte(sb, (uint8_t)(val >> 0));
     x86_64_emit_byte(sb, (uint8_t)(val >> 8));
     x86_64_emit_byte(sb, (uint8_t)(val >> 16));
     x86_64_emit_byte(sb, (uint8_t)(val >> 24));
     x86_64_emit_byte(sb, (uint8_t)(val >> 32));
     x86_64_emit_byte(sb, (uint8_t)(val >> 40));
     x86_64_emit_byte(sb, (uint8_t)(val >> 48));
     x86_64_emit_byte(sb, (uint8_t)(val >> 56));
}

uint8_t x86_64_generate_modrm(ModRM_Mod mod, x86_64_R64 reg, uint8_t reg_or_mem)
{
     uint8_t modrm = 0;
     modrm |= (mod << 6);
     modrm |= (reg_or_mem << 3);
     modrm |= (reg << 0);
     return modrm;
}

void x86_64_emit_add_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b)
{
     x86_64_emit_byte_many(sb, "\x48\x01", 2);
     uint8_t modrm = x86_64_generate_modrm(MOD_REG, a, b);
     x86_64_emit_byte(sb, modrm);
}

void x86_64_emit_add_val(String_Builder *sb, x86_64_R64 reg, int val)
{
     x86_64_emit_byte(sb, '\x48');

     if (val >= -128 && val <= 127) {
          x86_64_emit_byte(sb, '\x83');
          x86_64_emit_byte(sb, '\xC0' + reg);
          x86_64_emit_byte(sb, (uint8_t)val);
     } else {
          if (reg == REG_RAX) {
               x86_64_emit_byte(sb, '\x05');
          } else {
               x86_64_emit_byte(sb, '\x81');
               x86_64_emit_byte(sb, '\xC0' + reg);
          }

          x86_64_emit_i32(sb, val);
     }
}

void x86_64_emit_add_byte_val(String_Builder *sb, x86_64_R64 reg, int val)
{
     x86_64_emit_byte(sb, '\x80');

     if (reg == REG_RSP) {
          x86_64_emit_byte_many(sb, "\x04\x24", 2);
     } else if (reg == REG_RBP) {
          x86_64_emit_byte_many(sb, "\x45\x00", 2);
     } else {
          x86_64_emit_byte(sb, reg);
     }

     x86_64_emit_byte(sb, (uint8_t)val);
}

void x86_64_emit_sub_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b)
{
     x86_64_emit_byte_many(sb, "\x48\x29", 2);
     uint8_t modrm = x86_64_generate_modrm(MOD_REG, a, b);
     x86_64_emit_byte(sb, modrm);
}

void x86_64_emit_sub_val(String_Builder *sb, x86_64_R64 reg, int val)
{
     x86_64_emit_byte(sb, '\x48');

     if (val >= -128 && val <= 127) {
          x86_64_emit_byte(sb, '\x83');
          x86_64_emit_byte(sb, '\xE8' + reg);
          x86_64_emit_byte(sb, (uint8_t)val);
     } else {
          if (reg == REG_RAX) {
               x86_64_emit_byte(sb, '\x2D');
          } else {
               x86_64_emit_byte(sb, '\x81');
               x86_64_emit_byte(sb, '\xE8' + reg);
          }

          x86_64_emit_i32(sb, val);
     }
}

void x86_64_emit_sub_byte_val(String_Builder *sb, x86_64_R64 reg, int val)
{
     x86_64_emit_byte(sb, '\x80');

     if (reg == REG_RSP) {
          x86_64_emit_byte_many(sb, "\x2C\x24", 2);
     } else if (reg == REG_RBP) {
          x86_64_emit_byte_many(sb, "\x6D\x00", 2);
     } else {
          x86_64_emit_byte(sb, '\x28' + reg);
     }

     x86_64_emit_byte(sb, (uint8_t)val);
}

void x86_64_emit_cmp_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b)
{
     x86_64_emit_byte_many(sb, "\x48\x39", 2);
     uint8_t modrm = x86_64_generate_modrm(MOD_REG, a, b);
     x86_64_emit_byte(sb, modrm);

}

void x86_64_emit_cmp_val(String_Builder *sb, x86_64_R64 reg, int val)
{
     x86_64_emit_byte(sb, '\x48');

     if (val >= -128 && val <= 127) {
          x86_64_emit_byte(sb, '\x83');
          x86_64_emit_byte(sb, '\xF8' + reg);
          x86_64_emit_byte(sb, (uint8_t)val);
     } else {
          if (reg == REG_RAX) {
               x86_64_emit_byte(sb, '\x3D');
          } else {
               x86_64_emit_byte(sb, '\x81');
               x86_64_emit_byte(sb, '\xF8' + reg);
          }

          x86_64_emit_i32(sb, val);
     }
}

void x86_64_emit_cmp_byte_val(String_Builder *sb, x86_64_R64 reg, int val)
{
     x86_64_emit_byte(sb, '\x80');

     if (reg == REG_RSP) {
          x86_64_emit_byte_many(sb, "\x3C\x24", 2);
     } else if (reg == REG_RBP) {
          x86_64_emit_byte_many(sb, "\x7D\x00", 2);
     } else {
          x86_64_emit_byte(sb, '\x38' + reg);
     }

     x86_64_emit_byte(sb, (uint8_t)val);
}

void x86_64_emit_mov_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b)
{
     x86_64_emit_byte_many(sb, "\x48\x89", 2);
     uint8_t modrm = x86_64_generate_modrm(MOD_REG, a, b);
     x86_64_emit_byte(sb, modrm);
}

void x86_64_emit_mov_val(String_Builder *sb, x86_64_R64 reg, int64_t val)
{
     if (val >= 0x00 && val <= 0xFFFFFFFF) {
          x86_64_emit_byte(sb, '\xB8' + reg);
          x86_64_emit_i32(sb, val);
     } else {
          x86_64_emit_byte(sb, '\x48');
          x86_64_emit_byte(sb, '\xB8' + reg);
          x86_64_emit_i64(sb, val);
     }
}

void x86_64_emit_mov_byte_val(String_Builder *sb, x86_64_R64 reg, int val)
{
     x86_64_emit_byte(sb, '\xC6');

     if (reg == REG_RSP) {
          x86_64_emit_byte_many(sb, "\x04\x24", 2);
     } else if (reg == REG_RBP) {
          x86_64_emit_byte_many(sb, "\x45\x00", 2);
     } else {
          x86_64_emit_byte(sb, reg);
     }

     x86_64_emit_byte(sb, (uint8_t)val);
}

void x86_64_emit_mov_byte_r8(String_Builder *sb, x86_64_R64 r64, x86_64_R8 r8)
{
     x86_64_emit_byte(sb, '\x88');
     uint8_t modrm = x86_64_generate_modrm(MOD_MEM, r64, r8);
     x86_64_emit_byte(sb, modrm);
}

void x86_64_emit_movzx_r64_r8(String_Builder *sb, x86_64_R64 r64, x86_64_R8 r8)
{
     x86_64_emit_byte_many(sb, "\x48\x0F\xB6", 3);
     uint8_t modrm = x86_64_generate_modrm(MOD_REG, r8, r64);
     x86_64_emit_byte(sb, modrm);
}

void x86_64_emit_xor_r64(String_Builder *sb, x86_64_R64 a, x86_64_R64 b)
{
     x86_64_emit_byte_many(sb, "\x48\x31", 2);
     uint8_t modrm = x86_64_generate_modrm(MOD_REG, a, b);
     x86_64_emit_byte(sb, modrm);
}

void x86_64_emit_push_r64(String_Builder *sb, x86_64_R64 reg)
{
     x86_64_emit_byte(sb, '\x50' + reg);
}

void x86_64_emit_pop_r64(String_Builder *sb, x86_64_R64 reg)
{
     x86_64_emit_byte(sb, '\x58' + reg);
}

void x86_64_emit_syscall(String_Builder *sb)
{
     x86_64_emit_byte_many(sb, "\x0F\x05", 2);
}

#endif // X86_64_IMPLEMENTATION

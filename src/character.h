#ifndef CHARACTER_H
#define CHARACTER_H 1

#include <stdio.h>
#include "common.h"

#define c_null ('\0')
#define is_char(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z'))
#define is_digit(c) ('0' <= (c) && (c) <= '9')
#define is_hex(c) (('0' <= (c) && (c) <= '9') || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define is_identc(c) (is_char(c) || is_digit(c) || (c == '_'))

int hex_to_int(char c);

#endif

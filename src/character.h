#ifndef CHARACTER_H
#define CHARACTER_H 1

#include "common.h"

#define is_char(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z'))
#define is_digit(c) ('0' <= (c) && (c) <= '9')
#define is_hex(c) (('0' <= (c) && (c) <= '9') || ('A' <= (c) && (c) <= 'F') || ('a' <= (c) && (c) <= 'f'))
#define is_identc(c) (is_char(c) || is_digit(c) || (c == '_'))

typedef enum {
    CC_ASCII, CC_NASCII, CC_UTF2, CC_UTF3, CC_UTF4, CC_UTF5, CC_UTF6, CC_UTF7, CC_UTF8, CC_UTF9
} CharClass;

extern uchar utf8_bound[10][9];

int hex_to_int(char c);
int read_utf8_char(char *c, ulli *result);
ulli string_to_ulli(char *c);
int utf8_strlen(char *c);
int parse_utf8(char *c, ulli *buffer);
CharClass det_cc(char *str);

#endif

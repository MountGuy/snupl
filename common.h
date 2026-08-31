#ifndef COMMON_H
#define COMMON_H

#define p_null NULL
#define c_null ('\0')
#define is_char(c) ('a' <= (c) && (c) <= 'z' || 'A' <= (c) && (c) <= 'Z')
#define is_digit(c) ('0' <= c && c <= '9')

typedef enum { T_IDENTITY, T_STRING, T_OPERATOR, T_END } TType;
typedef struct { char *string; } String;
typedef struct { char *token; TType ttype; } Token;

#endif
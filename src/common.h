#ifndef COMMON_H
#define COMMON_H 1

#define p_null NULL
#define c_null ('\0')
#define is_char(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z'))
#define is_digit(c) ('0' <= c && c <= '9')

typedef enum {B_FALSE, B_TRUE, B_VAR} Boolean;
typedef enum { T_IDENTITY, T_STRING, T_OPERATOR } TType;
typedef enum { E_ALTER, E_CONCAT, E_OPTION, E_REPEAT, E_STRING, E_IDENTITY, E_DEFINE } ExprKind;

typedef struct { char *string; TType ttype; } Token;

typedef struct Expr {
    ExprKind kind;
    union {
        struct { int expr_num; struct Expr **exprs; } nary;
        struct { int id; char *str; struct Expr *expr; } identity;
        struct { char *str; } string;
    };
} Expr;

typedef struct {
    int char_num, asset_num, tok_num;
    char *input, *asset, *top, **starts;
} Lexer;

typedef struct {
    Token *tokens;
    Expr *exprs, *defs, **buffer;
    int pos, tok_num, expr_num, def_num;
} Parser;

#endif
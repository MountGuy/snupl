#ifndef STRUCT_H
#define STRUCT_H 1

typedef enum { T_IDENTITY, T_STRING, T_OPERATOR, T_END } TType;
typedef enum { E_ALT, E_CON, E_OPT, E_REP, E_GRP, E_LETS, E_IDENT, E_TMP, E_END, E_DEF } ExprKind;

typedef struct { char *string; TType ttype; } Token;

typedef struct Expr {
    ExprKind kind;
    union {
        struct { struct Expr *l, *r; } binary;
        struct { struct Expr *expr; } unary;
        struct { char *string; } identity;
        struct { char *string; struct Expr *expr;} definition;
    };
} Expr;

typedef struct {
    int char_num, asset_num, tok_num;
    char *input, *asset, *top, **starts;
} Lexer;

typedef struct {
    Token *tokens;
    Expr *exprs;
    int pos, tok_num, expr_num;
} Parser;

#endif
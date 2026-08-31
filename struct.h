#ifndef STRUCT_H
#define STRUCT_H 1
typedef enum { T_IDENTITY, T_STRING, T_OPERATOR, T_END } TType;
typedef struct { char *token; TType ttype; } Token;


typedef enum { E_ALT, E_CON, E_OPT, E_REP, E_GRP, E_LETS, E_IDENT, E_TMP } ExprKind;

typedef struct Expr {
    ExprKind kind;
    union {
        struct { struct Expr *l_expr, *r_expr; } binary;
        struct { struct Expr *expr; } unary;
        struct { char *string; } identity;
        struct {} tmp;
    };
} Expr;

typedef struct {
    char *input;
    char *asset, *top, **starts;
    int char_num, asset_num, tok_num;
} Lexer;

typedef struct {
    Token *tokens;
    int pos;
    int tok_num;

    Expr arena[100];
    int arena_num;
} Parser;


#endif
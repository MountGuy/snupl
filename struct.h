#ifndef STRUCT_H
#define STRUCT_H 1
typedef enum { T_IDENTITY, T_STRING, T_OPERATOR, T_END } TType;
typedef struct { char *string; TType ttype; } Token;


typedef enum { E_ALT, E_CON, E_OPT, E_REP, E_GRP, E_LETS, E_IDENT, E_TMP, E_END, E_DEF } ExprKind;

typedef struct Expr {
    ExprKind kind;
    union {
        struct { struct Expr *l, *r; } binary;
        struct { struct Expr *expr; } unary;
        struct { char *string; } identity;
        struct { char *string; struct Expr *expr;} definition;
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

    Expr *exprs;
    int expr_num;
} Parser;


#endif
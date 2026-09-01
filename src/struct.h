#ifndef STRUCT_H
#define STRUCT_H 1

typedef enum { T_IDENTITY, T_STRING, T_OPERATOR, T_END } TType;
typedef enum { E_ALTER, E_CONCAT, E_OPTION, E_REPEAT, E_STRING, E_IDENTITY, E_DEFINE, E_END } ExprKind;

typedef struct { char *string; TType ttype; } Token;

typedef struct Expr {
    ExprKind kind;
    union {
        struct { int expr_num; struct Expr **exprs; } nary;
        struct { char *string; } identity;
        struct { char *string; struct Expr *expr; } definition;
    };
} Expr;


typedef struct {
    int char_num, asset_num, tok_num;
    char *input, *asset, *top, **starts;
} Lexer;

typedef struct {
    Token *tokens;
    Expr *exprs, **buffer;
    int pos, tok_num, expr_num;
} Parser;

#endif
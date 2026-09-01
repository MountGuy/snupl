#ifndef STRUCT_H
#define STRUCT_H 1

typedef enum { T_IDENTITY, T_STRING, T_OPERATOR, T_END } TType;
typedef enum { E_ALTER, E_CONCAT, E_OPT, E_REP, E_GRP, E_LETS, E_IDENT, E_TMP, E_END, E_DEF } ExprKind;

typedef struct { char *string; TType ttype; } Token;

typedef struct Expr {
    ExprKind kind;
    union {
        struct { struct Expr *l, *r; } binary;
        struct { struct Expr *expr; } unary;
        struct { char *string; } identity;
        struct { char *string; struct Expr *expr; } definition;
    };
} Expr;

typedef struct fExpr {
    ExprKind kind;
    union {
        struct { int expr_num; struct fExpr **exprs; } nary;
        struct { char *string; } identity;
        struct { char *string; struct fExpr *expr; } definition;
    };
} fExpr;


typedef struct {
    int char_num, asset_num, tok_num;
    char *input, *asset, *top, **starts;
} Lexer;

typedef struct {
    Token *tokens;
    Expr *exprs;
    int pos, tok_num, expr_num;
} Parser;

typedef struct {
    Token *tokens;
    fExpr *exprs;
    int pos, tok_num, expr_num;
} fParser;

#endif
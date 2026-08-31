#ifndef STRUCT_H
#define STRUCT_H 1
typedef enum { T_IDENTITY, T_STRING, T_OPERATOR, T_END } TType;
typedef struct { char *string; } String;
typedef struct { char *token; TType ttype; } Token;


typedef enum { E_ALT, E_CON, E_OPT, E_REP, E_GRP, E_LETS, E_IDENT, E_TMP } ExprKind;

typedef struct Expr {
    ExprKind kind;
    union {
        struct { struct Expr *l_expr, *r_expr; } alt;
        struct { struct Expr *l_expr, *r_expr; } con;
        struct { struct Expr *expr; } opt;
        struct { struct Expr *expr; } rep;
        struct { struct Expr *expr; } grp;
        struct { String letters; } lets;
        struct { String identity; } ident;
        struct {} tmp;
    };
} Expr;

typedef struct {
    Token *tokens;
    int pos;
    int tok_num;

    Expr arena[100];
    int arena_num;
} Parser;
#endif
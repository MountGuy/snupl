#ifndef COMMON_H
#define COMMON_H 1

#define p_null NULL
#define c_null ('\0')
#define is_char(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z'))
#define is_digit(c) ('0' <= c && c <= '9')

typedef enum { B_FALSE, B_TRUE, B_VAR } Boolean;
typedef enum { T_IDENTITY, T_STRING, T_OPERATOR } TType;
typedef enum { S_KEYWORD, S_STR } SType;
typedef enum { E_ALTER, E_CONCAT, E_OPTION, E_REPEAT, E_STRING, E_IDENTITY, E_DEFINE } ExprKind;
typedef enum { N_ALTER, N_CONCAT, N_REPEAT, N_PRIMARY } NFAKind;

#define C_EPS 0

typedef struct { char *string; TType ttype; } Token;

typedef struct Expr {
    ExprKind kind;
    union
    {
        struct { int expr_num; struct Expr **exprs; } nary;
        struct { int idx; char *str; struct Expr *expr; } identity;
        struct { char *str; } string;
    };
} Expr;

typedef struct {
    char *input;

    int char_num, asset_num;
    char *assets, *top, **starts;
    TType *asset_types;
    
    int tok_num;
    Token *tokens;
    
    int pos, expr_num, def_num;
    Expr *exprs, *defs;
} Parser;

typedef struct {
    int state_num, top_state, char_num;
    int ***trans;
} NFA_builder;

#endif
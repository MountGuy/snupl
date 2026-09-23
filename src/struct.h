#ifndef STRUCT_H
#define STRUCT_H 1

#include <stddef.h>

typedef unsigned long long int ulli;

typedef enum { M_IDENTITY, M_OPERATOR, M_STRING } MType;
typedef struct {
    char *string;
    MType type;
    int line, col, len;
} MetaToken;

typedef enum { E_ALTER, E_CONCAT, E_OPTION, E_REPEAT, E_STRING, E_CRANGE, E_IDENTITY } ExprKind;
typedef struct MetaExpr {
    ExprKind kind;
    union
    {
        struct { int expr_num; struct MetaExpr **exprs; } nary;
        struct { struct MetaExpr *expr; } unary;
        struct { char *value; } string;
        struct { int idx; char *id; } identity;
        struct { char lb, ub; } crange;
    };
    int idx;
} MetaExpr;

typedef struct {
    void *data;
    size_t unit;
    int max, used, expands;
} Chunk;

typedef struct {
    Chunk strings, string_heads, exprs, expr_lists;
    int string_num;
} Arena;

typedef enum { D_LETTER, D_TERM, D_GRAMMAR } DType;
typedef struct  {
    char *identity;
    DType type;
    MetaExpr *expr;
} MetaDef;

typedef struct {
    MetaToken *tokens;
    int token_num, cursor;

    Chunk defs;
    int def_num;

    Arena *arena;
} MetaParser;

typedef enum { T_EPS, T_VAR, T_CONST } TType;
typedef struct {
    char *name;
    int idx;
    TType type;
} TokenClass;

typedef struct {
    MetaDef *defs;
    TokenClass *tokcs;
    int def_num, tokc_num;
} Grammar;

typedef struct {
    TokenClass *tok_c;
    char *string;
    int string_len;
    int line, col;
} Token;

typedef struct {
    ulli *trans;
    int state_num, char_num, used_state_num;
    Chunk lubs;
    Grammar *grammar;
} NFABuilder;

typedef struct {
    ulli *trans;
    char *lbs, *ubs;
    TokenClass *tokcs;
    int state_num, char_num, exact_state_num;
    int *end_states, tokc_num;
} NFA;

typedef struct {
    int *lens, len;
    ulli *visiting, *tmp;
} NFAScanner;

typedef struct {
    ulli *sets;
    int set_num, set_size, offset, *can_eps;
    Chunk sup_sets, sub_sets;
} SetEquBuilder;

typedef struct {
    ulli *sets;
    int *sup_sets, *sub_sets;
    int equ_num, set_num, set_size, offset, exact_set_size;
} SetEqu;

typedef struct {
    Token *tokens;
    int token_num;

    int cursor;
} Parser;

#endif


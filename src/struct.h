#ifndef STRUCT_H
#define STRUCT_H 1

#include <stdbool.h>

typedef enum { M_IDENTITY, M_OPERATOR, M_STRING } MType;
typedef struct {
    char *string;
    MType type;
    int line, col;
} MetaToken;

typedef enum { E_ALTER, E_CONCAT, E_OPTION, E_REPEAT, E_STRING, E_CRANGE, E_IDENTITY, E_DEFINE } ExprKind;
typedef struct {
    ExprKind kind;
    union
    {
        struct { int expr_num; struct MetaExpr **exprs; } nary;
        struct { struct MetaExpr *expr; } unary;
        struct { char *value; } string;
        struct { char lb, ub; } crange;
    };
} MetaExpr;

typedef struct {
    char *buffer, **strings;
    int buffer_used, buffer_max, string_used, string_max;

    MetaExpr *exprs;
    int expr_used, expr_max;
} Arena;

typedef enum { D_LETTER, D_TERM, D_GRAMMAR } DType;
typedef struct  {
    char *identity;
    DType type;
    MetaExpr *expr;
} MetaDef;

typedef struct {
    char *input;
    int input_len, pos;

    MetaToken *tokens;
    int token_num;

    Arena *arena;
} MetaLexer;

typedef struct {
    MetaToken *tokens;
    int token_num;

    MetaDef *defs;
    int def_num;

    Arena *arena;
} MetaParser;

typedef struct {
    MetaDef *defs;
    int def_num;
} Grammar;

typedef enum { T_TERM, T_GRAMMAR } TType;
typedef struct {
    char *string;
    int string_len;

    TType type;
    int line, col;
} Token;

typedef struct {
    char *trans, *output;
    int start, end;
    int char_num, state_num, used_state_num, output_num;
} NFA;

typedef struct {
    char *input;
    int input_len, pos;

    Token *tokens;
    int token_num;

    Arena *arena;
} Lexer;

#endif


#ifndef STRUCT_H
#define STRUCT_H 1

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
} MetaExpr;

typedef struct {
    char *buffer, **strings;
    int buffer_used, buffer_max, string_used, string_max;

    MetaExpr *exprs, **expr_lists;
    int expr_used, expr_max, expr_list_used, expr_list_max;
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
    int token_num, cursor;

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
    char ***trans, **end_names;
    TType *end_types;
    int start, *ends, char_num, state_num, used_state_num, end_num;
    char *lbs, *ubs;
    Grammar *grammar;
} NFABuilder;

typedef struct {
    char ***trans, **end_names, *lbs, *ubs;
    TType *end_types;
    int start, *ends, char_num, state_num, end_num;
} NFA;

typedef struct {
    char *input;
    int input_len, pos;

    Token *tokens;
    int token_num;

    Arena *arena;
} Lexer;

#endif


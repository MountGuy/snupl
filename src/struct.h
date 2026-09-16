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
    void *buf;
    size_t unit;
    int length, used, expands;
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
    char *input;
    int input_len, pos;

    Chunk tokens;
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
    char *string, *name;
    int string_len;

    TType type;
    int line, col;
} Token;

typedef struct {
    char ***trans, **end_names, *lbs, *ubs;
    TType *end_types;
    int start, *ends, char_num, state_num, used_state_num, end_num;
    Grammar *grammar;
} NFABuilder;

typedef struct {
    char ***trans, **end_names, *lbs, *ubs;
    TType *end_types;
    int start, *ends, char_num, state_num, end_num;
} NFA;

typedef struct {
    int state_num, end_num, *visiting, *chars, *tmp, *lens, len;
    NFA *nfa;
} NFAScanner;

typedef struct {
    char *input, *line_start, *cursor;
    int input_len, line;

    Token *tokens;
    int token_num;

    NFA *nfa;
    Arena *arena;
} Lexer;

#endif


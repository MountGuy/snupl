#ifndef COMMON_H
#define COMMON_H 1

#define p_null NULL
#define c_null ('\0')
#define is_char(c) (('a' <= (c) && (c) <= 'z') || ('A' <= (c) && (c) <= 'Z'))
#define is_digit(c) ('0' <= c && c <= '9')
#define is_hex(c) (('0' <= c && c <= '9') || ('A' <= c && c <= 'F') || ('a' <= c && c <= 'f'))
#define flush (fflush(stdout))
#define newline printf("\n")
#define sepline printf("==============================================\n")

typedef enum { B_FALSE, B_TRUE, B_VAR } Boolean;
typedef enum { T_IDENTITY, T_STRING, T_OPERATOR } TType;
typedef enum { S_BASICS, S_GRAMMAR, S_IDENTITY} SType;
typedef enum { E_ALTER, E_CONCAT, E_OPTION, E_REPEAT, E_STRING, E_CRANGE, E_IDENTITY, E_DEFINE } ExprKind;
typedef enum { N_ALTER, N_CONCAT, N_REPEAT, N_PRIMARY } NFAKind;

#define C_EPS ('\0')
#define I_EPS 0
#define CMAP_SIZE 300

typedef struct { char *string; TType ttype; } GToken;

typedef struct {
    int asset_num;
    char *assets, *top, **starts;
    SType *asset_types;
} Asset;

typedef struct GExpr {
    ExprKind kind;
    union
    {
        struct { int expr_num; struct GExpr **exprs; } nary;
        struct { int idx; char *str; struct GExpr *expr; } identity;
        struct { char *str; } string;
        struct { char start, end; } crange;
    };
} GExpr;

typedef struct {
    char *input;
    int char_num;

    Asset *asset;
    
    int tok_num;
    GToken *tokens;
    
    int pos, expr_num, def_num;
    GExpr *exprs, *defs;
} GParser;

typedef struct {
    char *name, *l_chars, *r_chars;
    int char_num, state_num, used_state_num;
    int *trans;
    int start, end, *visiting, *visiting_new;
} NFA;

typedef struct {
    char *input;
    NFA *nfa;
    int nfa_num;

    char **strings;
    int string_num;
} Lexer;

#endif
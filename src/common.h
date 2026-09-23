#ifndef COMMON_H
#define COMMON_H 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define c_null ('\0')
#define p_null NULL
#define flush (fflush(stdout))
#define newline printf("\n")
#define sepline printf("==============================================\n")

typedef unsigned long long int ulli;
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

typedef enum { D_LETTER, D_TERM, D_GRAMMAR } DType;
typedef struct  {
    char *identity;
    DType type;
    MetaExpr *expr;
} MetaDef;

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

#endif

#ifndef EBNF_H
#define EBNF_H 1

#include "common.h"

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

int ebnf_lexer(char *buf, Token *tokens);
int ebnf_parser(int tok_num, Token *tokens);

Expr *parse_alt(Parser *parser);
Expr *parse_con(Parser *parser);
Expr *parse_prime(Parser *parser);

#define L_PAREN '('
#define L_BRACE '{'
#define L_BRAKET '['
#define R_PAREN ')'
#define R_BRACE '}'
#define R_BRAKET ']'

char groups[][3] = {{L_PAREN, R_PAREN, E_GRP}, {L_BRACE, R_BRACE, E_REP}, {L_BRAKET, R_BRAKET, E_OPT}};

char *ebnf_lparen = "(";
char *ebnf_rparen = ")";
char *ebnf_lbrace = "{";
char *ebnf_rbrace = "}";
char *ebnf_lbraket = "[";
char *ebnf_rbraket = "]";
char *ebnf_end = "\n";
char *ebnf_alt = "|";
char *ebnf_con = ",";


#endif
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

int compare_peek(Parser *parser, char *target);
int ebnf_parser(int tok_num, Token *tokens);

Token *peek_tok(Parser *parser);
Expr *enhance_arena(Parser *parser);
Expr *parse_alt(Parser *parser);
Expr *parse_con(Parser *parser);
Expr *parse_prime(Parser *parser);

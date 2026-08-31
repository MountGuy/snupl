#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "ebnf.h"

// #define debug
#define TTOK_C(t) ((t)->token[0])

char parens[][3] = { {'(', ')', E_GRP}, {'{', '}', E_REP}, {'[', ']', E_OPT}};

void enhance_parser(Parser *parser)
{
    #ifdef debug
    printf("consume %s\n", parser->tokens[parser->pos]);
    #endif
    parser->pos++;
}

Token *peek_tok(Parser *parser)
{
    return parser->tokens + parser->pos;
}

Expr *enhance_arena(Parser *parser)
{
    Expr *expr = parser->arena + parser->arena_num;
    parser->arena_num++;

    return expr;
}

int parser_end(Parser *parser)
{
    return parser->tokens[parser->pos].ttype == T_END;
}

void print_expr(Expr *expr)
{
    // #ifndef debug
    // return;
    // #endif
    ExprKind kind = expr->kind;

    switch (kind)
    {
        case E_ALT:
        {
            printf("<");
            print_expr(expr->alt.l_expr);
            printf(" | ");
            print_expr(expr->alt.r_expr);
            printf(">");
            break;
        }
        case E_CON:
        {
            printf("<");
            print_expr(expr->con.l_expr);
            printf(" , ");
            print_expr(expr->con.r_expr);
            printf(">");
            break;
        }
        case E_OPT:
        {
            printf("[");
            print_expr(expr->opt.expr);
            printf("]");
            break;
        }
        case E_REP:
        {
            printf("{");
            print_expr(expr->opt.expr);
            printf("}");
            break;
        }
        case E_GRP:
        {
            printf("(");
            print_expr(expr->opt.expr);
            printf(")");
            break;
        }
        case E_LETS:
        {
            printf("\"%s\"", expr->lets.letters.string);
            break;
        }
        case E_IDENT:
        {
            printf("%s", expr->ident.identity.string);
            break;
        }
        default:
        {
            printf("error on print expr\n");
            break;
        }

    }
}

void print_parser(Parser *parser)
{
    #ifndef debug
    return;
    #endif
    printf(">>>>>>>>>>>>>>>>>\n");
    for (int i = 0; i < parser->pos; i++)
    {
        printf("%d %s\n", parser->tokens[i].ttype, parser->tokens[i].token);
    }
    printf("\n");
    for (int i = parser->pos; i < parser->tok_num; i++)
    {
        printf("%d %s\n", parser->tokens[i].ttype, parser->tokens[i].token);
    }
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

int ebnf_parser(int tok_num, Token *tokens)
{
    printf("\ntokens: ");
    for (int i = 0; i < tok_num; i++)
    {
        printf("%s ", tokens[i].token);
    }
    printf("\n");
    Parser parser = {0};
    parser.tokens = tokens + 2;
    parser.tok_num = tok_num - 2;
    Expr *expr = parse_alt(&parser);
    print_expr(expr); printf("\n");
    return 0;

}

Expr *parse_alt(Parser *parser)
{
    #ifdef debug
    printf("parse alt\n");
    #endif
    Expr *expr1 = parse_con(parser);
    if (parser_end(parser)) return expr1;

    Token *tok = peek_tok(parser);
    if (tok->ttype == T_OPERATOR && TTOK_C(tok) == '|')
    {
        enhance_parser(parser);
        Expr *expr2 = parse_alt(parser);
        Expr *expr = enhance_arena(parser);
        expr->kind = E_ALT;
        expr->alt.l_expr = expr1;
        expr->alt.r_expr = expr2;
        #ifdef debug
        printf("return alt\n");
        #endif
        return expr;
    }
    else if (tok->ttype == T_OPERATOR && TTOK_C(tok) == ',')
    {
        enhance_parser(parser);
        Expr *expr2 = parse_con(parser);
        Expr *expr = enhance_arena(parser);
        expr->kind = E_CON;
        expr->con.l_expr = expr1;
        expr->con.r_expr = expr2;
        #ifdef debug
        printf("return alt\n");
        #endif
        return expr;
    }
    else if (TTOK_C(tok) == ')' || TTOK_C(tok) == '}' || TTOK_C(tok) == ']' || tok->ttype == T_END)
    {
        #ifdef debug
        printf("return alt\n");
        #endif
        return expr1;
    }
    else {
        printf("%d %c error on alt\n", tok->ttype, TTOK_C(tok));
    }
}

Expr *parse_con(Parser *parser)
{
    #ifdef debug
    printf("parse con\n");
    #endif
    Expr *expr1 = parse_prime(parser);
    Token *tok = peek_tok(parser);

    if (tok->ttype == T_OPERATOR && TTOK_C(tok) == ',')
    {
        enhance_parser(parser);
        Expr *expr2 = parse_con(parser);
        Expr *expr = enhance_arena(parser);
        expr->kind = E_CON;
        expr->con.l_expr = expr1;
        expr->con.r_expr = expr2;
        #ifdef debug
        printf("return con\n");
        #endif

        return expr;
    }
    else if (TTOK_C(tok) == ')' || TTOK_C(tok) == '}' || TTOK_C(tok) == ']' || TTOK_C(tok) == '|' || tok->ttype == T_END)
    {
        #ifdef debug
        printf("return con\n");
        #endif
        return expr1;
    }
    else printf("%d %c error on con\n", tok->ttype, TTOK_C(tok));
}

Expr *parse_prime(Parser *parser)
{
    #ifdef debug
    printf("parse prime\n");
    #endif
    Token *i_tok = peek_tok(parser);
    // print_parser(parser);

    for (int i = 0; i < sizeof(parens) / sizeof(parens[0]); i++)
    {
        if (i_tok->ttype == T_OPERATOR && TTOK_C(i_tok) == parens[i][0])
        {
            enhance_parser(parser);
            Expr *expr = parse_alt(parser);
            Token *f_tok = peek_tok(parser);
            if (f_tok->ttype == T_OPERATOR && TTOK_C(f_tok) == parens[i][1] || f_tok->ttype == T_END)
            {
                enhance_parser(parser);
                Expr *expr_new = enhance_arena(parser);
                expr_new->kind = parens[i][2];
                expr_new->grp.expr = expr;
                #ifdef debug
                printf("return prime\n");
                #endif
                return expr_new;
            }
            else
            {
                printf("%d %s %c %c\n", f_tok->ttype, f_tok->token, parens[i][0], parens[i][1]);
                printf("return prime with null\n");
                return p_null;
            }
        }
        if (i_tok->ttype == T_STRING)
        {
            enhance_parser(parser);
            Expr *expr = enhance_arena(parser);
            expr->kind = E_LETS;
            strcpy(expr->lets.letters.string, i_tok->token);
            #ifdef debug
            printf("return prime\n");
            #endif
            return expr;
        }
        if (i_tok->ttype == T_IDENTITY)
        {
            enhance_parser(parser);
            Expr *expr = enhance_arena(parser);
            expr->kind = E_IDENT;
            strcpy(expr->ident.identity.string, i_tok->token);
            #ifdef debug
            printf("return prime\n");
            #endif
            return expr;
        }
    }
    printf("%d %c error on prime\n", i_tok->ttype, TTOK_C(i_tok));
    exit(1);
}
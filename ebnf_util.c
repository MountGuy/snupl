#include <stdio.h>
#include <stdlib.h>

#include "ebnf_util.h"

char groups[N_GROUPS][3] = {
    {C_LPAREN, C_RPAREN, E_GRP},
    {C_LBRACE, C_RBRACE, E_REP},
    {C_LBRAKET, C_RBRAKET, E_OPT}
};
char *S_LPAREN = "(",
     *S_RPAREN = ")",
     *S_LBRACE = "{",
     *S_RBRACE = "}",
     *S_LBRAKET = "[",
     *S_RBRAKET = "]",
     *S_END = ";\n",
     *S_DEFINE = "=",
     *S_ALTER = "|",
     *S_CONCAT = ",";


void print_tokens(int tok_n, Token *tokens)
{
    for (int i = 0; i < tok_n; i++)
    {
        printf("%p %s\n", tokens[i].string, tokens[i].string);
    }
}

void advance_parser(Parser *parser)
{
    printf("(%d/%d) consume %s\n", parser->pos, parser->tok_num, parser->tokens[parser->pos].string);
    parser->pos++;
}

Token *peek_tok(Parser *parser)
{
    return parser->tokens + parser->pos;
}

Expr *alloc_expr(Parser *parser)
{
    Expr *expr = parser->exprs + parser->expr_num;
    parser->expr_num++;
    printf("expr num: %d\n", parser->expr_num);

    return expr;
}

int parser_end(Parser *parser)
{
    return parser->tokens[parser->pos].ttype == T_END;
}

void print_expr(Expr *expr)
{
    ExprKind kind = expr->kind;

    switch (kind)
    {
        case E_ALT:
        {
            printf("<");
            print_expr(expr->binary.l);
            printf(" | ");
            print_expr(expr->binary.r);
            printf(">");
            break;
        }
        case E_CON:
        {
            printf("<");
            print_expr(expr->binary.l);
            printf(" , ");
            print_expr(expr->binary.r);
            printf(">");
            break;
        }
        case E_OPT:
        {
            printf("[");
            print_expr(expr->unary.expr);
            printf("]");
            break;
        }
        case E_REP:
        {
            printf("{");
            print_expr(expr->unary.expr);
            printf("}");
            break;
        }
        case E_GRP:
        {
            printf("(");
            print_expr(expr->unary.expr);
            printf(")");
            break;
        }
        case E_LETS:
        {
            printf("\"%s\"", expr->identity.string);
            break;
        }
        case E_IDENT:
        {
            printf("%s", expr->identity.string);
            break;
        }
        case E_END:
        {
            printf("\n");
            break;
        }
        case E_DEF:
        {
            printf("%s := ", expr->definition.string);
            print_expr(expr->definition.expr);
            printf("\n");
            break;
        }
        default:
        {
            printf("error on print expr: %d\n", kind);
            exit(1);
            break;
        }
    }
}

void print_parser(Parser *parser)
{
    #ifndef debug
    return;
    #endif
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>\n");
    for (int i = 0; i < parser->pos; i++)
    {
        printf("%d %s\n", parser->tokens[i].ttype, parser->tokens[i].string);
    }
    printf("\n");
    for (int i = parser->pos; i < parser->tok_num; i++)
    {
        printf("%d %s\n", parser->tokens[i].ttype, parser->tokens[i].string);
    }
    printf("<<<<<<<<<<<<<<<<<<<<<<<<<<\n");
}

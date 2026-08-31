#include <stdio.h>

#include "ebnf_util.h"

char groups[N_GROUPS][3] = {{L_PAREN, R_PAREN, E_GRP}, {L_BRACE, R_BRACE, E_REP}, {L_BRAKET, R_BRAKET, E_OPT}};
char *ebnf_lparen = "(",
     *ebnf_rparen = ")",
     *ebnf_lbrace = "{",
     *ebnf_rbrace = "}",
     *ebnf_lbraket = "[",
     *ebnf_rbraket = "]",
     *ebnf_end = ";\n",
     *ebnf_def = "=",
     *ebnf_alt = "|",
     *ebnf_con = ",";


void print_tokens(int tok_n, Token *tokens)
{
    for (int i = 0; i < tok_n; i++)
    {
        printf(" %s", tokens[i].token);
    }
}

void advance_parser(Parser *parser)
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

Expr *alloc_arena(Parser *parser)
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
    ExprKind kind = expr->kind;

    switch (kind)
    {
        case E_ALT:
        {
            printf("<");
            print_expr(expr->binary.l_expr);
            printf(" | ");
            print_expr(expr->binary.r_expr);
            printf(">");
            break;
        }
        case E_CON:
        {
            printf("<");
            print_expr(expr->binary.l_expr);
            printf(" , ");
            print_expr(expr->binary.r_expr);
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
    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>\n");
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

#include "ebnf_util.h"

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

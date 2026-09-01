#include <stdio.h>
#include <stdlib.h>

#include "ebnf_util.h"

char groups[N_GROUPS][3] = {
    {C_LPAREN, C_RPAREN, E_GRP},
    {C_LBRACE, C_RBRACE, E_REP},
    {C_LBRAKET, C_RBRAKET, E_OPT}
};
char *S_LPAREN = "(", *S_RPAREN = ")",
     *S_LBRACE = "{", *S_RBRACE = "}",
     *S_LBRAKET = "[", *S_RBRAKET = "]",
     *S_END = ";", *S_DEFINE = "=",
     *S_ALTER = "|", *S_CONCAT = ",";


void print_tokens(int tok_num, Token *tokens)
{
    for (int i = 0; i < tok_num; i++)
    {
        // printf("%p %s\n", tokens[i].string, tokens[i].string);
    }
}

void print_fexpr(fExpr *expr)
{
    ExprKind kind = expr->kind;

    switch (kind)
    {
        case E_ALTER:
        {
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_fexpr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" | ");
            }
            printf(")");
            break;
        }
        case E_CONCAT:
        {
            printf("(");
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                print_fexpr(expr->nary.exprs[i]);
                if (i < expr->nary.expr_num - 1)
                    printf(" , ");
            }
            printf(")");
            break;
        }
        case E_OPT:
        {
            printf("[");
            print_fexpr(expr->nary.exprs[0]);
            printf("]");
            break;
        }
        case E_REP:
        {
            printf("{");
            print_fexpr(expr->nary.exprs[0]);
            printf("}");
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
            print_fexpr(expr->definition.expr);
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
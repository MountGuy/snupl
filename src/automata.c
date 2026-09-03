#include <stdlib.h>
#include <stdio.h>

#include "common.h"
#include "automata.h"

void lower_parser(Parser *parser)
{
    int def_num = parser->def_num;

    for (int i = 0; i < def_num; i++)
    {
        if (parser->defs[i].kind != E_DEFINE)
        {
            printf("wtf?\n");
            exit(1);
        }
        char *str = parser->defs[i].identity.str;

        if (str[0] != '_')
            continue;

        Expr *expr = parser->defs[i].identity.expr;
        expr = lower_expr(expr, parser);
        parser->defs[i].identity.expr = expr;
    }
}

Expr *lower_expr(Expr *expr, Parser *parser)
{
    switch (expr->kind)
    {
        case E_IDENTITY:
        {
            int idx = expr->identity.idx;
            Expr *_expr = parser->defs[idx].identity.expr;
            Expr *lowered = lower_expr(_expr, parser);
            return lowered;
        }
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                Expr *_expr = expr->nary.exprs[i];
                Expr *lowered = lower_expr(_expr, parser);
                expr->nary.exprs[i] = lowered;
            }
            return expr;
        }
        default:
            return expr;
    }
}
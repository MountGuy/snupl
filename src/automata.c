#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "automata.h"
#include "ebnf_util.h"

void resolve_parser(Parser *parser)
{
    int def_num = parser->def_num;
    for (int i = 0; i < def_num; i++)
    {
        if (parser->defs[i].kind != E_DEFINE)
        {
            exit(1);
        }
        char *str = parser->defs[i].identity.str;

        if (str[0] != '_')
            continue;

        resolve_expr(parser->defs[i].identity.expr, parser);
        unroll_expr(parser->defs[i].identity.expr, parser);
    }
    print_parser(parser);
}

void resolve_expr(Expr *expr, Parser *parser)
{
    switch (expr->kind)
    {
        case E_IDENTITY:
        {
            int idx = expr->identity.idx;
            Expr *def_body = parser->defs[idx].identity.expr;
            *expr = *def_body;
            if (expr->kind != E_STRING)
            {
                expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr->nary.expr_num);
                memcpy(expr->nary.exprs, def_body->nary.exprs, sizeof(Expr*) * expr->nary.expr_num);
            }
            resolve_expr(expr, parser);
            return;
        }
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
        {
            for (int i = 0; i < expr->nary.expr_num; i++)
                resolve_expr(expr->nary.exprs[i], parser);
            return;
        }
        case E_STRING:
            return;
        default:
            printf("wtf? %d\n", expr->kind);
            return;
    }
}

void unroll_expr(Expr *expr, Parser *parser)
{
    ExprKind kind = expr->kind;
    switch (kind)
    {
        case E_STRING:
            break;
        case E_REPEAT:
        case E_OPTION:
        {
            unroll_expr(expr->nary.exprs[0], parser);
            break;
        }
        case E_ALTER:
        case E_CONCAT:
        {
            Expr **buffer = (Expr**) malloc(sizeof(Expr*) * parser->expr_num);
            int expr_num = 0;

            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                Expr *tmp = expr->nary.exprs[i];
                unroll_expr(tmp, parser);
                if (tmp->kind == kind)
                {
                    memcpy(buffer + expr_num, tmp->nary.exprs, sizeof(Expr*) * tmp->nary.expr_num);
                    expr_num += tmp->nary.expr_num;
                }
                else
                {
                    buffer[expr_num] = tmp;
                    expr_num++;
                }
            }

            Expr **old = expr->nary.exprs;
            set_nary_expr(expr, kind, buffer, expr_num);
            free(old);
            free(buffer);
            
            break;
        }
        default:
            exit(1);
    }
}

void build_nfa(Expr *expr)
{

}
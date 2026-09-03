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

        Expr *expr = parser->defs[i].identity.expr;
        resolve_expr(&(parser->defs[i].identity.expr), parser);
        expr = unroll_expr(expr, parser);
        parser->defs[i].identity.expr = expr;
    }
    print_parser(parser);
}

void resolve_expr(Expr **expr, Parser *parser)
{
    switch ((*expr)->kind)
    {
        case E_IDENTITY:
        {
            int idx = (*expr)->identity.idx;
            *expr = parser->defs[idx].identity.expr;
            resolve_expr(expr, parser);
            return;
        }
        case E_ALTER:
        case E_CONCAT:
        case E_OPTION:
        case E_REPEAT:
        {
            for (int i = 0; i < (*expr)->nary.expr_num; i++)
                resolve_expr((*expr)->nary.exprs + i, parser);
            return;
        }
        default:
            return;
    }
}

Expr *unroll_expr(Expr *expr, Parser *parser)
{
    switch (expr->kind)
    {
        case E_STRING:
            return expr;
        case E_REPEAT:
        case E_OPTION:
        {
            Expr *body = expr->nary.exprs[0];
            body = unroll_expr(body, parser);
            expr->nary.exprs[0] = body;
            return expr;
        }
        case E_CONCAT:
        {
            Expr **buffer = (Expr**) malloc(sizeof(Expr*) * parser->expr_num);
            int expr_num = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                Expr *tmp = expr->nary.exprs[i];
                tmp = unroll_expr(tmp, parser);
                if (tmp->kind == E_CONCAT)
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
            free(expr->nary.exprs);
            expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr_num);
            memcpy(expr->nary.exprs, buffer, sizeof(Expr*) * expr_num);
            free(buffer);
            expr->nary.expr_num = expr_num;
            return expr;
        }
        case E_ALTER:
        {
            Expr **buffer = (Expr**) malloc(sizeof(Expr*) * parser->expr_num);
            int expr_num = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                Expr *tmp = expr->nary.exprs[i];
                tmp = unroll_expr(tmp, parser);
                if (tmp->kind == E_ALTER)
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
            free(expr->nary.exprs);
            expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr_num);
            memcpy(expr->nary.exprs, buffer, sizeof(Expr*) * expr_num);
            free(buffer);
            expr->nary.expr_num = expr_num;
            return expr;
        }
        default:
            exit(1);
    }
}

void build_nfa(Expr *expr)
{

}
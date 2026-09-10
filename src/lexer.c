#include "common.h"
#include "struct.h"
#include "lexer.h"
#include "dump.h"

#define C_EPS ('\0')
#define I_EPS 0

int alloc_NFA_state(NFABuilder *builder)
{
    return builder->used_state_num++;
}

int find_char(char lb, char ub, NFABuilder *builder)
{
    for (int i = 0; i < builder->char_num; i++)
        if (builder->lbs[i] == lb && builder->ubs[i] == ub) 
            return i;
    builder->lbs[builder->char_num] = lb;
    builder->ubs[builder->char_num] = ub;

    return builder->char_num++;
}

void count_state(MetaExpr *expr, NFABuilder *builder)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                count_state(expr->nary.exprs[i], builder);
            builder->state_num += expr->nary.expr_num + (expr->kind == E_ALTER? 1: 0);
            break;
        case E_OPTION:
        case E_REPEAT:
            count_state(expr->unary.expr, builder);
            builder->state_num += 2;
            break;
        case E_STRING:
            builder->state_num += strlen(expr->string.value);
            break;
        case E_CRANGE:
            find_char(expr->crange.lb, expr->crange.ub, builder);
            builder->state_num += 1;
            break;
        case E_IDENTITY:
            MetaExpr *body = builder->grammar->defs[expr->identity.idx].expr;
            count_state(body, builder);
            break;
        default:
            printf("Unexpected expr kind during count_state\n");
            exit(1);
    }
}

void regist_char(MetaExpr *expr, NFABuilder *builder)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                regist_char(expr->nary.exprs[i], builder);
            break;
        case E_OPTION:
        case E_REPEAT:
            regist_char(expr->unary.expr, builder);
            break;
        case E_STRING:
            for (char *c = expr->string.value; *c; c++)
                find_char(*c, *c, builder);
            break;
        case E_CRANGE:
            find_char(expr->crange.lb, expr->crange.ub, builder);
            break;
        case E_IDENTITY:
            break;
        default:
            printf("Unexpected expr kind during regist_char\n");
            exit(1);
    }
}

int _build_NFA(MetaExpr *expr, int start, NFABuilder *builder)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int end = alloc_NFA_state(builder);
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                int _start = alloc_NFA_state(builder);
                int _end = _build_NFA(expr->nary.exprs[i], _start, builder);
                builder->trans[start][_start][I_EPS] = true;
                builder->trans[_end][end][I_EPS] = true;
            }
            return end;
        }
        case E_CONCAT:
        {
            int cur_start = start, cur_end;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                cur_end = _build_NFA(expr->nary.exprs[i], cur_start, builder);
                cur_start = alloc_NFA_state(builder);
                builder->trans[cur_end][cur_start][I_EPS] = true;
            }
            return cur_start;
        }
        case E_OPTION:
        {
            int end = alloc_NFA_state(builder);

            int body_start = alloc_NFA_state(builder);
            int body_end = _build_NFA(expr->unary.expr, body_start, builder);
            
            builder->trans[start][body_start][I_EPS] = true;
            builder->trans[body_end][end][I_EPS] = true;
            builder->trans[start][end][I_EPS] = true;

            return end;
        }
        case E_REPEAT:
        {
            int end = alloc_NFA_state(builder);

            int body_start = alloc_NFA_state(builder);
            int body_end = _build_NFA(expr->unary.expr, body_start, builder);
            
            builder->trans[start][body_start][I_EPS] = true;
            builder->trans[body_end][body_start][I_EPS] = true;
            builder->trans[body_end][end][I_EPS] = true;
            builder->trans[start][end][I_EPS] = true;

            return end;
        }
        case E_STRING:
        {
            int cur_start = start, cur_end;
            for (char *c = expr->string.value; *c; c++)
            {
                cur_end = alloc_NFA_state(builder);
                int cdx = find_char(*c, *c, builder);
                builder->trans[cur_start][cur_end][cdx] = true;
                cur_start = cur_end;
            }
            return cur_start;
        }
        case E_CRANGE:
        {
            char lb = expr->crange.lb, ub = expr->crange.ub;
            int idx = find_char(lb, ub, builder);
            int end = alloc_NFA_state(builder);
            builder->trans[start][end][idx] = true;
            return end;
        }
        case E_IDENTITY:
        {
            int idx = expr->identity.idx;
            MetaExpr *body = builder->grammar->defs[idx].expr;
            return _build_NFA(body, start, builder);
        }
        default:
            printf("Unexpected meta expression type during building NFA: %d\n", expr->kind);
            exit(1);
    }
}

int _build_NFA_string(MetaExpr *expr, int start, NFABuilder *builder)
{

}

void build_NFA(Grammar *grammar)
{
    NFABuilder builder;
    builder.grammar = grammar;
    builder.lbs = (char*) malloc(sizeof(char) * 1000);
    builder.ubs = (char*) malloc(sizeof(char) * 1000);
    builder.lbs[0] = C_EPS;
    builder.ubs[0] = C_EPS;
    builder.char_num = 1;
    builder.state_num = 1;
    builder.used_state_num = 0;

    for (int i = 0; i < grammar->def_num; i++)
    {
        if (grammar->defs[i].type == D_TERM)
        {
            builder.state_num++;
            count_state(grammar->defs[i].expr, &builder);
        }
        regist_char(grammar->defs[i].expr, &builder);
    }

    int s = builder.state_num, c = builder.char_num;
    int cs = (int) sizeof(char), ps = (int) sizeof(char*);

    void *buffer = (void*) malloc(ps * s + ps * s * s + cs * s * s * c);
    builder.trans = (char***) buffer;

    for (int i = 0; i < s; i++)
    {
        builder.trans[i] = (char**) (buffer + ps * s + ps * i * s);
        for (int j = 0; j < s; j++)
            builder.trans[i][j] = (char*) (buffer + ps * s + ps * s * s + cs * (i * s * c + j * c));
    }

    for (int i = 0; i < grammar->def_num; i++)
        if (grammar->defs[i].type == D_TERM)
        {
            int start = alloc_NFA_state(&builder);
            printf("starting with %d\n", start);
            _build_NFA(grammar->defs[i].expr, start, &builder);
        }
}



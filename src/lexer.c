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

void add_ending(char *name, int end, NFABuilder *builder)
{
    for (int i = 0; i < builder->end_num; i++)
    {
        if (name == builder->end_names[i] && end == builder->ends[i])
            return;
    }
    builder->end_names[builder->end_num] = name;
    builder->ends[builder->end_num++] = end;
}

void prescan(MetaExpr *expr, DType type, NFABuilder *builder)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            if (type == D_TERM)
                builder->state_num += expr->nary.expr_num + (expr->kind == E_ALTER? 1: 0);
            for (int i = 0; i < expr->nary.expr_num; i++)
                prescan(expr->nary.exprs[i], type, builder);
            break;
        case E_OPTION:
        case E_REPEAT:
            if (type == D_TERM)
                builder->state_num += 2;
            prescan(expr->unary.expr, type, builder);
            break;
        case E_STRING:
            for (char *c = expr->string.value; *c; c++)
                find_char(*c, *c, builder);
            builder->state_num += strlen(expr->string.value) + 1;
            break;
        case E_CRANGE:
            find_char(expr->crange.lb, expr->crange.ub, builder);
            if (type == D_TERM)
                builder->state_num += 1;
            break;
        case E_IDENTITY:
            if (type == D_LETTER || type == D_TERM)
            {
                MetaExpr *body = builder->grammar->defs[expr->identity.idx].expr;
                prescan(body, type, builder);
            }
            break;
        default:
            printf("Unexpected expr kind during prescan\n");
            exit(1);
    }
}

int _build_NFA(MetaExpr *expr, int start, DType type, NFABuilder *builder)
{
    if (type == D_TERM)
    {
        switch (expr->kind)
        {
            case E_ALTER:
            {
                int end = alloc_NFA_state(builder);
                for (int i = 0; i < expr->nary.expr_num; i++)
                {
                    int _start = alloc_NFA_state(builder);
                    int _end = _build_NFA(expr->nary.exprs[i], _start, type, builder);
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
                    cur_end = _build_NFA(expr->nary.exprs[i], cur_start, type, builder);
                    cur_start = alloc_NFA_state(builder);
                    builder->trans[cur_end][cur_start][I_EPS] = true;
                }
                return cur_start;
            }
            case E_OPTION:
            {
                int end = alloc_NFA_state(builder);

                int body_start = alloc_NFA_state(builder);
                int body_end = _build_NFA(expr->unary.expr, body_start, type, builder);
                
                builder->trans[start][body_start][I_EPS] = true;
                builder->trans[body_end][end][I_EPS] = true;
                builder->trans[start][end][I_EPS] = true;

                return end;
            }
            case E_REPEAT:
            {
                int end = alloc_NFA_state(builder);

                int body_start = alloc_NFA_state(builder);
                int body_end = _build_NFA(expr->unary.expr, body_start, type, builder);
                
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
                return _build_NFA(body, start, type, builder);
            }
            default:
                printf("Unexpected meta expression type during building NFA: %d\n", expr->kind);
                exit(1);
        }
    }

    else if (type == D_GRAMMAR)
    {
        switch (expr->kind)
        {
            case E_ALTER:
            case E_CONCAT:
            {
                for (int i = 0; i < expr->nary.expr_num; i++)
                    _build_NFA(expr->nary.exprs[i], start, type, builder);
                return -1;
            }
            case E_OPTION:
            case E_REPEAT:
            {
                _build_NFA(expr->unary.expr, start, type, builder);
                return -1;
            }
            case E_STRING:
            {
                int cur_start = alloc_NFA_state(builder), cur_end;
                builder->trans[builder->start][cur_start][I_EPS] = true;
                for (char *c = expr->string.value; *c; c++)
                {
                    cur_end = alloc_NFA_state(builder);
                    int cdx = find_char(*c, *c, builder);
                    builder->trans[cur_start][cur_end][cdx] = true;
                    cur_start = cur_end;
                }
                add_ending(expr->string.value, cur_start, builder);
                return -1;
            }
            case E_CRANGE:
            case E_IDENTITY:
                return -1;
        }
    }
    printf("wtf\n");
}

void build_trans(NFABuilder *builder)
{
    int s = builder->state_num, c = builder->char_num;
    int cs = (int) sizeof(char), ps = (int) sizeof(char*);

    void *buffer = (void*) malloc(ps * s + ps * s * s + cs * s * s * c);
    builder->trans = (char***) buffer;

    for (int i = 0; i < s; i++)
    {
        builder->trans[i] = (char**) (buffer + ps * s + ps * i * s);
        for (int j = 0; j < s; j++)
            builder->trans[i][j] = (char*) (buffer + ps * s + ps * s * s + cs * (i * s * c + j * c));
    }
}

void build_NFA(Grammar *grammar, NFA *nfa)
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
    builder.end_names = (char**) malloc(sizeof(char*) * 1000);
    builder.end_num = 0;
    builder.ends = (int*) malloc(sizeof(int) * 1000);

    builder.start = alloc_NFA_state(&builder);

    for (int i = 0; i < grammar->def_num; i++)
    {
        MetaDef def = grammar->defs[i];
        prescan(def.expr, def.type, &builder);
    }

    build_trans(&builder);

    for (int i = 0; i < grammar->def_num; i++)
    {
        MetaDef def = grammar->defs[i];
        if (def.type == D_TERM)
        {
            int _start = alloc_NFA_state(&builder);
            builder.trans[builder.start][_start][I_EPS] = true;
            int _end = _build_NFA(def.expr, _start, D_TERM, &builder);
            add_ending(def.identity, _end, &builder);
        }
        else if (def.type == D_GRAMMAR)
        {
            _build_NFA(def.expr, -1, D_GRAMMAR, &builder);
        }
    }

    nfa->trans = builder.trans;
    nfa->end_names = builder.end_names;
    nfa->lbs = builder.lbs;
    nfa->ubs = builder.ubs;
    nfa->end_types = builder.end_types;
    nfa->start = builder.start;
    nfa->ends = builder.ends;
    nfa->char_num = builder.char_num;
    nfa->state_num = builder.state_num;
    nfa->end_num = builder.end_num;
}



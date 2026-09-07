#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "lexer.h"
#include "lexer_util.h"


int _build_NFA(GExpr *expr, int start, NFA *nfa, Lexer *lexer)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int end = alloc_NFA_state(nfa);
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                int _start = alloc_NFA_state(nfa);
                int _end = _build_NFA(expr->nary.exprs[i], _start, nfa, lexer);
                NFA_TRANS(start, _start, I_EPS, nfa) = B_TRUE;
                NFA_TRANS(_end, end, I_EPS, nfa) = B_TRUE;
            }
            return end;
        }
        case E_CONCAT:
        {
            int prev_end, prev_start = start;
            for (int i = 0; i < expr->nary.expr_num; i++)
            {
                prev_end = _build_NFA(expr->nary.exprs[i], prev_start, nfa, lexer);
                prev_start = alloc_NFA_state(nfa);
                NFA_TRANS(prev_end, prev_start, I_EPS, nfa) = B_TRUE;
            }
            return prev_end;
        }
        case E_OPTION:
        {
            int body_start = alloc_NFA_state(nfa);
            int body_end = _build_NFA(expr->nary.exprs[0], body_start, nfa, lexer);
            int end = alloc_NFA_state(nfa);
            NFA_TRANS(start, body_start, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(body_end, end, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(start, end, I_EPS, nfa) = B_TRUE;
            return end;
        }
        case E_REPEAT:
        {
            int body_start = alloc_NFA_state(nfa);
            int body_end = _build_NFA(expr->nary.exprs[0], body_start, nfa, lexer);
            int end = alloc_NFA_state(nfa);
            NFA_TRANS(start, body_start, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(body_end, body_start, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(body_end, end, I_EPS, nfa) = B_TRUE;
            NFA_TRANS(start, end, I_EPS, nfa) = B_TRUE;
            return end;
        }
        case E_STRING:
        {
            int prev_end, prev_start = start;
            for (char *c = expr->string.str; *c; c++)
            {
                int i = find_char(*c, *c, lexer);
                prev_end = alloc_NFA_state(nfa);
                NFA_TRANS(prev_start, prev_end, i, nfa) = B_TRUE;
                prev_start = prev_end;
            }
            return prev_end;
        }
        case E_CRANGE:
        {
            char lb = expr->crange.lb, ub = expr->crange.up;
            int i = find_char(lb, ub, lexer);
            int end = alloc_NFA_state(nfa);
            NFA_TRANS(start, end, i, nfa) = B_TRUE;
            return end;
        }
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

void build_NFA(GExpr *expr, NFA *nfa, Lexer *lexer)
{
    nfa->state_num = count_state(expr) + 1;
    nfa->used_state_num = 0;
    nfa->char_num = lexer->char_num;

    nfa->trans = (int*) calloc(nfa->state_num * nfa->state_num * lexer->char_num, sizeof(int));
    nfa->start = alloc_NFA_state(nfa);
    nfa->end = _build_NFA(expr, nfa->start, nfa, lexer);

    nfa->visiting = (int*) malloc(sizeof(int) * nfa->state_num);
    nfa->visiting_new = (int*) malloc(sizeof(int) * nfa->state_num);

    find_reachable(nfa);
    absurb_eps(nfa);
}

void regist_NFA(GParser *parser, Lexer *lexer)
{
    lexer->nfa = (NFA*) malloc(sizeof(NFA) * (parser->asset->asset_num));

    int nfa_num = 0;
    GExpr exprs[parser->asset->asset_num];

    for (int i = 0; i < parser->def_num; i++)
    {
        char *term = parser->defs[i].identity.str;
        if (IS_LEXING_TERM(term))
        {
            exprs[nfa_num] = *(parser->defs[i].identity.expr);
            lexer->nfa[nfa_num].name = term;
            nfa_num++;
        }
    }

    for (int i = 0; i < parser->asset->asset_num; i++)
    {
        SType stype = parser->asset->stypes[i];
        if (stype == S_GRAMMAR)
        {
            exprs[nfa_num].kind = E_STRING;
            exprs[nfa_num].string.str = parser->asset->starts[i];
            lexer->nfa[nfa_num].name = parser->asset->starts[i];
            nfa_num++;
        }
    }

    lexer->nfa_num = nfa_num;
    lexer->is_alive = (int*) malloc(sizeof(int) * nfa_num);
    lexer->nfa_result = (int*) malloc(sizeof(int) * nfa_num);
    lexer->char_valid = (int*) malloc(sizeof(int) * nfa_num);
    lexer->lens = (int*) malloc(sizeof(int) * nfa_num);

    for (int i = 0; i < lexer->nfa_num; i++)
        build_NFA(exprs + i, lexer->nfa + i, lexer);
}

//-----------------------------------------------------------------

void init_NFA_run(NFA *nfa)
{
    for (int i = 0; i < nfa->state_num; i++)
        nfa->visiting[i] = NFA_TRANS(nfa->start, i, I_EPS, nfa);
}

int step_NFA(int *char_valid, NFA *nfa)
{
    int state_num = nfa->state_num, char_num = nfa->char_num, total = 0;

    for (int i = 0; i < state_num; i++)
        nfa->visiting_new[i] = 0;

    for (int k = 0; k < char_num; k++)
    {
        if (!char_valid[k])
            continue;
        for (int i = 0; i < state_num; i++)
        {
            if (!nfa->visiting[i])
                continue;
            for (int j = 0; j < state_num; j++)
            {
                nfa->visiting_new[j] |= NFA_TRANS(i, j, k, nfa);
                total |= nfa->visiting_new[j];
            }
        }
    }
    
    int *tmp = nfa->visiting;
    nfa->visiting = nfa->visiting_new;
    nfa->visiting_new = tmp;

    return total > 0;

}

int accepts_next_token(char c, Lexer *lexer)
{
    int alive_num = 0;

    for (int i = 0; i < lexer->char_num; i++)
        lexer->char_valid[i] = (lexer->char_lbs[i] <= c) && (c <= lexer->char_ubs[i]);
    
    for (int i = 0; i < lexer->nfa_num; i++)
    {
        if (!lexer->is_alive[i])
            continue;

        lexer->is_alive[i] = step_NFA(lexer->char_valid, lexer->nfa + i);
        lexer->lens[i] += lexer->is_alive[i];
        alive_num += lexer->is_alive[i];
    }

    return alive_num;
}

void lexing(GParser *parser, Lexer *lexer)
{
    regist_char(parser->asset, lexer);
    regist_NFA(parser, lexer);

    char *cursor = lexer->input;

    while (*cursor)
    {
        for (int i = 0; i < lexer->nfa_num; i++)
        {
            lexer->is_alive[i] = B_TRUE;
            lexer->lens[i] = 0;
            init_NFA_run(lexer->nfa + i);
        }

        int tok_len = 0;
        SKIP_SPACE(cursor);
        if (*cursor == c_null) return;

        while (accepts_next_token(*(cursor + tok_len), lexer))
        {
            for (int i = 0; i < lexer->nfa_num; i++)
                lexer->nfa_result[i] = lexer->nfa[i].visiting[lexer->nfa[i].end];
            tok_len++;
        }
        int best_idx = -1;

        for (int i = 0; i < lexer->nfa_num; i++)
        {
            if (lexer->lens[i] > lexer->lens[best_idx] && lexer->nfa_result[i])
                best_idx = i;
        }

        if (best_idx != -1)
        {
            printf("%.*s ", tok_len, cursor);
            cursor += tok_len;
        }
        else
        {
            printf("failed to lex\n");
            exit(1);
        }
    }
}


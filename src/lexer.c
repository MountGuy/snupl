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

void lexing(GParser *parser, Lexer *lexer)
{
    regist_assets(parser->asset, lexer);

    int char_num = lexer->char_num;
    int char_valid[lexer->char_num];
    
    int nfa_num = 0;
    lexer->nfa = (NFA*) malloc(sizeof(NFA) * (parser->def_num + lexer->string_num));
    lexer->nfa_num = 0;

    for (int i = 0; i < parser->def_num; i++)
    {
        char *term = parser->defs[i].identity.str;
        if (term[0] == '_' && term[1] != '_')
        {
            build_NFA(parser->defs[i].identity.expr, lexer->nfa + nfa_num, lexer);
            lexer->nfa[nfa_num].name = parser->defs[i].identity.str;
            nfa_num++;
        }
    }

    for (int i = 0; i < lexer->string_num; i++)
    {
        GExpr expr;
        expr.kind = E_STRING;
        expr.string.str = lexer->strings[i];
        build_NFA(&expr, lexer->nfa + nfa_num, lexer);
        lexer->nfa[nfa_num].name = expr.string.str;
        nfa_num++;
    }
    lexer->nfa_num = nfa_num;

    char *cursor = lexer->input;
    int is_alive[nfa_num];

    while (*cursor)
    {
        for (int i = 0; i < nfa_num; i++)
        {
            is_alive[i] = B_TRUE;
            init_NFA_run(lexer->nfa + i);
        }

        int tok_len = 0;

        while (B_TRUE)
        {
            int alive_num = 0;
            char letter = *(cursor + tok_len);

            for (int i = 0; i < char_num; i++)
                char_valid[i] = (lexer->char_lbs[i] <= letter) && (letter <= lexer->char_ubs[i]);
            
            for (int i = 0; i < lexer->nfa_num; i++)
            {
                if (!is_alive[i])
                    continue;
                is_alive[i] = step_NFA(char_valid, lexer->nfa + i);
                alive_num += is_alive[i];
            }
            if (alive_num == 0)
            {
                for (int i =0; i < tok_len; i++)
                {
                    printf("%c", *(cursor + i));
                }
                newline;
                cursor += tok_len;
                while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n') cursor++;
                break;
            }
            else
            {
                tok_len++;
            }
        }
    }

}
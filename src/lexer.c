#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "lexer.h"
#include "ebnf_util.h"

#define NFA_has_c(nfa, c) ((nfa)->char_to_idx[(int) (c)] != -1)

int find_char(char left, char right, Lexer *lexer)
{
    for (int i = 0; i < lexer->char_num; i++)
        if (left == lexer->l_chars[i] && right == lexer->r_chars[i])
            return i;
    printf("wtf findchar\n");
    exit(1);
}

void build_NFA(GExpr *expr, NFA *nfa, Lexer *lexer)
{
    nfa->state_num = count_state(expr) + 1;
    nfa->used_state_num = 0;
    nfa->char_num = lexer->char_num;

    printf("%d %d\n", nfa->state_num, lexer->char_num);
    nfa->trans = (int*) calloc(nfa->state_num * nfa->state_num * lexer->char_num, sizeof(int));
    nfa->start = alloc_NFA_state(nfa);
    nfa->end = _build_NFA(expr, nfa->start, nfa, lexer);

    nfa->visiting = (int*) malloc(sizeof(int) * nfa->state_num);
    nfa->visiting_new = (int*) malloc(sizeof(int) * nfa->state_num);

    find_reachable(nfa);
    absurb_eps(nfa);

    // printf("state num: %d char_num: %d table: %d KB\n", nfa->state_num, nfa->char_num, nfa->state_num * nfa->state_num * nfa->char_num / 1024);
}

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
            char left = expr->crange.start, right = expr->crange.end;
            int i = find_char(left, right, lexer);
            int end = alloc_NFA_state(nfa);
            NFA_TRANS(start, end, i, nfa) = B_TRUE;
            return end;
        }
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

int count_state(GExpr *expr)
{
    switch (expr->kind)
    {
        case E_ALTER:
        {
            int count = 1;
            for (int i = 0; i < expr->nary.expr_num; i++)
            count += count_state(expr->nary.exprs[i]) + 1;
            return count;
        }
        case E_CONCAT:
        {
            int count = 0;
            for (int i = 0; i < expr->nary.expr_num; i++)
            count += count_state(expr->nary.exprs[i]) + 1;
            return count;
        }   
        case E_OPTION:
        case E_REPEAT:
            return count_state(expr->nary.exprs[0]) + 2;
        case E_STRING:
            return strlen(expr->string.str);
        case E_CRANGE:
            return 1;
        default:
            printf("wtf 4 %d\n", expr->kind);
            exit(1);
    }
}

int alloc_NFA_state(NFA *nfa)
{
    int state = nfa->used_state_num;
    nfa->used_state_num++;
    return state;
}


void find_reachable(NFA *nfa)
{
    int state_num = nfa->used_state_num;
    int *stack = (int*) malloc(sizeof(int) * state_num);
    int *visited = (int*) malloc(sizeof(int) * state_num);
    int curr_state, backed_state = -1, next_state, top;

    for (int state = 0; state < state_num; state++)
    {
        for (int i = 0; i < state_num; i++)
            visited[i] = 0;
        top = 0;
        stack[0] = state;
        visited[state] = 1;
        backed_state = -1;

        while (top >= 0)
        {
            curr_state = stack[top];
            if (backed_state == -1)
                next_state = 0;
            else
                next_state = backed_state + 1;
            while
            (
                next_state < state_num && (
                !NFA_TRANS(curr_state, next_state, I_EPS, nfa) ||
                visited[next_state] == 1
            ))
            next_state++;

            if (next_state == state_num)
            {
                backed_state = curr_state;
                curr_state = stack[top];
                top--;
                continue;
            }
            else
            {
                top++;
                stack[top] = next_state;
                backed_state = -1;
                visited[next_state] = 1;
                continue;
            }
        }

        for (int i = 0; i < state_num; i++)
            NFA_TRANS(state, i, I_EPS, nfa) |= visited[i];
    }
}

void absurb_eps(NFA *nfa)
{
    int state_num = nfa->state_num, char_num = nfa->char_num;
    
    for (int i = 0; i < state_num; i++)
        for (int j = 0; j < state_num; j++)
            for (int k = 0; k < char_num; k++)
                if (NFA_TRANS(i, j, k, nfa))
                    for (int l = 0; l < state_num; l++)
                        NFA_TRANS(i, l, k, nfa) |= NFA_TRANS(j, l, I_EPS, nfa);
}

void regist_char(char left, char right, Lexer *lexer)
{
    int char_num = lexer->char_num;
    for (int i = 0; i < char_num; i++)
        if (left == lexer->l_chars[i] && right == lexer->r_chars[i])
            return;

    lexer->l_chars[char_num] = left;
    lexer->r_chars[char_num] = right;
    lexer->char_num++;
}

void regist_string(char *string, Lexer *lexer)
{
    for (int i = 0; i < lexer->gm_str_num; i++)
        if (string == lexer->gm_strs[i])
            return;

    lexer->gm_strs[lexer->gm_str_num] = string;
    lexer->gm_str_num++;
}

void regist_assets(Asset *asset, Lexer *lexer)
{
    lexer->gm_strs = (char**) malloc(sizeof(char*) * asset->asset_size);
    lexer->gm_str_num = 0;
    lexer->l_chars = (char*) malloc(sizeof(char*) * asset->asset_size);
    lexer->r_chars = (char*) malloc(sizeof(char*) * asset->asset_size);
    lexer->char_num = 0;

    for (int i = 0; i < asset->asset_num; i++)
    {
        char *string = asset->starts[i];
        SType stype = asset->stypes[i];

        switch (stype)
        {
            case S_BASICS:
                for (char *c = string; *c; c++)
                    regist_char(*c, *c, lexer);
            break;
            case S_CRANGE:
                regist_char(string[0], string[1], lexer);
                break;
            case S_GRAMMAR:
                regist_string(string, lexer);
                break;
            case S_IDENTITY:
                break;
            default:
                printf("wtf lexing\n");
                exit(1);
        }
    }
}

void init_NFA_run(NFA *nfa)
{
    for (int i = 0; i < nfa->state_num; i++)
        nfa->visiting[i] = NFA_TRANS(nfa->start, i, I_EPS, nfa);
}

int step_NFA(char c, NFA *nfa, Lexer *lexer)
{
    int state_num = nfa->state_num, char_num = lexer->char_num;
    for (int i = 0; i < state_num; i++)
        nfa->visiting_new[i] = 0;

    for (int k = 0; k < char_num; k++)
    {
        if (!(lexer->l_chars[k] <= c && c <= lexer->r_chars[k]))
            continue;
        for (int i = 0; i < state_num; i++)
        {
            if (!nfa->visiting[i])
                continue;
            for (int j = 0; j < state_num; j++)
                nfa->visiting_new[j] |= NFA_TRANS(i, j, k, nfa);
        }
    }
    
    int *tmp = nfa->visiting;
    nfa->visiting = nfa->visiting_new;
    nfa->visiting_new = tmp;
    
    int total = 0;
    for (int i = 0; i < state_num; i++)
        total += nfa->visiting[i];

    return total > 0;

}

void lexing(GParser *parser, Lexer *lexer)
{
    regist_assets(parser->asset, lexer);

    int nfa_num = 0;

    lexer->nfa = (NFA*) malloc(sizeof(NFA) * parser->lexterm_num);
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
    lexer->nfa_num = nfa_num;

    char *c = lexer->input;

    while (*c)
    {
        int left_string[lexer->gm_str_num];
        for (int i = 0; i < lexer->nfa_num; i++)
        {
            init_NFA_run(lexer->nfa + i);
        }
        for (int i = 0; i < lexer->gm_str_num; i++)
            left_string[i] = B_TRUE;

        int string_len = 0;
        while (B_TRUE)
        {
            int left_count = 0;
            for (int i = 0; i < lexer->nfa_num; i++)
            {
                int result = step_NFA(*(c + string_len), lexer->nfa + i, lexer);
                // if (result)
                //     printf("%s still can accept %c\n", lexer->nfa[i].name, *(c + string_len));
                // else
                //     printf("%s can't accept %c\n", lexer->nfa[i].name, *(c + string_len));
                left_count += result;

            }
            for (int i = 0; i < lexer->gm_str_num; i++)
            {
                if (!left_string[i])
                    continue;
                else if ((lexer->gm_strs[i][string_len] == c_null) ||
                    (lexer->gm_strs[i][string_len] != *(c + string_len)))
                {
                    left_string[i] = B_FALSE;
                    continue;
                }
                else
                    left_count++;
            }
            if (left_count == 0)
            {
                for (int i =0; i < string_len; i++)
                {
                    printf("%c",*(c + i));
                }
                newline;
                c += string_len;
                while (*c == ' ' || *c == '\t' || *c == '\n') c++;
                break;
            }
            else
            {
                string_len++;
            }
        }
    }

}
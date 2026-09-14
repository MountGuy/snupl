#include "common.h"
#include "struct.h"
#include "lexer.h"
#include "dump.h"
#include "arena.h"

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

void add_ending(char *name, int end, TType end_type, NFABuilder *builder)
{
    for (int i = 0; i < builder->end_num; i++)
    {
        if (name == builder->end_names[i] && end == builder->ends[i])
            return;
    }
    builder->end_names[builder->end_num] = name;
    builder->end_types[builder->end_num] = end_type;
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
            builder->state_num += strlen(expr->string.value);
            if (type == D_GRAMMAR)
                builder->state_num += 1;
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
                add_ending(expr->string.value, cur_start, T_GRAMMAR, builder);
                return -1;
            }
            case E_CRANGE:
            case E_IDENTITY:
                return -1;
        }
    }
    printf("Unexpected case during build NFA...\n");
    exit(1);
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

void postproc_trans(NFABuilder *builder)
{
    int state_num = builder->state_num, char_num = builder->char_num;
    for (int i = 0; i < state_num; i++)
        builder->trans[i][i][I_EPS] = true;

    for (int c = 0; c < char_num; c++)
        for (int k = 0; k < state_num; k++)
            for (int i = 0; i < state_num; i++)
                if (builder->trans[i][k][c])
                    for (int j = 0; j < state_num; j++)
                        builder->trans[i][j][c] |= builder->trans[i][k][c] && builder->trans[k][j][I_EPS];
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
    builder.end_types = (TType*) malloc(sizeof(TType) * 1000);
    builder.end_num = 0;
    builder.ends = (int*) malloc(sizeof(int) * 1000);

    builder.start = alloc_NFA_state(&builder);

    for (int i = 0; i < grammar->def_num; i++)
    {
        MetaDef def = grammar->defs[i];
        if (def.type == D_TERM)
            builder.state_num++;
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
            add_ending(def.identity, _end, T_TERM, &builder);
        }
        else if (def.type == D_GRAMMAR)
        {
            _build_NFA(def.expr, -1, D_GRAMMAR, &builder);
        }
    }

    postproc_trans(&builder);

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

void init_scanner(NFAScanner *scanner)
{
    NFA *nfa = scanner->nfa;
    int start = nfa->start;
    for (int i = 0; i < scanner->state_num; i++)
        scanner->visiting[i] = nfa->trans[start][i][I_EPS];
    scanner->visiting[start] = true;

    for (int i = 0; i < scanner->end_num; i++)
        scanner->lens[i] = 0;
    scanner->len = 0;
}

int step_NFA(char letter, NFAScanner *scanner)
{
    NFA *nfa = scanner->nfa;
    scanner->len++;
    
    for (int i = 0; i < nfa->state_num; i++)
        scanner->tmp[i] = false;

    for (int cdx = 1; cdx < nfa->char_num; cdx++)
        if (nfa->lbs[cdx] <= letter && letter <= nfa->ubs[cdx])
            for (int j = 0; j < nfa->state_num; j++)
                if (scanner->visiting[j])
                    for (int k = 0; k < nfa->state_num; k++)
                        scanner->tmp[k] |= nfa->trans[j][k][cdx];

    memcpy(scanner->visiting, scanner->tmp, sizeof(int) * nfa->state_num);

    for (int i = 0; i < nfa->end_num; i++)
    {
        int end = nfa->ends[i];
        if (scanner->visiting[end] == true)
            scanner->lens[i] = scanner->len;
    }

    int alive_state = 0;
    for (int i = 0; i < nfa->state_num; i++)
        if (scanner->visiting[i])
            alive_state++;

    return alive_state;
}

void skip_nontoken(Lexer *lexer)
{
    char *cursor = lexer->cursor;

    while (true)
    {
        if (*cursor == '/' && *(cursor + 1) == '/')
        {
            while (*cursor != '\n')
                cursor++;
            cursor++;
            lexer->line++;
            lexer->line_start = cursor;
        }
        else if (*cursor == ' ' || *cursor == '\t')
            while(*cursor == ' ' || *cursor == '\t')
                cursor++;
        else if (*cursor == '\n')
        {
            cursor++;
            lexer->line++;
            lexer->line_start = cursor;
        }
        else
            break;
    }
    lexer->cursor = cursor;
}

void lexing(Lexer *lexer)
{
    lexer->tokens = (Token*) malloc(sizeof(Token) * lexer->input_len);
    lexer->cursor = lexer->input;
    lexer->line_start = lexer->input;
    lexer->token_num = 0;
    lexer->line = 1;

    NFAScanner scanner;
    NFA *nfa = lexer->nfa;
    scanner.nfa = nfa;
    scanner.state_num = nfa->state_num;
    scanner.end_num = nfa->end_num;
    scanner.visiting = (int*) malloc(sizeof(int) * nfa->state_num);
    scanner.tmp = (int*) malloc(sizeof(int) * nfa->state_num);
    scanner.lens = (int*) malloc(sizeof(int) * nfa->end_num);

    skip_nontoken(lexer);
    while (lexer->cursor - lexer->input < lexer->input_len)
    {
        init_scanner(&scanner);
        
        int tok_len = 0;
        while (step_NFA(*(lexer->cursor + tok_len), &scanner))
            tok_len++;

        int best_idx = -1, best_len = 0;
        for (int i = 0; i < nfa->end_num; i++)
        {
            if (
                (scanner.lens[i] > best_len) || 
                (scanner.lens[i] == best_len && (nfa->end_types[best_idx] == T_TERM && nfa->end_types[i] == T_GRAMMAR))
            )
            {
                best_idx = i;
                best_len = scanner.lens[i];
            }
        }
            
        if (best_idx != -1)
        {
            Token token = {
                .string = add_string(lexer->cursor, best_len, lexer->arena),
                .string_len = best_len,
                .col = lexer->cursor - lexer->line_start + 1,
                .line = lexer->line,
                .type = nfa->end_types[best_idx],
                .name = nfa->end_names[best_idx],
            };
            lexer->tokens[lexer->token_num++] = token;
            lexer->cursor += best_len;
            skip_nontoken(lexer);
        }
        else
        {
            printf("failed to lex\n");
            exit(1);
        }
    }
}



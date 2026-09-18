#include "lexer.h"

#define BYTE_SIZE 8
#define SLB (sizeof(unsigned long long int) * BYTE_SIZE)
#define OFFSET(s, c, e, sn, cn) ((e) + (sn) * ((c) + (cn) * (s)))
#define READ_OFFSET(p, o) ((p)[(o) / SLB] & ((unsigned long long int) 1 << ((o) % SLB)))
#define WRITE_OFFSET(p, o) ((p)[(o) / SLB] |= ((unsigned long long int) 1 << ((o) % SLB)))

int alloc_NFA_state(NFABuilder *builder)
{
    return builder->used_state_num++;
}

void add_char(char lb, char ub, Chunk *lubs)
{
    char *data = lubs->data;
    for (int i = 0; i < lubs->used; i += 2)
        if (lb == data[i] && ub == data[i + 1])
            return;

    append_data(&lb, 1, lubs);
    append_data(&ub, 1, lubs);
}

int find_char(char lb, char ub, Chunk *lubs)
{
    char *data = lubs->data;
    for (int i = 0; i < lubs->used; i += 2)
        if (data[i] == lb && data[i + 1] == ub)
            return i / 2;

    printf("Unregisted character: %c~%c\n", lb, ub);
    exit(1);
}

int count_state(MetaExpr *expr, Grammar *grammar)
{
    switch (expr->kind)
    {
        case E_ALTER:
        case E_CONCAT:
            int total = expr->nary.expr_num + (expr->kind == E_ALTER);
            for (int i = 0; i < expr->nary.expr_num; i++)
                total += count_state(expr->nary.exprs[i], grammar);
            return total;
        case E_OPTION:
        case E_REPEAT:
            return count_state(expr->unary.expr, grammar) + 2;
        case E_STRING:
            return strlen(expr->string.value);
        case E_CRANGE:
            return 1;
        case E_IDENTITY:
            MetaExpr *body = grammar->defs[expr->identity.idx].expr;
            return count_state(body, grammar);
        default:
            printf("Unexpected expr kind during prescan\n");
            exit(1);
    }
}

void gather_char(MetaExpr *expr, Chunk *lubs)
{
    switch (expr->kind)
    {
        case E_STRING:
            for (char *c = expr->string.value; *c; c++)
                add_char(*c, *c, lubs);
            break;
        case E_CRANGE:
            add_char(expr->crange.lb, expr->crange.ub, lubs);
            break;
        case E_ALTER:
        case E_CONCAT:
            for (int i = 0; i < expr->nary.expr_num; i++)
                gather_char(expr->nary.exprs[i], lubs);
            break;
        case E_OPTION:
        case E_REPEAT:
            gather_char(expr->unary.expr, lubs);
        default:
            break;
    }
}

int can_trans(int start, int cdx, int end, NFABuilder *builder)
{
    int offset = OFFSET(start, cdx, end, builder->state_num, builder->char_num);
    return READ_OFFSET(builder->trans, offset) > 0;
}

void add_trans(int start, int cdx, int end, NFABuilder *builder)
{
    int offset = OFFSET(start, cdx, end, builder->state_num, builder->char_num);
    WRITE_OFFSET(builder->trans, offset);
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
                add_trans(start, I_EPS, _start, builder);
                add_trans(_end, I_EPS, end, builder);
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
                add_trans(cur_end, I_EPS, cur_start, builder);
            }
            return cur_start;
        }
        case E_OPTION:
        {
            int end = alloc_NFA_state(builder);
            int body_start = alloc_NFA_state(builder);
            int body_end = _build_NFA(expr->unary.expr, body_start, builder);

            add_trans(start, I_EPS, body_start, builder);
            add_trans(body_end, I_EPS, end, builder);
            add_trans(start, I_EPS, end, builder);

            return end;
        }
        case E_REPEAT:
        {
            int end = alloc_NFA_state(builder);
            int body_start = alloc_NFA_state(builder);
            int body_end = _build_NFA(expr->unary.expr, body_start, builder);

            add_trans(start, I_EPS, body_start, builder);
            add_trans(body_end, I_EPS, body_start, builder);
            add_trans(body_end, I_EPS, end, builder);
            add_trans(start, I_EPS, end, builder);

            return end;
        }
        case E_STRING:
        {
            int cur_start = start, cur_end;
            for (char *c = expr->string.value; *c; c++)
            {
                cur_end = alloc_NFA_state(builder);
                int cdx = find_char(*c, *c, &builder->lubs);
                add_trans(cur_start, cdx, cur_end, builder);
                cur_start = cur_end;
            }
            return cur_start;
        }
        case E_CRANGE:
        {
            char lb = expr->crange.lb, ub = expr->crange.ub;
            int idx = find_char(lb, ub, &builder->lubs);
            int end = alloc_NFA_state(builder);
            add_trans(start, idx, end, builder);
            return end;
        }
        case E_IDENTITY:
        {
            int idx = expr->identity.idx;
            MetaExpr *body = builder->grammar->defs[idx].expr;
            int end = _build_NFA(body, start, builder);
            return end;
        }
        default:
            printf("Unexpected meta expression type during building NFA: %d\n", expr->kind);
            exit(1);
    }
    printf("Unexpected case during build NFA...\n");
    exit(1);
}

void postproc_trans(NFABuilder *builder)
{
    int state_num = builder->state_num, char_num = builder->char_num;
    unsigned long long int *trans = builder->trans;

    for (int i = 0; i < state_num; i++)
        add_trans(i, I_EPS, i, builder);

    for (int c = 0; c < char_num; c++)
        for (int k = 0; k < state_num; k++)
            for (int i = 0; i < state_num; i++)
                if (can_trans(i, c, k, builder))
                {
                    int off_dest = OFFSET(i, c, 0, state_num, char_num);
                    int off_source = OFFSET(k, I_EPS, 0, state_num, char_num);
                    for (int j = 0; j < state_num / SLB; j++)
                        trans[off_dest / SLB + j] |= trans[off_source / SLB + j];
                }
}

void build_NFA(Grammar *grammar, NFA *nfa)
{
    char c = C_EPS;

    Chunk lubs = init_chunk(sizeof(char), 1);
    append_data(&c, 1, &lubs);
    append_data(&c, 1, &lubs);

    int exact_state_num = 0;
    for (int i = 0; i < grammar->def_num; i++)
    {
        MetaDef def = grammar->defs[i];
        gather_char(def.expr, &lubs);
        if (def.type == D_TERM)
            exact_state_num += count_state(def.expr, grammar) + 1;
    }

    for (int i = 0; i < grammar->tokc_num; i++)
    {
        TokenClass tc = grammar->tokcs[i];
        if (tc.type == T_CONST)
            exact_state_num += strlen(tc.name) + 2;
    }

    int state_num = (exact_state_num / SLB + 1) * SLB;

    int char_num = lubs.used / 2;
    NFABuilder builder = {
        .trans = calloc(state_num * char_num * state_num / BYTE_SIZE, 1),
        .state_num = state_num,
        .char_num = char_num,
        .used_state_num = 1,
        .lubs = lubs,
        .grammar = grammar,
    };
    int *end_states = malloc(sizeof(int) * grammar->tokc_num);

    for (int i = 0; i < grammar->tokc_num; i++)
    {
        TokenClass class = grammar->tokcs[i];
        if (class.type == T_VAR)
        {
            MetaExpr *expr = grammar->defs[class.idx].expr;
            end_states[i] = _build_NFA(expr, 0, &builder);
        }
        else if (class.type == T_CONST)
        {
            MetaExpr expr = {
                .kind = E_STRING,
                .string.value = class.name,
            };
            end_states[i] = _build_NFA(&expr, 0, &builder);
        }
    }

    postproc_trans(&builder);

    nfa->trans = builder.trans;
    nfa->lbs = malloc(sizeof(char) * lubs.used / 2);
    nfa->ubs = malloc(sizeof(char) * lubs.used / 2);
    nfa->tokcs = grammar->tokcs;
    nfa->state_num = builder.state_num;
    nfa->exact_state_num = exact_state_num;
    nfa->char_num = builder.char_num;
    nfa->end_states = end_states;
    nfa->tokc_num = grammar->tokc_num;

    char *data = lubs.data;
    for (int i = 0; i < char_num; i++)
    {
        nfa->lbs[i] = data[2 * i];
        nfa->ubs[i] = data[2 * i + 1];
    }
}

void init_scanner(NFA *nfa, NFAScanner *scanner)
{
    memcpy(scanner->visiting, nfa->trans, nfa->state_num / BYTE_SIZE);

    for (int i = 0; i < nfa->tokc_num; i++)
        scanner->lens[i] = 0;
    scanner->len = 0;
}

int step_NFA(char letter, NFA *nfa, NFAScanner *scanner)
{
    int state_num = nfa->state_num, char_num = nfa->char_num;
    scanner->len++;

    for (int i = 0; i < state_num / SLB; i++)
        scanner->tmp[i] = 0;

    for (int cdx = 1; cdx < char_num; cdx++)
        if (nfa->lbs[cdx] <= letter && letter <= nfa->ubs[cdx])
            for (int j = 0; j < state_num; j++)
                if (READ_OFFSET(scanner->visiting, j))
                {
                    int offset = OFFSET(j, cdx, 0, state_num, char_num);
                    for (int k = 0; k < state_num / SLB; k++)
                        scanner->tmp[k] |= nfa->trans[k + offset / SLB];
                }

    memcpy(scanner->visiting, scanner->tmp, state_num / BYTE_SIZE);
    for (int i = 0; i < nfa->tokc_num; i++)
    {
        int end = nfa->end_states[i];
        if (READ_OFFSET(scanner->visiting, end))
            scanner->lens[i] = scanner->len;
    }

    int alive_state = 0;
    for (int i = 0; i < nfa->state_num; i++)
        if (READ_OFFSET(scanner->visiting, i))
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
    lexer->cursor = lexer->input;
    lexer->line_start = lexer->input;
    lexer->line = 1;

    Chunk tokens = init_chunk(sizeof(Token), 1);

    NFA *nfa = lexer->nfa;
    TokenClass *tokcs = nfa->tokcs;
    NFAScanner scanner = {
        .visiting = malloc(nfa->state_num / BYTE_SIZE),
        .tmp = malloc(nfa->state_num / BYTE_SIZE),
        .lens = malloc(sizeof(int) * nfa->tokc_num),
    };

    skip_nontoken(lexer);
    while (lexer->cursor - lexer->input < lexer->input_len)
    {
        init_scanner(nfa, &scanner);

        int tok_len = 0;
        while (step_NFA(*(lexer->cursor + tok_len), nfa, &scanner))
            tok_len++;

        int best_idx = -1, best_len = 0;
        for (int i = 0; i < nfa->tokc_num; i++)
        {
            if (
                (scanner.lens[i] > best_len) ||
                (scanner.lens[i] == best_len && tokcs[i].type == T_CONST)
            )
            {
                best_idx = i;
                best_len = scanner.lens[i];
            }
        }

        if (best_idx != -1)
        {
            Token token = {
                .tok_c = tokcs + best_idx,
                .string = add_string(lexer->cursor, best_len, lexer->arena),
                .string_len = best_len,
                .line = lexer->line,
                .col = lexer->cursor - lexer->line_start + 1,
            };
            append_data(&token, 1, &tokens);
            lexer->cursor += best_len;
            skip_nontoken(lexer);
        }
        else
        {
            printf("failed to lex\n");
            exit(1);
        }
    }
    free(scanner.visiting);
    free(scanner.tmp);
    free(scanner.lens);
    lexer->token_num = tokens.used;
    lexer->tokens = fix_chunk(&tokens);
}

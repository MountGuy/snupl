#include "lexer.h"
#include "bitop.h"
#include "dump.h"

#define IS_SKIP(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

static Chunk nfa_lexing(const char *input, NFAScanner *scanner, NFA *nfa);
static Chunk dfa_lexing(const char *input, DFA *dfa);

static Chunk slice_to_tokens(const char *input, Chunk *slice, Arena *arena, TokenClass *tokcs)
{
    int token_num = slice->used - 1;
    int *result = slice->data;
    Chunk tokens = init_large_chunk(sizeof(Token), token_num, true);

    if (token_num == 0 || result[3 * token_num] == -1)
        printf("failed to lex\n"), exit(1);

    int line = 1, last_nl = -1, cursor = 0;
    for (int i = 0; i < token_num; i++)
    {
        int tokc_idx = result[3 * i + 0];
        int start = result[3 * i + 1];
        int length = result[3 * i + 2];

        for (; cursor < start; cursor++)
            if (input[cursor] == '\n')
                line++, last_nl = cursor;

        Token token = {
            .tok_c = tokcs + tokc_idx,
            .string = add_string(input + start, length, arena),
            .string_len = length,
            .line = line,
            .col = start - last_nl,
        };
        append_data(&token, 1, &tokens);
    }

    return tokens;
}

const char *nfa_scan(const char *input, NFA *nfa)
{
    void *buffer = malloc(nfa->state_num / BYTE_SIZE * 2 + sizeof(int) * nfa->tokc_num);
    NFAScanner scanner = {
        .visiting = buffer,
        .tmp = buffer + nfa->state_num / BYTE_SIZE,
        .lens = buffer + nfa->state_num / BYTE_SIZE * 2,
    };
    Chunk slice = nfa_lexing(input, &scanner, nfa);
    const char *cursor = input + ((int*) slice.data)[slice.used * 3 - 2];

    del_chunk(&slice), free(buffer);

    return cursor;
}

Chunk lexing_through_nfa(const char *input, Grammar *grammar, Arena *arena)
{
    NFA nfa = build_NFA(grammar);
    NFAScanner scanner = {
        .visiting = malloc(nfa.state_num / BYTE_SIZE),
        .tmp = malloc(nfa.state_num / BYTE_SIZE),
        .lens = malloc(sizeof(int) * nfa.tokc_num),
    };

    Chunk slice = nfa_lexing(input, &scanner, &nfa);
    Chunk tokens = slice_to_tokens(input, &slice, arena, nfa.tokcs);
    del_chunk(&slice);
    del_NFA(&nfa);
    free(scanner.visiting), free(scanner.tmp), free(scanner.lens);

    return tokens;
}

static Chunk nfa_lexing(const char *input, NFAScanner *scanner, NFA *nfa)
{
    const char *cursor = input;
    TokenClass *tokcs = nfa->tokcs;
    Chunk results = init_chunk(sizeof(int) * 3, true);

    while (*cursor)
    {
        init_scanner(nfa, scanner);
        
        while(IS_SKIP(*cursor))
            cursor++;

        if (*cursor == c_null)
            break;

        int tok_len = 0;
        while (step_NFA(cursor[tok_len], nfa, scanner))
            tok_len++;

        int best_idx = -1, best_len = 0;
        for (int i = 0; i < nfa->tokc_num; i++)
            if (
                (scanner->lens[i] > best_len) ||
                (scanner->lens[i] > 0 && best_idx == -1) ||
                (scanner->lens[i] == best_len && best_len > 0 && tokcs[i].type == T_CONST)
            )
                best_idx = i, best_len = scanner->lens[i];


        int bound[3] = { best_idx, cursor - input, best_len };
        append_data(bound, 1, &results);

        if (best_idx == -1)
            return results;

        cursor += best_len;
    }

    int end[3] = { -2, cursor - input, 0 };
    append_data(end, 1, &results);

    return results;
}

const char *dfa_scan(const char *input, DFA *dfa)
{
    Chunk slice = dfa_lexing(input, dfa);
    const char *cursor = input + ((int*) slice.data)[slice.used * 3 - 2];

    del_chunk(&slice);

    return cursor;
}

Chunk lexing_through_dfa(const char *input, Grammar *grammar, Arena *arena)
{
    DFA dfa = build_DFA(grammar);
    Chunk slice = dfa_lexing(input, &dfa);
    Chunk tokens = slice_to_tokens(input, &slice, arena, dfa.tokcs);
    del_chunk(&slice);
    del_DFA(&dfa);

    return tokens;
}

static Chunk dfa_lexing(const char *input, DFA *dfa)
{
    const char *cursor = input;
    int (*trans)[256] = dfa->trans;
    int *tokc_output = dfa->tokc_output;
    Chunk results = init_chunk(sizeof(int) * 3, true);

    while (*cursor)
    {
        while(IS_SKIP(*cursor))
            cursor++;

        if (*cursor == c_null)
            break;

        int state = 1, length = 0, best_length = -1, best_idx = -1;

        while (state != 0)
        {
            state = trans[state][(uchar) cursor[length++]];
            if (tokc_output[state] >= 0)
                best_length = length, best_idx = tokc_output[state];
        }
        int bound[3] = { best_idx, cursor - input, best_length };
        append_data(bound, 1, &results);

        if (best_idx == -1)
            return results;

        cursor += best_length;
    }

    int end[3] = { -2, cursor - input, 0 };
    append_data(end, 1, &results);

    return results;
}

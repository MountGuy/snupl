#include "nfa.h"
#include "character.h"
#include "lexer.h"
#include "bitop.h"
#include "dump.h"

#define IS_SKIP(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

Chunk lexing(char *input, Grammar *grammar, Arena *arena)
{
    int input_len = strlen(input), line = 1;
    char *cursor = input, *last_nl = input - 1;

    Chunk tokens = init_chunk(sizeof(Token), 1);
    NFA nfa = build_NFA(grammar);
    TokenClass *tokcs = nfa.tokcs;

    void *buffer = malloc(nfa.state_num / BYTE_SIZE * 2 + sizeof(int) * nfa.tokc_num);
    NFAScanner scanner = {
        .visiting = buffer,
        .tmp = buffer + nfa.state_num / BYTE_SIZE,
        .lens = buffer + nfa.state_num / BYTE_SIZE * 2,
    };

    while (cursor - input < input_len)
    {
        init_scanner(&nfa, &scanner);

        for (; IS_SKIP(*cursor); cursor++)
            if (*cursor == '\n')
                line++, last_nl = cursor;

        if (*cursor == c_null)
            break;

        int tok_len = 0;
        while (step_NFA(cursor[tok_len], 1, &nfa, &scanner))
            tok_len++;

        int best_idx = -1, best_len = 0;
        for (int i = 0; i < nfa.tokc_num; i++)
            if (
                (scanner.lens[i] > best_len) ||
                (scanner.lens[i] > 0 && best_idx == -1) ||
                (scanner.lens[i] == best_len && best_len > 0 && tokcs[i].type == T_CONST)
            )
                best_idx = i, best_len = scanner.lens[i];

        if (best_idx != -1)
        {
            Token token = {
                .tok_c = tokcs + best_idx,
                .string = add_string(cursor, best_len, arena),
                .string_len = best_len,
                .line = line,
                .col = cursor - last_nl,
            };
            append_data(&token, 1, &tokens);
            cursor += best_len;
        }
        else
            printf("failed to lex\n"), exit(1);
    }

    free(buffer);
    del_NFA(&nfa);

    return tokens;
}

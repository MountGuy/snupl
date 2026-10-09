#include "dfa.h"
#include "bitop.h"
#include "chunk.h"
#include "dump.h"

int find_state(int state_size, ulli *state, Chunk *states)
{
    int state_num = states->used;
    ulli *data = states->data;

    for (int i = 0; i < state_num; i++)
    {
        for (int j = 0; j < state_size; j++)
        {
            if (state[j] != data[i * state_size + j])
                goto next;
        }
        return i;
        next:
    }
    return -1;
}

void nfa_trans(ulli *before, ulli *after, uchar letter, NFA *nfa)
{
    int nstate_num = nfa->state_num, char_num = nfa->char_num;

    for (int i = 0; i < nstate_num / SZLIB; i++)
        after[i] = 0;

    for (int cdx = 1; cdx < char_num; cdx++)
        if (nfa->lbs[cdx] <= letter && letter <= nfa->ubs[cdx])
            for (int j = 0; j < nstate_num; j++)
                if (READ_OFFSET(before, j))
                {
                    int offset = OFFSET(j, cdx, 0, nstate_num, char_num);
                    for (int k = 0; k < nstate_num / SZLIB; k++)
                        after[k] |= nfa->trans[k + offset / SZLIB];
                }
}

void add_dfa_trans(int before, int after, uchar letter, DFABuilder *builder)
{
    int max = builder->trans.max;

    if (after >= max || before >= max)
        expand_chunk(&builder->trans);

    int *data = builder->trans.data;
    data += before * 256;

    data[letter] = after;
}

void nfa_to_dfa(ulli *before, NFA *nfa, DFABuilder *builder)
{
    int state_size = builder->state_size;
    int before_idx = find_state(state_size, before, &builder->states);

    ulli after[state_size];

    for (int i = 0; i < 256; i++)
    {
        nfa_trans(before, after, (uchar) i, nfa);
        int after_idx = find_state(state_size, after, &builder->states);
        if (after_idx == -1)
        {
            after_idx = builder->states.used;
            append_data(after, 1, &builder->states);
            add_dfa_trans(before_idx, after_idx, (uchar) i, builder);
            nfa_to_dfa(after, nfa, builder);
        }
        else
            add_dfa_trans(before_idx, after_idx, (uchar) i, builder);
    }
}

DFA build_DFA(Grammar *grammar)
{
    NFA nfa = build_NFA(grammar);
    int state_size = nfa.state_num / SZLIB;

    DFABuilder builder = {
        .used_state_num = 0,
        .state_size = state_size,
        .trans = init_chunk(sizeof(int) * 256, true),
        .states = init_chunk(sizeof(ulli) * state_size, true),
        .nfa = &nfa,
    };

    ulli before[state_size];

    for (int i = 0; i < state_size; i++)
        before[i] = 0;
    append_data(before, 1, &builder.states);

    for (int i = 0; i < state_size; i++)
        before[i] = nfa.trans[i];
    append_data(before, 1, &builder.states);

    nfa_to_dfa(before, &nfa, &builder);

    int state_num = builder.states.used;

    DFA dfa = {
        .trans = malloc(sizeof (*dfa.trans) * state_num),
        .state_num = state_num,
        .tokc_output = malloc(sizeof(int) * state_num),
        .tokc_num = nfa.tokc_num,
        .tokcs = nfa.tokcs, 
    };

    for (int i = 0; i < dfa.state_num; i++)
    {
        int *trans = (int*) builder.trans.data + 256 * i;
        for (int c = 0; c < 256; c++)
            dfa.trans[i][c] = trans[c];
    }

    TokenClass *tokcs = nfa.tokcs;

    for (int i = 0; i < dfa.state_num; i++)
    {
        int tokc_idx = -1;
        ulli *state = (ulli*) builder.states.data + state_size * i;
        for (int i = 0; i < nfa.tokc_num; i++)
        {
            int end_state = nfa.end_states[i];
            if (READ_OFFSET(state, end_state))
            {
                if (tokc_idx == -1)
                    tokc_idx = i;
                else if (tokcs[i].type == T_CONST && tokcs[tokc_idx].type == T_VAR)
                    tokc_idx = i;
                else if (tokcs[i].type == T_VAR && tokcs[tokc_idx].type == T_CONST)
                    continue;
                else
                {
                    printf("Some state has ambiguous token class output!\n");
                    printf("%d %s\n", tokcs[i].type, tokcs[i].name);
                    printf("%d %s\n", tokcs[tokc_idx].type, tokcs[tokc_idx].name);
                    exit(1);
                }
            }
        }
        dfa.tokc_output[i] = tokc_idx;
    }

    del_chunk(&builder.states);
    del_chunk(&builder.trans);

    return dfa;
}


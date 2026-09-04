#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ebnf_util.h"
#include "ebnf.h"
#include "automata.h"


int main(int argv, char *argc[])
{
    if (argv < 2)
    {
        printf("Grammar file path required...\n");
        return 1;
    }

    FILE *fp = fopen(argc[1], "r");
    if (fp == p_null)
    {
        printf("Failed to open grammar file...\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    int char_num = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char*) malloc(sizeof(char) * (char_num + 10));
    fread(buf, 1, char_num, fp);
    buf[char_num] = c_null;
    
    GParser gparser;
    gparser.input = buf;
    ebnf_lexer(&gparser);
    ebnf_parser(&gparser);

    NFA *nfa = (NFA*) malloc(sizeof(NFA) * 100);
    GExpr exprs[10];
    int nfa_num = 0;
    for (int i = 0; i < gparser.def_num; i++)
        if (gparser.defs[i].identity.str[0] == '_' && gparser.defs[i].identity.str[1] != '_')
        {
            build_NFA(gparser.defs[i].identity.expr, nfa + nfa_num);
            exprs[nfa_num] = gparser.defs[i];
            nfa_num++;
        }

    fp = fopen(argc[2], "r");
    char problem[100];
    char answer[100];
    int correct = 0;
    int incorrect = 0;
    while(B_TRUE)
    {
        int i = 0;
        if (!fgets(problem, char_num, fp)) break;
        fgets(answer, char_num, fp);
        problem[strlen(problem) - 1] = c_null;
        answer[strlen(answer) - 1] = c_null;
        for (i = 0; i < nfa_num; i++)
        {
            int result = run_NFA(problem, nfa + i);
            if (result)
            {
                if (strcmp(exprs[i].identity.str, answer) == 0)
                {
                    correct++;
                    break;
                }
                else
                {
                    printf("incorrect! %s is a %s, not %s\n", problem, answer, exprs[i].identity.str);
                    incorrect++;
                    break;
                }
            }
        }
        if (i == nfa_num)
        {
            if (strcmp(answer, "REJECT") == 0)
                correct++;
            else
            {
                printf("incorrect! %s is %s, not invalid\n", problem, answer);
                incorrect++;
            }
        }
    }

    printf("total %d correct out of %d problems\n", correct, correct + incorrect);
    fclose(fp);

    return 0;
}
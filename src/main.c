#include "common.h"
#include "struct.h"
#include "arena.h"
#include "meta.h"
#include "lexer.h"
#include "dump.h"

char *read_file(char *filename)
{
    FILE *fp = fopen(filename, "r");

    fseek(fp, 0, SEEK_END);
    int char_num_ = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char*) malloc(sizeof(char) * (char_num_ + 10));
    fread(buf, 1, char_num_, fp);
    buf[char_num_] = c_null;

    return buf;
}

int main(int argv, char *argc[])
{
    char *buf = read_file(argc[1]);
    Arena arena = init_arena();

    Chunk tokens = meta_lexing(buf, &arena);
    Grammar grammar = meta_parsing(tokens, &arena);

    NFA nfa;
    build_NFA(&grammar, &nfa);

    free(buf);
    buf = read_file(argc[2]);

    Lexer lexer;
    lexer.input = buf;
    lexer.input_len = strlen(buf);
    lexer.nfa = &nfa;
    lexer.arena = &arena;
    lexing(&lexer);

    print_lexing_result(&lexer);

    return 0;
}
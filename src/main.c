#include "common.h"
#include "arena.h"
#include "meta.h"
#include "lexer.h"
#include "dump.h"
#include "parser.h"

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
    char *buf1 = read_file(argc[1]);
    char *buf2 = read_file(argc[2]);

    Arena arena = init_arena();
    Chunk m_tokens = meta_lexing(buf1, &arena);
    Grammar grammar = meta_parsing(m_tokens, &arena);
    FirstFollow ff = solve_ff(&grammar);
    print_ff_tp(&ff, &grammar);

    Chunk tokens = lexing(buf2, &grammar, &arena);
    printf("Lexing done: total %d tokens\n", tokens.used);

    return 0;
}
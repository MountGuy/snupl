#include "common.h"
#include "struct.h"
#include "arena.h"
#include "meta.h"
#include "dump.h"

int main(int argv, char *argc[])
{
    FILE *fp = fopen("snupl2.gm", "r");


    fseek(fp, 0, SEEK_END);
    int char_num = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char*) malloc(sizeof(char) * (char_num + 10));
    fread(buf, 1, char_num, fp);
    buf[char_num] = c_null;

    // printf("%s\n", buf);
    
    Arena arena;
    init_arena(char_num, &arena);

    MetaLexer lexer;
    lexer.arena = &arena;
    lexer.input = buf;
    lexer.input_len = strlen(buf);

    meta_lexing(&lexer);

    MetaParser parser;
    parser.arena = &arena;
    parser.tokens = lexer.tokens;
    parser.token_num = lexer.token_num;

    meta_parsing(&parser);

    return 0;
}
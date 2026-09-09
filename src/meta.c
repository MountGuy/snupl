#include "meta.h"
#include "dump.h"

char operators[11][2] = {
    "(", ")", "{", "}", "[", "]", ";", "=", "|", ",", "~"
};


void meta_lexing(MetaLexer *lexer)
{
    MetaToken *tokens = (MetaToken*) malloc(sizeof(MetaToken) * (lexer->input_len + 10));
    int tok_num = 0;

    char *cursor = lexer->input;
    skip_space(cursor);

    while (cursor[0])
    {
        MetaToken token = { p_null };
        if (cursor[0] == '/' && cursor[1] == '/')
        {
            while (cursor[0] != '\n')
                cursor++;
            cursor++;
        }
        else if (cursor[0] == '0' && cursor[1] == 'x')
        {
            char hex_str[2] = { hex_to_int(cursor[1]) * 16 + hex_to_int(cursor[2]), c_null };
            token.string = add_string(hex_str, 1, lexer->arena);
            token.type = M_STRING;
            cursor += 4;
            print_meta_token(token);
        }
        else if (is_identc(cursor[0]))
        {
            int string_len = 0;
            while (is_identc(cursor[string_len]))
                string_len++;
            token.string = add_string(cursor, string_len, lexer->arena);
            token.type = M_IDENTITY;
            cursor += string_len;
            print_meta_token(token);
        }
        else if (cursor[0] == '\"')
        {
            int string_len = 1;
            cursor++;
            while (cursor[string_len] != '\"')
                string_len++;
            token.string = add_string(cursor, string_len, lexer->arena);
            token.type = M_STRING;
            cursor += string_len + 1;
            print_meta_token(token);
        }
        else
        {
            for (int i = 0; i < 11; i++)
            {
                if (cursor[0] == operators[i][0])
                {
                    cursor++;
                    token.string = operators[i];
                    token.type = M_OPERATOR;
                    print_meta_token(token);
                    break;
                }
            }
        }
        if (token.string == p_null)
        {
            printf("Grammar lexing error...\n");
            exit(1);
        }
        else
        {
            tokens[tok_num] = token;
            tok_num++;
        }

        skip_space(cursor);
    }

    lexer->tokens = tokens;
    lexer->token_num = tok_num;

}


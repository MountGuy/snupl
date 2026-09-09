#include "arena.h"
#include "meta.h"
#include "dump.h"

char meta_operators[11][2] = {
    "(", ")", "{", "}", "[", "]", ";", "=", "|", ",", "~"
};

MetaToken *peek_tok(MetaParser *parser)
{
    return parser->tokens + parser->cursor;
}

MetaToken *peek_next(MetaParser *parser)
{
    return parser->tokens + parser->cursor + 1;
}

MetaToken *advance_parser(MetaParser *parser)
{
    return parser->tokens + parser->cursor++; 
}



void meta_lexing(MetaLexer *lexer)
{
    MetaToken *tokens = (MetaToken*) malloc(sizeof(MetaToken) * (lexer->input_len + 10));
    int tok_num = 0;

    char *cursor = lexer->input, *line_front = lexer->input;
    int line = 1;

    while (cursor[0])
    {
        MetaToken token = { p_null };
        if (cursor[0] == '/' && cursor[1] == '/')
        {
            while (cursor[0] != '\n')
                cursor++;
            cursor++;
            line++;
            line_front = cursor;
        }
        else if (cursor[0] == '0' && cursor[1] == 'x')
        {
            char hex_str[2] = { hex_to_int(cursor[1]) * 16 + hex_to_int(cursor[2]), c_null };
            token.string = add_string(hex_str, 1, lexer->arena);
            token.type = M_STRING;
            token.line = line;
            token.col = cursor - line_front + 1;
            token.len = 4;
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
            token.line = line;
            token.col = cursor - line_front + 1;
            token.len = string_len;
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
            token.line = line;
            token.col = cursor - line_front;
            token.len = string_len + 2;
            cursor += string_len + 1;
            print_meta_token(token);
        }
        else
        {
            for (int i = 0; i < 11; i++)
            {
                if (cursor[0] == meta_operators[i][0])
                {
                    token.string = meta_operators[i];
                    token.type = M_OPERATOR;
                    token.line = line;
                    token.col = cursor - line_front + 1;
                    token.len = 1;
                    cursor++;
                    print_meta_token(token);
                    break;
                }
            }
        }
        if (token.string != p_null)
        {
            tokens[tok_num] = token;
            tok_num++;
        }

        while (*cursor == ' ' || *cursor == '\t' || *cursor == '\n')
        {
            if (*cursor == '\n')
            {
                line++;
                line_front = cursor + 1;
            }
            cursor++;
        }
    }

    lexer->tokens = tokens;
    lexer->token_num = tok_num;

}

void meta_parsing(MetaParser *parser)
{

}

void *parse_define(MetaParser *parser)
{
    return p_null;

}

MetaExpr *parse_alter(MetaParser *parser)
{
    return p_null;
}

MetaExpr *parse_concat(MetaParser *parser)
{
    return p_null;

}

MetaExpr *parse_primary(MetaParser *parser)
{
    return p_null;

}

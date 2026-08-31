#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"
#include "ebnf.h"

#define debug
#define TTOK_C(t) ((t)->string[0])

int indent;

void print_indent()
{
    for (int i = 0; i < indent; i++)
        printf("    ");
}

char *search_asset(Lexer *asset, char *target)
{
    for (int i = 0; i < asset->asset_num; i++)
        if (strcmp(asset->starts[i], target) == 0)
            return asset->starts[i];

    int target_len = strlen(target);

    strcpy(asset->top, target);
    asset->starts[asset->asset_num] = asset->top;
    asset->top += target_len + 1;
    asset->asset_num += 1;
    return asset->starts[asset->asset_num - 1];
}

void init_lexer(char *input, Lexer *lexer)
{
    int char_num = strlen(input);

    lexer->input = input;
    lexer->asset = (char*) malloc(sizeof(char) * char_num);
    lexer->starts = (char**) malloc(sizeof(char*) * char_num);
    lexer->top = lexer->asset;
    lexer->asset_num = 0;

}

void ebnf_lexer(char *input, Lexer *lexer, Token *tokens)
{
    init_lexer(input, lexer);
    
    int tok_num = 0;

    for (char *c = lexer->input; *c; c++)
    {
        if (*c == ' ' || *c == '\n')
        {
            *c = c_null;
            continue;
        }
        else if (*c == ';')
        {
            tokens[tok_num].string = S_END;
            tokens[tok_num].ttype = T_END;
            *c = c_null;
        }
        else if (is_char(*c))
        {
            tokens[tok_num].string = c;
            tokens[tok_num].ttype = T_IDENTITY;
            while (is_char(*(c + 1)) || is_digit(*(c + 1))) c++;
        }
        else if (*c == '\"')
        {
            tokens[tok_num].string = c + 1;
            tokens[tok_num].ttype = T_STRING;
            *c = c_null;
            while (*c != '\"') c++;
            *c = c_null;
        }
        else {
            switch (*c)
            {
                case C_LPAREN:
                tokens[tok_num].string = S_LPAREN;
                break;
                case C_RPAREN:
                tokens[tok_num].string = S_RPAREN;
                break;
                case C_LBRACE:
                tokens[tok_num].string = S_LBRACE;
                break;
                case C_RBRACE:
                tokens[tok_num].string = S_RBRACE;
                break;
                case C_LBRAKET:
                tokens[tok_num].string = S_LBRAKET;
                break;
                case C_RBRAKET:
                tokens[tok_num].string = S_RBRAKET;
                break;
                case C_DEFINE:
                tokens[tok_num].string = S_DEFINE;
                break;
                case C_ALTER:
                tokens[tok_num].string = S_ALTER;
                break;
                case C_CONCAT:
                tokens[tok_num].string = S_CONCAT;
                break;
            }
            *c = c_null;
            tokens[tok_num].ttype = T_OPERATOR;
        }
        tok_num++;
    }

    for (int i = 0; i < tok_num; i++)
    {
        if (tokens[i].ttype == T_IDENTITY || tokens[i].ttype == T_STRING)
        {
            char *stored = search_asset(lexer, tokens[i].string);
            tokens[i].string = stored;
        }
    }

    lexer->tok_num = tok_num;
}

int ebnf_parser(Lexer *lexer, Token *tokens)
{
    Parser parser;
    parser.tokens = tokens;
    parser.tok_num = lexer->tok_num;
    parser.pos = 0;
    parser.exprs = (Expr*) malloc(sizeof(Expr) * (parser.tok_num + 5) * 2);

    while (parser.pos < parser.tok_num)
    {
        parse_def(&parser);
    }

}

Expr *parse_def(Parser *parser)
{
    Token *tok = peek_tok(parser);
    advance_parser(parser);
    Token *tok2 = peek_tok(parser);
    advance_parser(parser);

    if (!(tok->ttype == T_IDENTITY && tok2->ttype == T_OPERATOR && tok2->string == S_DEFINE))
    {
    }

    Expr *curr_expr = parse_alt(parser);
    Token *tok3 = peek_tok(parser);
    if (tok3->string != S_END)
    {
    }
    advance_parser(parser);
    Expr *new_expr = alloc_expr(parser);
    new_expr->kind = E_DEF;
    new_expr->definition.string = tok->string;
    new_expr->definition.expr = curr_expr;

    print_expr(new_expr);
    return new_expr;
}

Expr *parse_alt(Parser *parser)
{
    Expr *curr_expr = parse_con(parser);
    if (parser_end(parser)) return curr_expr;

    Token *tok = peek_tok(parser);
    TType ttype = tok->ttype;

    switch (tok->string[0])
    {
        case C_ALTER:
            advance_parser(parser);
            Expr *next_expr = parse_alt(parser);
            Expr *new_expr = alloc_expr(parser);
            new_expr->kind = E_ALT;
            new_expr->binary.l = curr_expr;
            new_expr->binary.r = next_expr;
            return new_expr;
        case C_RPAREN:
        case C_RBRACE:
        case C_RBRAKET:
        case C_END:
            return curr_expr;
        default:
    }

}

Expr *parse_con(Parser *parser)
{
    Expr *curr_expr = parse_prime(parser);

    Token *tok = peek_tok(parser);
    TType ttype = tok->ttype;

    switch (tok->string[0])
    {
        case C_CONCAT:
            advance_parser(parser);
            Expr *next_expr = parse_con(parser);
            Expr *new_expr = alloc_expr(parser);
            new_expr->kind = E_CON;
            new_expr->binary.l = curr_expr;
            new_expr->binary.r = next_expr;
            return new_expr;
        case C_ALTER:
        case C_RPAREN:
        case C_RBRACE:
        case C_RBRAKET:
        case C_END:
            return curr_expr;
        default:
    }
}

Expr *parse_prime(Parser *parser)
{
    Token *i_token = peek_tok(parser);
    TType ttype = i_token->ttype;
    char *string = i_token->string;

    switch (ttype)
    {
        case T_STRING:
        case T_IDENTITY:
        {
            advance_parser(parser);
            Expr *expr = alloc_expr(parser);
            expr->kind = (ttype == T_STRING? E_LETS: E_IDENT);
            expr->identity.string = string;
            return expr;
        }
        case T_OPERATOR:
        {
            for (int i = 0; i < sizeof(groups) / sizeof(groups[0]); i++)
            {
                if (string[0] == groups[i][0])
                {
                    advance_parser(parser);
                    Expr *curr_expr = parse_alt(parser);
                    Token *f_token = peek_tok(parser);
                    char *f_string = f_token->string;
                    if (f_string[0] == groups[i][1])
                    {
                        advance_parser(parser);
                        Expr *new_expr = alloc_expr(parser);
                        new_expr->kind = groups[i][2];
                        new_expr->unary.expr = curr_expr;
                        return new_expr;
                    }
                }
            }
        }
        default:
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"
#include "ebnf.h"


char *search_asset(Lexer *lexer, char *target)
{
    for (int i = 0; i < lexer->asset_num; i++)
        if (strcmp(lexer->starts[i], target) == 0)
            return lexer->starts[i];

    strcpy(lexer->top, target);
    
    lexer->starts[lexer->asset_num] = lexer->top;
    lexer->top += strlen(target) + 1;
    lexer->asset_num += 1;

    return lexer->starts[lexer->asset_num - 1];
}

void ebnf_lexer(char *input, Lexer *lexer, Token *tokens)
{
    int char_num = strlen(input);
    int tok_num = 0;

    lexer->input = input;
    lexer->asset = (char*) malloc(sizeof(char) * (char_num + 10));
    lexer->starts = (char**) malloc(sizeof(char*) * (char_num + 10));
    lexer->top = lexer->asset;
    lexer->asset_num = 0;    

    for (char *c = lexer->input; *c; c++)
    {
        if (*c == ' ' || *c == '\n')
        {
            *c = c_null;
            continue;
        }
        if (*c == ';')
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
            *c = c_null;
            tokens[tok_num].string = c + 1;
            tokens[tok_num].ttype = T_STRING;
            while (*c != '\"') c++;
            *c = c_null;
        }
        else 
        {
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
    Parser parser = {
        .tokens = tokens,
        .pos = 0,
        .tok_num = lexer->tok_num,
        .exprs = (Expr*) malloc(sizeof(Expr) * (lexer->tok_num + 10)),
        .expr_num = 0,
    };

    while (parser.pos < parser.tok_num)
        parse_define(&parser);

    return 0;
}

Expr *parse_define(Parser *parser)
{
    Token *tok1 = peek_tok(parser); advance_parser(parser);
    Token *tok2 = peek_tok(parser); advance_parser(parser);

    if (!(tok1->ttype == T_IDENTITY && tok2->ttype == T_OPERATOR && tok2->string == S_DEFINE))
    {
        printf("definition format error\n");
        exit(1);
    }

    Expr *curr_expr = parse_alter(parser);
    Token *tok3 = peek_tok(parser);
    if (tok3->string != S_END)
    {
        printf("definition format error\n");
        exit(1);
    }
    advance_parser(parser);
    Expr *new_expr = alloc_expr(parser);
    new_expr->kind = E_DEF;
    new_expr->definition.string = tok1->string;
    new_expr->definition.expr = curr_expr;

    print_expr(new_expr);
    return new_expr;
}

Expr *parse_alter(Parser *parser)
{
    Expr *curr_expr = parse_concat(parser);
    Token *tok = peek_tok(parser);

    switch (tok->string[0])
    {
        case C_ALTER:
            advance_parser(parser);
            Expr *next_expr = parse_alter(parser);
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
            printf("unexpected parsing: parse alter\n");
            exit(1);
    }

}

Expr *parse_concat(Parser *parser)
{
    Expr *curr_expr = parse_primary(parser);
    Token *tok = peek_tok(parser);

    switch (tok->string[0])
    {
        case C_CONCAT:
            advance_parser(parser);
            Expr *next_expr = parse_concat(parser);
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
            printf("unexpected parsing: parse concat\n");
            exit(1);
    }
}

Expr *parse_primary(Parser *parser)
{
    Token *curr_token = peek_tok(parser);
    TType ttype = curr_token->ttype;
    char *string = curr_token->string;

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
                    Expr *curr_expr = parse_alter(parser);
                    Token *next_token = peek_tok(parser);
                    char *next_string = next_token->string;
                    if (next_string[0] == groups[i][1])
                    {
                        advance_parser(parser);
                        Expr *new_expr = alloc_expr(parser);
                        new_expr->kind = groups[i][2];
                        new_expr->unary.expr = curr_expr;
                        return new_expr;
                    }
                }
            }
            printf("unexpected parsing: parse primary\n");
            exit(1);
        }
        default:
        {
            printf("unexpected parsing: parse primary\n");
            exit(1);
        }
    }
}
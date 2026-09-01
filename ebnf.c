#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"
#include "ebnf.h"


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

    print_fexpr(new_expr);
    return new_expr;
}

Expr *parse_alter(Parser *parser)
{
    int expr_num = 0, last_tok = parser->tok_num - parser->pos + 10;
    Expr **exprs = (Expr**) malloc(sizeof(exprs) * last_tok);

    while (true)
    {
        exprs[expr_num] = parse_concat(parser);
        char tok_char = peek_tok(parser)->string[0];
        expr_num++;
        
        if (tok_char == C_END ||
            tok_char == C_RPAREN || tok_char == C_RBRACE || tok_char == C_RBRAKET)
            break;
        if (tok_char == C_ALTER)
        {
            advance_parser(parser);
            continue;
        }
        printf("error\n");
        exit(1);
    }
    if (expr_num == 1)
    {
        Expr *expr = exprs[0];
        free(exprs);
        return expr;
    }
    else
    {
        Expr *expr = alloc_expr(parser);
        expr->kind = E_ALTER;
        expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr_num);
        expr->nary.expr_num = expr_num;
        memcpy(expr->nary.exprs, exprs, sizeof(Expr*) * expr_num);
        free(exprs);
        return expr;
    }
}

Expr *parse_concat(Parser *parser)
{
    int expr_num = 0, last_tok = parser->tok_num - parser->pos + 10;
    Expr **exprs = (Expr**) malloc(sizeof(exprs) * last_tok);

    while (true)
    {
        exprs[expr_num] = parse_primary(parser);
        char tok_char = peek_tok(parser)->string[0];
        expr_num++;

        if (tok_char == C_ALTER || tok_char == C_END ||
            tok_char == C_RPAREN || tok_char == C_RBRACE || tok_char == C_RBRAKET)
            break;
        if (tok_char == C_CONCAT)
        {
            advance_parser(parser);
            continue;
        }
        printf("error\n");
        exit(1);
    }
    if (expr_num == 1)
    {
        Expr *expr = exprs[0];
        free(exprs);
        return expr;
    }
    else
    {
        Expr *expr = alloc_expr(parser);
        expr->kind = E_CONCAT;
        expr->nary.exprs = (Expr**) malloc(sizeof(Expr*) * expr_num);
        expr->nary.expr_num = expr_num;
        memcpy(expr->nary.exprs, exprs, sizeof(Expr*) * expr_num);
        free(exprs);
        return expr;
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
            if (string == S_LPAREN || string == S_LBRACE || string == S_LBRAKET)
            {
                advance_parser(parser);
                Expr *curr_expr = parse_alter(parser);
                Token *next_token = peek_tok(parser);
                char *next_string = next_token->string;

                if (string == S_LPAREN && next_string == S_RPAREN)
                {
                    advance_parser(parser);

                    return curr_expr;
                }
                else if ((string == S_LBRACE && next_string == S_RBRACE)
                      || (string == S_LBRAKET && next_string == S_RBRAKET) )
                {
                    advance_parser(parser);
                    Expr *new_expr = alloc_expr(parser);
                    new_expr->kind = (string == S_LBRACE? E_REP: E_OPT);
                    new_expr->nary.expr_num = 1;
                    new_expr->nary.exprs = (Expr**) malloc(sizeof(Expr*));
                    new_expr->nary.exprs[0] = curr_expr;

                    return new_expr;                    
                }
            }

            printf("1. unexpected parsing: parse primary, %s %s\n", string, peek_tok(parser)->string);
            exit(1);
        }
        default:
        {
            printf("2. unexpected parsing: parse primary\n");
            exit(1);
        }
    }
}
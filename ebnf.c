#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ebnf_util.h"
#include "ebnf.h"

// #define debug
#define TTOK_C(t) ((t)->token[0])

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
        if (*c == ' ' || *c == '\n') continue;
        else if (*c == ';')
        {
            tokens[tok_num].token = S_END;
            tokens[tok_num].ttype = T_END;
            *c = c_null;
        }
        else if (is_char(*c))
        {
            tokens[tok_num].token = c;
            tokens[tok_num].ttype = T_IDENTITY;
            while (is_char(*(c + 1)) || is_digit(*(c + 1))) c++;
        }
        else if (*c == '\"')
        {
            tokens[tok_num].token = c + 1;
            tokens[tok_num].ttype = T_STRING;
            *c = c_null;
            while (*c != '\"') c++;
            *c = c_null;
        }
        else {
            switch (*c)
            {
                case C_LPAREN:
                tokens[tok_num].token = S_LPAREN;
                break;
                case C_RPAREN:
                tokens[tok_num].token = S_RPAREN;
                break;
                case C_LBRACE:
                tokens[tok_num].token = S_LBRACE;
                break;
                case C_RBRACE:
                tokens[tok_num].token = S_RBRACE;
                break;
                case C_LBRAKET:
                tokens[tok_num].token = S_LBRAKET;
                break;
                case C_RBRAKET:
                tokens[tok_num].token = S_RBRAKET;
                break;
                case C_DEFINE:
                tokens[tok_num].token = S_DEFINE;
                break;
                case C_ALTER:
                tokens[tok_num].token = S_ALTER;
                break;
                case C_CONCAT:
                tokens[tok_num].token = S_CONCAT;
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
            char *stored = search_asset(lexer, tokens[i].token);
            tokens[i].token = stored;
        }
    }

    lexer->tok_num = tok_num;
}

int ebnf_parser(Lexer *lexer, Token *tokens)
{

    // printf("\ntokens: ");
    // for (int i = 0; i < tok_num; i++)
    // {
    //     printf("%s ", tokens[i].token);
    // }
    // printf("\n");
    // Parser parser = {0};
    // parser.tokens = tokens + 2;
    // parser.tok_num = tok_num - 2;
    // Expr *expr = parse_alt(&parser);
    // print_expr(expr); printf("\n");
    // return 0;

}

Expr *parse_alt(Parser *parser)
{
    #ifdef debug
    printf("parse alt\n");
    #endif
    Expr *curr_expr = parse_con(parser);
    if (parser_end(parser)) return curr_expr;

    Token *tok = peek_tok(parser);
    TType ttype = tok->ttype;
    char *token = tok->token;


    if (token == S_ALTER)
    {
        advance_parser(parser);
        Expr *next_expr = parse_alt(parser);
        Expr *new_expr = alloc_expr(parser);
        new_expr->kind = E_ALT;
        new_expr->binary.l = curr_expr;
        new_expr->binary.r = next_expr;
        #ifdef debug
        printf("return alt\n");
        #endif
        return new_expr;
    }
    else if (token == S_RPAREN || token == S_RBRACE || token == S_RBRACE || token == S_END)
    {
        #ifdef debug
        printf("return alt\n");
        #endif
        return curr_expr;
    }
    else {
        printf("%d %s error on alt\n", ttype, token);
    }
}

Expr *parse_con(Parser *parser)
{
    #ifdef debug
    printf("parse con\n");
    #endif
    Expr *expr1 = parse_prime(parser);
    Token *tok = peek_tok(parser);

    if (tok->ttype == T_OPERATOR && TTOK_C(tok) == ',')
    {
        advance_parser(parser);
        Expr *expr2 = parse_con(parser);
        Expr *expr = alloc_expr(parser);
        expr->kind = E_CON;
        expr->binary.l = expr1;
        expr->binary.r = expr2;
        #ifdef debug
        printf("return con\n");
        #endif

        return expr;
    }
    else if (TTOK_C(tok) == ')' || TTOK_C(tok) == '}' || TTOK_C(tok) == ']' || TTOK_C(tok) == '|' || tok->ttype == T_END)
    {
        #ifdef debug
        printf("return con\n");
        #endif
        return expr1;
    }
    else printf("%d %c error on con\n", tok->ttype, TTOK_C(tok));
}

Expr *parse_prime(Parser *parser)
{
    #ifdef debug
    printf("parse prime\n");
    #endif
    Token *i_tok = peek_tok(parser);
    // print_parser(parser);

    for (int i = 0; i < sizeof(groups) / sizeof(groups[0]); i++)
    {
        if (i_tok->ttype == T_OPERATOR && TTOK_C(i_tok) == groups[i][0])
        {
            advance_parser(parser);
            Expr *expr = parse_alt(parser);
            Token *f_tok = peek_tok(parser);
            if (f_tok->ttype == T_OPERATOR && TTOK_C(f_tok) == groups[i][1] || f_tok->ttype == T_END)
            {
                advance_parser(parser);
                Expr *expr_new = alloc_expr(parser);
                expr_new->kind = groups[i][2];
                expr_new->unary.expr = expr;
                #ifdef debug
                printf("return prime\n");
                #endif
                return expr_new;
            }
            else
            {
                printf("%d %s %c %c\n", f_tok->ttype, f_tok->token, groups[i][0], groups[i][1]);
                printf("return prime with null\n");
                return p_null;
            }
        }
        if (i_tok->ttype == T_STRING)
        {
            advance_parser(parser);
            Expr *expr = alloc_expr(parser);
            expr->kind = E_LETS;
            strcpy(expr->identity.string, i_tok->token);
            #ifdef debug
            printf("return prime\n");
            #endif
            return expr;
        }
        if (i_tok->ttype == T_IDENTITY)
        {
            advance_parser(parser);
            Expr *expr = alloc_expr(parser);
            expr->kind = E_IDENT;
            strcpy(expr->identity.string, i_tok->token);
            #ifdef debug
            printf("return prime\n");
            #endif
            return expr;
        }
    }
    printf("%d %c error on prime\n", i_tok->ttype, TTOK_C(i_tok));
    exit(1);
}
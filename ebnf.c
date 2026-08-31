#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "ebnf.h"

// #define debug
#define TTOK_C(t) ((t)->token[0])

int ebnf_lexer(char *buf, Token *tokens)
{
    int tok_n = 0;

    for (char *c = buf; *c; c++)
    {
        if (*c == ' ') continue;
        else if (*c == '\n')
        {
            tokens[tok_n].token = ebnf_end;
            tokens[tok_n].ttype = T_END;
            *c = c_null;
        }
        else if (is_char(*c))
        {
            char *start = c;
            while (is_char(*(c + 1)) || is_digit(*(c + 1))) c++;
            strncpy(tokens[tok_n].token, start, c - start + 1);
            tokens[tok_n].token[c - start + 1] = c_null;
            tokens[tok_n].ttype = T_IDENTITY;
            // printf("%s %d %d %d\n", tokens[tok_n].token, tok_n, start - str, c - start + 1);
        }
        else if (*c == '\"')
        {
            char *start = c;
            c++;
            while (*c != '\"') c++;
            strncpy(tokens[tok_n].token, start + 1, c - start - 1);
            tokens[tok_n].token[c - start - 1] = c_null;
            tokens[tok_n].ttype = T_STRING;
            // printf("%s %d %d %d\n", tokens[tok_n].token, tok_n, start + 1 - str, c - start - 1);
        }
        else {
            tokens[tok_n].token[0] = *c;
            tokens[tok_n].token[1] = c_null;
            tokens[tok_n].ttype = T_OPERATOR;
        }
        tok_n++;
    }
    tokens[tok_n].ttype = T_END;

    return tok_n;
}

int ebnf_parser(int tok_num, Token *tokens)
{
    printf("\ntokens: ");
    for (int i = 0; i < tok_num; i++)
    {
        printf("%s ", tokens[i].token);
    }
    printf("\n");
    Parser parser = {0};
    parser.tokens = tokens + 2;
    parser.tok_num = tok_num - 2;
    Expr *expr = parse_alt(&parser);
    print_expr(expr); printf("\n");
    return 0;

}

Expr *parse_alt(Parser *parser)
{
    #ifdef debug
    printf("parse alt\n");
    #endif
    Expr *expr1 = parse_con(parser);
    if (parser_end(parser)) return expr1;

    Token *tok = peek_tok(parser);
    if (tok->ttype == T_OPERATOR && TTOK_C(tok) == '|')
    {
        advance_parser(parser);
        Expr *expr2 = parse_alt(parser);
        Expr *expr = alloc_arena(parser);
        expr->kind = E_ALT;
        expr->alt.l_expr = expr1;
        expr->alt.r_expr = expr2;
        #ifdef debug
        printf("return alt\n");
        #endif
        return expr;
    }
    else if (tok->ttype == T_OPERATOR && TTOK_C(tok) == ',')
    {
        advance_parser(parser);
        Expr *expr2 = parse_con(parser);
        Expr *expr = alloc_arena(parser);
        expr->kind = E_CON;
        expr->con.l_expr = expr1;
        expr->con.r_expr = expr2;
        #ifdef debug
        printf("return alt\n");
        #endif
        return expr;
    }
    else if (TTOK_C(tok) == ')' || TTOK_C(tok) == '}' || TTOK_C(tok) == ']' || tok->ttype == T_END)
    {
        #ifdef debug
        printf("return alt\n");
        #endif
        return expr1;
    }
    else {
        printf("%d %c error on alt\n", tok->ttype, TTOK_C(tok));
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
        Expr *expr = alloc_arena(parser);
        expr->kind = E_CON;
        expr->con.l_expr = expr1;
        expr->con.r_expr = expr2;
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
                Expr *expr_new = alloc_arena(parser);
                expr_new->kind = groups[i][2];
                expr_new->grp.expr = expr;
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
            Expr *expr = alloc_arena(parser);
            expr->kind = E_LETS;
            strcpy(expr->lets.letters.string, i_tok->token);
            #ifdef debug
            printf("return prime\n");
            #endif
            return expr;
        }
        if (i_tok->ttype == T_IDENTITY)
        {
            advance_parser(parser);
            Expr *expr = alloc_arena(parser);
            expr->kind = E_IDENT;
            strcpy(expr->ident.identity.string, i_tok->token);
            #ifdef debug
            printf("return prime\n");
            #endif
            return expr;
        }
    }
    printf("%d %c error on prime\n", i_tok->ttype, TTOK_C(i_tok));
    exit(1);
}
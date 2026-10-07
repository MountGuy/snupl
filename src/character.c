#include "character.h"

#include <stdio.h>
#include <stdlib.h>

int hex_to_int(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    if ('a' <= c && c <= 'f')
        return c - 'a' + 10;
    else
    {
        printf("Unvalid hex character: %c\n", c);
        exit(1);
    }
}

int read_utf8_char(char *s, ulli *result)
{
    const unsigned char *c = (const unsigned char *) s;   // signed char 문제 차단

    if (c[0] < 0x80)
    {
        *result = c[0];
        return 1;
    }
    if ((c[0] & 0xE0) == 0xC0) {
        *result = ((ulli)(c[0] & 0x1F) << 6) | (c[1] & 0x3F);
        return 2;
    }
    if ((c[0] & 0xF0) == 0xE0) {
        *result = ((ulli)(c[0] & 0x0F) << 12) | ((ulli)(c[1] & 0x3F) << 6) | (c[2] & 0x3F);
        return 3;
    }
    if ((c[0] & 0xF8) == 0xF0) {
        *result = ((ulli)(c[0] & 0x07) << 18) | ((ulli)(c[1] & 0x3F) << 12)
                | ((ulli)(c[2] & 0x3F) << 6) | (c[3] & 0x3F);
        return 4;
    }
    printf("invalid UTF-8 lead byte: 0x%02x\n", c[0]);
    exit(1);
}

ulli string_to_ulli(char *c)
{
    ulli result = 0;
    if (c[0] == '0' && c[1] == 'x')
    {
        c += 2;
        while (is_hex(*c))
            result = result * 16 + hex_to_int(*c), c++;
    
        return result;
    }
    else
    {
        read_utf8_char(c, &result);
        return result;
    }
}

int utf8_strlen(char *c)
{
    int length = 0;
    ulli buffer;

    while (*c)
        c += read_utf8_char(c, &buffer), length++;

    return length;
}

int parse_utf8(char *c, ulli *buffer)
{
    int length = 0;

    while (*c)
        c += read_utf8_char(c, buffer + length), length++;

    return length;
}

#include "character.h"

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


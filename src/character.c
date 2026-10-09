#include "character.h"

#define IN(lb, c, ub) (((lb) <= (c)) && ((c) <= (ub)))

uchar utf8_bound[10][9] = {
    { 1, 0x80, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 1, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    { 2, 0xC2, 0xDF, 0x80, 0xBF, 0x00, 0x00, 0x00, 0x00 },
    { 3, 0xE0, 0xE0, 0xA0, 0xBF, 0x80, 0xBF, 0x00, 0x00 },
    { 3, 0xE1, 0xEC, 0x80, 0xBF, 0x80, 0xBF, 0x00, 0x00 },
    { 3, 0xED, 0xED, 0x80, 0x9F, 0x80, 0xBF, 0x00, 0x00 },
    { 3, 0xEE, 0xEF, 0x80, 0xBF, 0x80, 0xBF, 0x00, 0x00 },
    { 4, 0xF0, 0xF0, 0x90, 0xBF, 0x80, 0xBF, 0x80, 0xBF },
    { 4, 0xF1, 0xF3, 0x80, 0xBF, 0x80, 0xBF, 0x80, 0xBF },
    { 4, 0xF4, 0xF4, 0x80, 0x8F, 0x80, 0xBF, 0x80, 0xBF },
};

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
    const unsigned char *c = (const unsigned char *) s;

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

CharClass det_cc(const char *_str)
{
    uchar *str = (uchar*) _str;
    for (CharClass cc = CC_ASCII; cc <= CC_UTF9; cc++)
    {
        int width = utf8_bound[cc][0];
        int det = true;
        for (int i = 0; i < width; i++)
        {
            int lb = utf8_bound[cc][2 * i + 1], ub = utf8_bound[cc][2 * i + 2];
            det = det && IN(lb, str[i], ub);
        }
        if (det && str[width] == c_null)
            return cc;
    }
    printf("neither a valid utf8 encoding nor non ascii 1byte\n");
    exit(1);
}

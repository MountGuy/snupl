#include "stdlib.h"
#include "stdio.h"
#include "string.h"

int main()
{
    char *str1 = "무호랑 히오스 하실분";
    char *str2 = "I am your father";

    printf("length of %s: %d\n", str1, strlen(str1));
    printf("length of %s: %d\n", str2, strlen(str2));

    return 0;

}
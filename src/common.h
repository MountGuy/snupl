#ifndef COMMON_H
#define COMMON_H 1

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define c_null ('\0')
#define p_null NULL
#define flush (fflush(stdout))
#define newline printf("\n")
#define sepline printf("==============================================\n")
#define skip_space(p) while (*(p) == ' ' || *(p) == '\t' || *(p) == '\n') (p)++


#endif
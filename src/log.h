#ifndef LOG_H
#define LOG_H 1

#include "common.h"

/* MetaToken 은 포인터로만 쓰이므로 전방 선언으로 충분하다.
   meta.h 를 포함하면 chunk.h -> log.h -> meta.h -> arena.h -> chunk.h 순환이 생긴다. */
typedef struct MetaToken MetaToken;

void print_error(char *message);
void print_error_mtoken(char *comment, MetaToken *token);

#endif

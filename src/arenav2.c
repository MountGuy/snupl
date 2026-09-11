#include <stdlib.h>
#include <string.h>

#define DEFAULT_MAX 1000
#define PSIZE (sizeof(void*))
#define CSIZE (sizeof(char))

typedef double MetaExpr;

typedef struct {
    void *buffer;
    size_t size, used;
} Buffer;

typedef struct {
    Buffer *strings, string_heads, *m_exprs, *m_expr_lists;
    int sb_len, meb_len, melb_len;
    int sb_max, meb_max, melb_max;
} Arena;


Buffer init_buffer(size_t size)
{
    Buffer buffer = {
        .buffer = malloc(size),
        .used = 0,
        .size = size,
    };

    return buffer;
}

Buffer expand_buffer(Buffer buffer)
{
    Buffer new_buffer = {
        .buffer = malloc(buffer.size * 2),
        .used = buffer.used,
        .size = buffer.size * 2
    };
    memcpy(new_buffer.buffer, buffer.buffer, buffer.size);
    free(buffer.buffer);
    return new_buffer;
}

void init_arena(Arena *arena)
{
    arena->strings = (Buffer*) malloc(sizeof(Buffer) * DEFAULT_MAX);
    arena->m_exprs = (Buffer*) malloc(sizeof(Buffer) * DEFAULT_MAX);
    arena->m_expr_lists = (Buffer*) malloc(sizeof(Buffer) * DEFAULT_MAX);
    
    arena->strings[0] = init_buffer(CSIZE * DEFAULT_MAX);
    arena->string_heads = init_buffer(PSIZE * DEFAULT_MAX);
    arena->m_exprs[0] = init_buffer(sizeof(MetaExpr) * DEFAULT_MAX);
    arena->m_expr_lists[0] = init_buffer(PSIZE * DEFAULT_MAX);

    arena->sb_len = 1;
    arena->meb_len = 1;
    arena->melb_len = 1;

    arena->sb_max = DEFAULT_MAX;
    arena->meb_max = DEFAULT_MAX;
    arena->melb_max = DEFAULT_MAX;
}

char *insert_string(char *string, int string_len, Arena *arena)
{
    Buffer s_buffer = arena->strings[arena->sb_len - 1];

    if (s_buffer.used + CSIZE * (string_len + 1) < s_buffer.size)
    {
        char *return_val = s_buffer.buffer + s_buffer.used;
        memcpy(return_val, string, CSIZE * string_len);
        return_val[string_len] = '\0';
        arena->strings[arena->sb_len - 1].used += CSIZE * (string_len + 1);
        
        return return_val;
    }

    int alloc_len = string_len + 1 > DEFAULT_MAX? string_len + 1 : DEFAULT_MAX;

    if (arena->sb_len == arena->sb_max)
    {
        Buffer *tmp = arena->strings;
        arena->strings = malloc(sizeof(Buffer) * arena->sb_max * 2);
        memcpy(arena->strings, tmp, sizeof(Buffer) * arena->sb_max);
        free(tmp);
        arena->sb_max *= 2;
    }

    s_buffer = init_buffer(alloc_len);
    
    memcpy(s_buffer.buffer, string, CSIZE * string_len);
    ((char*) s_buffer.buffer)[string_len] = '\0';
    s_buffer.used += CSIZE * (string_len + 1);
    arena->strings[arena->sb_len] = s_buffer;
    arena->sb_len++;

    return (char*) s_buffer.buffer;
}

void *insert_string_head(char *string_head, Arena *arena)
{
    Buffer sh_buffer = arena->string_heads;

    if (sh_buffer.used == sh_buffer.size)
        sh_buffer = expand_buffer(sh_buffer);
    *(char**) (sh_buffer.buffer + sh_buffer.used) = string_head;
    sh_buffer.used += PSIZE;

    arena->string_heads = sh_buffer;
}

char *add_string(char *string, int string_len, Arena *arena)
{
    for (int i = 0; i < arena->string_heads.used; i += PSIZE)
    {
        char *str = *(char**)(arena->string_heads.buffer + i);
        if (strncmp(str, string, string_len) == 0 && strlen(str) == string_len)
            return str;
    }

    char *head = insert_string(string, string_len, arena);
    insert_string_head(head, arena);

    return head;
}
#include <stdlib.h>
#include <string.h>

#define DEFAULT_MAX 1000
#define C_NULL ('\0')
#define PSIZE (sizeof(void*))
#define CSIZE (sizeof(char))
#define BSIZE (sizeof(Buffer))
#define MSIZE (sizeof(MetaExpr))
#define BC (Buffer*)
#define CC (char*)
#define VC (void*)

typedef double MetaExpr;

typedef struct {
    void *buffer;
    size_t unit, size, used;
} Buffer;

typedef struct {
    Buffer strings, string_heads;
    int string_num;
} Arena;


Buffer init_buffer(size_t unit, int length)
{
    Buffer buffer = {
        .buffer = malloc(unit * length),
        .unit = unit,
        .size = unit * length,
        .used = 0,
    };

    return buffer;
}

void expand_buffer(Buffer *buffer)
{
    Buffer new_buffer = {
        .buffer = malloc(buffer->size * 2),
        .unit = buffer->unit,
        .used = buffer->used,
        .size = buffer->size * 2
    };
    memcpy(new_buffer.buffer, buffer->buffer, buffer->size);
    free(buffer->buffer);
    *buffer = new_buffer;
}

void *index_buffer(int idx, Buffer *buffer)
{
    return buffer->buffer + buffer->unit * idx;
}

void *index_last(Buffer *buffer)
{
    return buffer->buffer + buffer->used - buffer->unit;
}

void *append_data(void *source, int length, Buffer *buffer)
{
    memcpy(buffer->buffer + buffer->used, source, buffer->unit * length);
    void *return_val = buffer->buffer + buffer->used;
    buffer->used += buffer->unit * length;
    return return_val;
}

void *write_data(void *source, int idx, Buffer *buffer)
{

}

int has_space(int length, Buffer *buffer)
{
    return buffer->size >= buffer->used + length * buffer->unit;
}

void init_arena(Arena *arena)
{    
    arena->strings = init_buffer(BSIZE, DEFAULT_MAX);
    *(BC index_buffer(0, &arena->strings)) = init_buffer(CSIZE, DEFAULT_MAX);
    arena->string_heads = init_buffer(PSIZE, DEFAULT_MAX);

    arena->string_num = 0;
}

char *insert_string(char *string, int string_len, Arena *arena)
{
    int alloc_len = string_len + 1 > DEFAULT_MAX? string_len + 1 : DEFAULT_MAX;
    Buffer *last_buf = BC index_last(&arena->strings);

    if (!has_space(string_len + 1, last_buf))
    {
        if (!has_space(1, &arena->strings))
            expand_buffer(&arena->strings);

        Buffer tmp = init_buffer(CSIZE, alloc_len);
        append_data(VC (&tmp), 1, &arena->strings);
    }

    last_buf = BC index_last(&arena->strings);
    char *head = CC append_data(VC string, string_len + 1, last_buf);
    head[string_len] = C_NULL;

    return head;
}

void insert_string_head(char *string_head, Arena *arena)
{
    Buffer *sh_buffer = &arena->string_heads;
    void *_string_head = VC string_head;

    if (!has_space(1, sh_buffer))
        expand_buffer(sh_buffer);
    append_data(&_string_head, 1, sh_buffer);
}

char *add_string(char *string, int string_len, Arena *arena)
{
    for (int i = 0; i < arena->string_num; i++)
    {
        char *str = *(char**) index_buffer(i, &arena->string_heads);
        if (strncmp(str, string, string_len) == 0 && strlen(str) == string_len)
            return str;
    }

    
    char *head = insert_string(string, string_len, arena);
    insert_string_head(head, arena);
    arena->string_num++;

    return head;
}
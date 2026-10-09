// 렉싱 쓰루풋 벤치마크: 문법은 한 번만 빌드하고 NFA 경로와 DFA 경로로
// 코퍼스를 쭉 훑는다.
//
//========================================================= lexer 쪽 계약
//
//   const char *nfa_scan(const char *input, NFA *nfa);
//   const char *dfa_scan(const char *input, DFA *dfa);
//
// 토큰을 저장하지 않고 입력을 훑고, **멈춘 위치**를 리턴한다.
//   - 끝까지 읽었으면 입력의 NUL 자리 (input + strlen(input))
//   - 렉싱이 실패하면 실패한 바이트의 자리 (exit 하지 않는다)
// 자동기계는 호출자가 미리 만들어 넘긴다. 문법 하나 빌드가 13초라 재사용이 핵심.
//
//========================================================= 사용법
//
//   ./lexbench <문법.gm> [소스파일...]
//   find <디렉토리> -name '*.lean' | ./lexbench <문법.gm>   (인자가 없으면 stdin)
//
// 옵션
//   --repeat N    각 경로를 N 번 돌려 최소 시간을 취한다 (기본 1)
//   --limit N     파일 N 개만 쓴다 (기본 전부)
//   --nfa-limit N NFA 경로는 앞 N 개 파일만 훑는다 (기본 전부).
//                 NFA 는 바이트마다 state x char class 를 전수 훑어서 DFA 보다
//                 네 자리수 느리다. 코퍼스 전체를 NFA 로 돌리면 수십 분이다.
//                 비율은 두 경로가 공통으로 본 집합에서 계산한다.
//   --quiet       실패 샘플 목록을 찍지 않는다
//   --list-failures  끝까지 못 읽은 파일을 "경로<TAB>실패오프셋" 으로 전부 찍고
//                 끝낸다 (원인 분석용. 측정은 안 한다)
//
// 측정에서 제외되는 것
//   - 파일 I/O: 전부 미리 메모리에 올린다
//   - 문법 파싱 / NFA 구축 / DFA 구축: 따로 재서 따로 보고한다
//   - 토큰 저장과 string interning: *_scan 이 애초에 안 한다. 양쪽 경로에
//     똑같이 붙는 비용이고 켜두면 자동기계 차이가 묻힌다 (실측: DFA 패스
//     0.018s -> 저장만 해도 0.039s -> interning 까지 켜면 0.212s).
//     여기서 재는 건 자동기계 스테핑이다.
//
// 측정 대상은 "끝까지 렉싱되는 파일" 뿐이다. 그래야 두 경로가 정확히 같은
// 바이트를 훑는다.

#include "common.h"
#include "arena.h"
#include "chunk.h"
#include "meta.h"
#include "nfa.h"
#include "dfa.h"
#include "lexer.h"

#include <time.h>

#define MAX_SAMPLE 24

typedef struct {
    char *path;
    char *text;
    long bytes;
    long consumed;      // 분류 단계에서 DFA 가 읽어낸 바이트
    int clean;          // 끝까지 읽었는가
} File;

typedef struct {
    char bytes[MAX_SAMPLE + 1];
    int len;
    long count;
} Sample;

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec * 1e-9;
}

static char *read_whole(const char *path, long *len)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
        return p_null;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = malloc(size + 1);
    if (fread(buf, 1, size, fp) != (size_t) size)
    {
        free(buf), fclose(fp);
        return p_null;
    }
    buf[size] = c_null;
    fclose(fp);

    *len = size;
    return buf;
}

//==================================================================== 실패 샘플

static void add_sample(const char *at, Chunk *samples)
{
    int len = 0;
    while (len < MAX_SAMPLE && at[len] && at[len] != '\n')
        len++;

    Sample *data = samples->data;
    for (int i = 0; i < samples->used; i++)
        if (data[i].len == len && memcmp(data[i].bytes, at, len) == 0)
        {
            data[i].count++;
            return;
        }

    if (samples->used >= 4096)
        return;

    Sample s = { .len = len, .count = 1 };
    memcpy(s.bytes, at, len);
    append_data(&s, 1, samples);
}

static int by_count(const void *a, const void *b)
{
    long d = ((const Sample *) b)->count - ((const Sample *) a)->count;
    return (d > 0) - (d < 0);
}

//======================================================================= 한 패스

// 멈춘 자리만 보면 되므로 리턴값 하나로 끝난다. stops 에 파일별 경계를 적어두면
// 두 경로 일치 검사가 공짜가 된다 (파일당 store 한 번, 바이트 루프 밖이다).
#define LEX_PASS(name, scan, automaton_type)                                   \
static long name(automaton_type *m, File *files, int file_num, long *stops)    \
{                                                                              \
    long bytes = 0;                                                            \
                                                                               \
    for (int i = 0; i < file_num; i++)                                         \
    {                                                                          \
        long n = scan(files[i].text, m) - files[i].text;                       \
        stops[i] = n;                                                          \
        bytes += n;                                                            \
    }                                                                          \
                                                                               \
    return bytes;                                                              \
}

LEX_PASS(nfa_pass, nfa_scan, NFA)
LEX_PASS(dfa_pass, dfa_scan, DFA)

//========================================================================= 보고

static void report(const char *label, int files, long bytes, double secs,
                   double build_secs)
{
    double mb = bytes / 1048576.0;
    double s = (secs > 1e-6? secs : 1e-6);      // 타이머 해상도 바닥

    printf("  %-3s  파일 %5d  %7.2f MB  %8.3f s  %9.2f MB/s\n",
           label, files, mb, secs, mb / s);
    printf("       구축 %6.3f s 포함하면 %.2f MB/s\n", build_secs, mb / (s + build_secs));
}

//========================================================================== main

int main(int argc, char *argv[])
{
    int repeat = 1, limit = 0, nfa_limit = 0, quiet = 0, list_failures = 0;
    const char *grammar_path = p_null;

    Chunk paths = init_chunk(sizeof(char *), true);

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--repeat") == 0 && i + 1 < argc)
            repeat = atoi(argv[++i]);
        else if (strcmp(argv[i], "--limit") == 0 && i + 1 < argc)
            limit = atoi(argv[++i]);
        else if (strcmp(argv[i], "--nfa-limit") == 0 && i + 1 < argc)
            nfa_limit = atoi(argv[++i]);
        else if (strcmp(argv[i], "--quiet") == 0)
            quiet = 1;
        else if (strcmp(argv[i], "--list-failures") == 0)
            list_failures = 1;
        else if (!grammar_path)
            grammar_path = argv[i];
        else
        {
            char *p = strdup(argv[i]);
            append_data(&p, 1, &paths);
        }
    }

    if (!grammar_path)
    {
        printf("사용법: lexbench <문법.gm> [소스파일...]   "
               "(파일 인자가 없으면 stdin 에서 한 줄에 하나씩 읽는다)\n");
        return 1;
    }

    if (paths.used == 0)
    {
        char line[4096];
        while (fgets(line, sizeof line, stdin))
        {
            int len = strlen(line);
            while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
                line[--len] = c_null;
            if (len == 0)
                continue;
            char *p = strdup(line);
            append_data(&p, 1, &paths);
        }
    }

    //---------------------------------------------------------- 코퍼스 로드

    char **path_data = paths.data;
    int path_num = (limit > 0 && limit < paths.used? limit : paths.used);

    File *files = malloc(sizeof(File) * (path_num + 1));
    int file_num = 0;
    long total_bytes = 0;

    for (int i = 0; i < path_num; i++)
    {
        long len = 0;
        char *text = read_whole(path_data[i], &len);
        if (!text)
        {
            printf("읽기 실패: %s\n", path_data[i]);
            continue;
        }
        files[file_num++] = (File) { .path = path_data[i], .text = text, .bytes = len };
        total_bytes += len;
    }

    if (file_num == 0)
    {
        printf("읽은 파일이 없다\n");
        return 1;
    }

    printf("코퍼스  파일 %d 개, %.2f MB\n", file_num, total_bytes / 1048576.0);

    //------------------------------------------------------------ 문법 빌드 (1회)

    long gm_len = 0;
    char *gm_text = read_whole(grammar_path, &gm_len);
    if (!gm_text)
    {
        printf("문법 파일을 못 읽었다: %s\n", grammar_path);
        return 1;
    }

    Arena arena = init_arena();

    double t0 = now();
    Chunk m_tokens = meta_lexing(gm_text, &arena);
    Grammar grammar = meta_parsing(m_tokens, &arena);
    double t_meta = now() - t0;

    t0 = now();
    NFA nfa = build_NFA(&grammar);
    double t_nfa = now() - t0;

    t0 = now();
    DFA dfa = build_DFA(&grammar);
    double t_dfa = now() - t0;

    printf("문법    %s -- 규칙 %d, 토큰클래스 %d\n",
           grammar_path, grammar.def_num, grammar.tokc_num);
    printf("구축    메타파싱 %.3f s | NFA %d states / %d classes %.3f s"
           " | DFA %d states %.3f s\n\n",
           t_meta, nfa.state_num, nfa.char_num, t_nfa, dfa.state_num, t_dfa);

    //-------------------------------------- 분류: 끝까지 렉싱되는 파일 (측정 제외)

    Chunk samples = init_chunk(sizeof(Sample), true);
    int clean_num = 0;
    long clean_bytes = 0;

    for (int i = 0; i < file_num; i++)
    {
        const char *stop = dfa_scan(files[i].text, &dfa);

        files[i].consumed = stop - files[i].text;
        files[i].clean = (files[i].consumed == files[i].bytes);

        if (files[i].clean)
            clean_num++, clean_bytes += files[i].bytes;
        else
            add_sample(stop, &samples);
    }

    if (list_failures)
    {
        for (int i = 0; i < file_num; i++)
            if (!files[i].clean)
                printf("%s\t%ld\n", files[i].path, files[i].consumed);
        return 0;
    }

    printf("커버리지  끝까지 렉싱되는 파일 %d / %d (%.1f%%), %.2f MB / %.2f MB (%.1f%%)\n",
           clean_num, file_num, 100.0 * clean_num / file_num,
           clean_bytes / 1048576.0, total_bytes / 1048576.0,
           100.0 * clean_bytes / total_bytes);

    //------------------------------------------------------------- 측정 대상 선별

    File *bench_files = malloc(sizeof(File) * (file_num + 1));
    int bench_num = 0;

    for (int i = 0; i < file_num; i++)
        if (files[i].clean)
            bench_files[bench_num++] = files[i];

    if (bench_num == 0)
    {
        printf("\n측정할 파일이 없다. 문법을 넓혀야 한다.\n");
        goto failures;
    }

    int common_num = (nfa_limit > 0 && nfa_limit < bench_num? nfa_limit : bench_num);

    printf("측정 대상  파일 %d 개 (끝까지 렉싱되는 것만), repeat %d (최소 시간 채택)\n",
           bench_num, repeat);
    if (common_num < bench_num)
        printf("           NFA 는 --nfa-limit 로 앞 %d 개만 (비율은 이 공통 집합에서)\n",
               common_num);
    newline;

    //--------------------------------------------------------------------- 측정

    double nfa_secs = 1e30, dfa_secs = 1e30, dfa_all_secs = 1e30;
    long nfa_bytes = 0, dfa_bytes = 0, dfa_all_bytes = 0;

    long *nfa_stops = malloc(sizeof(long) * bench_num);
    long *dfa_stops = malloc(sizeof(long) * bench_num);

    for (int k = 0; k < repeat; k++)
    {
        t0 = now();
        long b = nfa_pass(&nfa, bench_files, common_num, nfa_stops);
        double s = now() - t0;
        if (s < nfa_secs)
            nfa_secs = s, nfa_bytes = b;

        t0 = now();
        b = dfa_pass(&dfa, bench_files, common_num, dfa_stops);
        s = now() - t0;
        if (s < dfa_secs)
            dfa_secs = s, dfa_bytes = b;

        if (common_num < bench_num)
        {
            t0 = now();
            b = dfa_pass(&dfa, bench_files, bench_num, dfa_stops);
            s = now() - t0;
            if (s < dfa_all_secs)
                dfa_all_secs = s, dfa_all_bytes = b;
        }
    }

    printf("렉싱 쓰루풋 (토큰 저장 / string interning 없음)\n");
    report("NFA", common_num, nfa_bytes, nfa_secs, t_nfa);
    report("DFA", common_num, dfa_bytes, dfa_secs, t_dfa);
    if (common_num < bench_num)
    {
        printf("  -- 코퍼스 전체를 DFA 로만 --\n");
        report("DFA", bench_num, dfa_all_bytes, dfa_all_secs, t_dfa);
    }
    printf("\n  같은 %d 개 파일에서  NFA / DFA = %.1f 배\n",
           common_num, nfa_secs / (dfa_secs > 1e-6? dfa_secs : 1e-6));

    //------------------------------------------- 두 경로 일치 검사 (파일마다 경계)

    printf("\n일치 검사  ");

    int mismatch = 0;
    for (int i = 0; i < common_num; i++)
        if (nfa_stops[i] != dfa_stops[i])
        {
            if (mismatch++ == 0)
                newline;
            printf("  불일치 %s: NFA %ld, DFA %ld\n",
                   bench_files[i].path, nfa_stops[i], dfa_stops[i]);
        }

    if (mismatch == 0)
        printf("파일 %d 개에서 두 경로가 멈춘 자리가 전부 동일 (%ld 바이트)\n",
               common_num, dfa_bytes);
    else
        printf("  불일치 %d 건\n", mismatch);

    //------------------------------------------------------------------ 실패 샘플

failures:
    if (!quiet && samples.used > 0)
    {
        Sample *data = samples.data;
        qsort(data, samples.used, sizeof(Sample), by_count);

        int show = (samples.used < 20? samples.used : 20);
        printf("\n렉싱 실패 지점 상위 %d / %d 종\n", show, samples.used);
        for (int i = 0; i < show; i++)
            printf("  %7ld 회  %.*s\n", data[i].count, data[i].len, data[i].bytes);
    }

    return 0;
}

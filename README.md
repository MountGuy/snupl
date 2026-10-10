# snupl

EBNF 문법 파일을 **입력으로 받는** 스캐너 겸 파서. 문법을 하드코딩하지 않는다.

`.gm` 파일에 적힌 EBNF 를 읽어서

1. 어휘 규칙(`_` 로 시작하는 규칙 + 문법에 등장하는 모든 리터럴) 으로
   **바이트 단위 NFA** 를 Thompson construction 으로 짜고,
2. subset construction 으로 **DFA** (`int trans[state][256]`) 로 변환해서
3. 소스 파일을 maximal munch 로 토큰화한 뒤
4. 나머지 규칙으로 재귀 하향 파싱해서 AST 를 찍는다.

바이트 단위라서 UTF-8 이 그냥 된다. 문법 파일에 멀티바이트 문자를 직접 쓸 수
있고 (`"α" ~ "ω"`), 범위의 양 끝점은 UTF-8 바이트열을 사전식으로 분해해서
NFA 에 올린다. overlong / surrogate / U+10FFFF 초과는 밴드 구조상 애초에
표현되지 않는다.

원래는 서울대 4190.409 SnuPL/2 컴파일러 코스웍을 따라가던 자습 프로젝트고,
지금은 같은 코드베이스를 Lean 4 렉서로 돌리는 쪽으로 쓰고 있다.
남은 작업은 [todo.md](todo.md).

```
src/
  main.c          드라이버
  meta.c/h        .gm 파일의 EBNF 를 읽는 메타 렉서 + 메타 파서
  character.c/h   UTF-8 프리미티브, 코드포인트 ↔ 바이트열, 밴드 분해
  nfa.c/h         Thompson construction, ε-closure, 바이트 단위 NFA
  dfa.c/h         subset construction
  lexer.c/h       maximal munch 토큰화 (NFA 경로 / DFA 경로)
  parser.c/h      FIRST/FOLLOW, nullable 분석, 재귀 하향 파서
  chunk.c/h       unit 를 들고 다니는 가변 길이 컨테이너
  arena.c/h       arena allocator + string interning
  dump.c/h        토큰 / AST / 문법 출력
  bitop.h         비트셋 매크로
  common.h        공용 타입
```

## 구동

```sh
make                                      # -> ./scanner
./scanner <문법.gm> <소스파일>
```

```sh
./scanner grammar.gm test/array01.mod     # SnuPL/2
./scanner test/lean.gm test/lean.lean     # Lean 4
```

토큰 목록과 AST 를 stdout 으로 찍는다.

### make 타깃

| 타깃 | 하는 일 |
|---|---|
| `make` | `./scanner` 빌드 (`.o` 는 `build/`, 헤더 의존성은 `-MMD` 로 자동) |
| `make test` | `grammar.gm` × `test/array01.mod` |
| `make lean` | `test/lean.gm` × `test/lean.lean` |
| `make simple` | `simple.gm` × `simple.mod` |
| `make rtest` | `gen_pair.py` 로 문법+프로그램 쌍을 랜덤 생성해서 돌린다 |
| `make asan` | ASan + UBSan 빌드를 `build-asan/` 에 따로 만든다 → `./scanner-asan` |
| `make check-headers` | 각 헤더가 단독으로 컴파일되는지 (포함 순서 의존 / 순환 포함) |
| `make lexbench` | 렉싱 쓰루풋 벤치마크를 `-O2` 로 빌드 → `./lexbench` |
| `make mathlib` | mathlib4 를 받아서 NFA/DFA 쓰루풋 비교 (약 1분 30초) |
| `make clean` | 빌드 산출물 제거 |

### 벤치마크

```sh
make mathlib                    # mathlib4 를 받아서 NFA vs DFA 쓰루풋 비교
```

문법을 한 번만 빌드하고 코퍼스를 NFA 경로와 DFA 경로로 각각 쭉 훑는다.

```sh
find <디렉토리> -name '*.lean' | ./lexbench <문법.gm> [옵션]
./lexbench <문법.gm> <소스파일...>
```

| 옵션 | 뜻 |
|---|---|
| `--repeat N` | 각 경로를 N 번 돌려 최소 시간을 취한다 |
| `--limit N` | 파일 N 개만 쓴다 |
| `--nfa-limit N` | NFA 경로는 앞 N 개만 (NFA 가 DFA 보다 네 자리수 느리다) |
| `--quiet` | 실패 지점 목록을 찍지 않는다 |

"끝까지 렉싱되는 파일만" 측정한다. 그래야 두 경로가 정확히 같은 바이트를
훑는다. 파일 I/O 와 문법/NFA/DFA 구축은 따로 재서 따로 보고하고, 토큰 저장과
string interning 은 아예 안 한다 (재는 건 자동기계 스테핑이다).
현재 기준선은 [todo.md](todo.md) 끝에 있다.

### 테스트 도구

```sh
python3 gen_tc.py                  # testcase.gm 용 .tc 케이스 생성
python3 gen_pair.py                # t.gm + t.mod 쌍 생성 (--unicode 로 UTF-8 포함)
python3 test/gen_lean_expected.py  # test/lean.expected 재생성 (파이썬 레퍼런스 렉서)
./count.sh                         # .c/.h 줄 수 집계
```

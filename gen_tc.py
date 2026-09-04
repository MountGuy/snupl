#!/usr/bin/env python3
'''
snupl2.gm 의 lexical 규칙(_char, _string, _ident, _number)에 맞는 테스트케이스를
잔뜩 생성한다. 확장자 .tc

출력은 두 줄이 한 쌍이다:
    <입력>
    <기대 결과>

기대 결과는 규칙 이름(_char/_string/_ident/_number) 또는 REJECT.

  __letter     = "A".."Z" | "a".."z" | "_"
  __digit      = "0".."9"
  __hexdigit   = __digit | "A".."F" | "a".."f"
  __hexencoded = "\\x" , __hexdigit , __hexdigit
  __character  = __letter | __digit | "\\n" | "\\t" | """ | "\\'" | "\\\\" | __hexencoded
  _char        = "'" , ( __character | "\\0" ) , "'"
  _string      = """ , { __character } , """
  _ident       = __letter , { __letter | __digit }
  _number      = __digit , { __digit } , [ "L" ]

사용법:
  ./gen_tc.py                       # generated.tc 에 유효 토큰 생성
  ./gen_tc.py -n 500 -o big.tc      # 규칙당 500개
  ./gen_tc.py --invalid -o bad.tc   # 거부되어야 할 입력만 생성
  ./gen_tc.py -r _ident,_number     # 특정 규칙만
  ./gen_tc.py --seed 42             # 재현 가능하게
  ./gen_tc.py --tokens-only         # 정답 줄 없이 입력만
'''

import argparse
import random
import string
import sys

LETTERS = string.ascii_uppercase + string.ascii_lowercase + "_"
DIGITS = string.digits
HEXDIGITS = string.digits + "ABCDEF" + "abcdef"

# __character 의 각 대안을 "소스에 적히는 그대로"의 문자열로 표현한다.
# 이 문법에는 이스케이프 처리가 없으므로 "\n" 은 역슬래시 + n 두 글자다.
ESCAPES = ["\\n", "\\t", "\\'", "\\\\"]

RULES = ["_char", "_string", "_ident", "_number"]
REJECT = "REJECT"


def hexencoded(rng):
    return "\\x" + rng.choice(HEXDIGITS) + rng.choice(HEXDIGITS)


def character(rng, bare_quote=False):
    """__character 하나를 무작위로."""
    kinds = ["letter", "digit", "escape", "hex"]
    if bare_quote:
        kinds.append("quote")
    k = rng.choice(kinds)
    if k == "letter":
        return rng.choice(LETTERS)
    if k == "digit":
        return rng.choice(DIGITS)
    if k == "escape":
        return rng.choice(ESCAPES)
    if k == "quote":
        return '"'
    return hexencoded(rng)


def tag(toks, rule):
    """토큰 목록에 기대 결과를 붙여 (입력, 정답) 쌍으로."""
    return [(t, rule) for t in toks]


# --------------------------------------------------------------------------
# 유효 토큰
# --------------------------------------------------------------------------

def corner_char(bare_quote):
    """_char 의 모든 대안을 최소 한 번씩."""
    out = ["'a'", "'Z'", "'_'", "'0'", "'9'", "'\\0'"]
    out += ["'%s'" % e for e in ESCAPES]
    out += ["'\\x00'", "'\\xFF'", "'\\xff'", "'\\x7f'", "'\\xAb'", "'\\x0A'"]
    if bare_quote:
        out.append("'\"'")
    return out


def corner_string(bare_quote):
    """_string: { } 이므로 빈 문자열도 유효."""
    out = [
        '""',                      # 0글자
        '"a"', '"Z"', '"_"', '"7"',
        '"abc"', '"Hello_World_123"',
        '"\\n"', '"\\t"', '"\\\\"', '"\\\'"',
        '"\\n\\t\\\\"',
        '"\\x41"', '"\\x00\\xff"',
        '"a\\x41b\\nc"',
        '"' + LETTERS + '"',       # 모든 letter
        '"' + DIGITS + '"',
    ]
    if bare_quote:
        out += ['"""', '"a"b"']
    return out


def corner_ident():
    return [
        "a", "Z", "_",             # 최소 길이
        "_a", "a_", "__", "___",
        "a0", "a9", "_0",
        "abc", "ABC", "aBcDeF",
        "_fdsa", "asdf_123", "ADdafWD", "ad1123",
        "x" * 64,
        "_" * 32,
        "a1b2c3d4e5f6g7h8i9j0",
        "L", "Lx", "xL",           # number 의 L 접미사와 헷갈리기 쉬운 것
    ]


def corner_number():
    return [
        "0", "1", "9",             # 한 자리
        "0L", "1L", "9L",
        "00", "007", "0000000",    # 앞자리 0
        "10", "123", "999999999",
        "123L", "007L",
        "2147483647", "2147483648",        # int 경계
        "9223372036854775807L",            # longint 경계
        "1" * 40,                  # 아주 긴 것
        "1" * 40 + "L",
    ]


def random_char(rng, bare_quote):
    if rng.random() < 0.08:
        return "'\\0'"
    return "'" + character(rng, bare_quote) + "'"


def random_string(rng, bare_quote, max_len):
    n = rng.randint(0, max_len)
    return '"' + "".join(character(rng, bare_quote) for _ in range(n)) + '"'


def random_ident(rng, max_len):
    n = rng.randint(0, max_len - 1)
    return rng.choice(LETTERS) + "".join(
        rng.choice(LETTERS + DIGITS) for _ in range(n)
    )


def random_number(rng, max_len):
    n = rng.randint(1, max_len)
    body = "".join(rng.choice(DIGITS) for _ in range(n))
    return body + ("L" if rng.random() < 0.3 else "")


# --------------------------------------------------------------------------
# 거부되어야 할 입력
# --------------------------------------------------------------------------

def invalid_cases():
    return [
        # _char: 내용이 정확히 하나여야 함
        "''",                      # 빈 char
        "'ab'", "'abc'",           # 두 글자 이상
        "'a",                      # 안 닫힘
        "a'",                      # 안 열림
        "'''",                     # 따옴표가 이스케이프 안 됨
        "'\\'",                    # 역슬래시 하나로 끝
        "'\\z'", "'\\q'",          # 정의되지 않은 이스케이프
        "'\\x'", "'\\x4'",         # hexdigit 부족
        "'\\xZZ'", "'\\xG0'",      # hexdigit 아님
        "'\\x412'",                # hexdigit 초과
        "'\\00'",                  # \0 는 그 자체로 하나여야 함

        # _string
        '"abc',                    # 안 닫힘
        'abc"',                    # 안 열림
        '"\\z"', '"\\q"',          # 정의되지 않은 이스케이프
        '"\\x4"', '"\\xZZ"',
        '"\\"',                    # 역슬래시가 닫는 따옴표를 먹음

        # _ident: 숫자로 시작 불가, letter/digit 외 문자 불가
        "1abc", "0x", "9_",
        "ab-c", "a+b", "a.b", "a b", "a\tb",
        "ab$", "#ab", "@x", "ab!",

        # _number: 숫자 뒤 L 은 최대 하나, 끝에만
        "L", "LL",
        "1LL", "12L3", "1L2",
        "0x12", "1.0", "1e5",
        "-1", "+1",                # 부호는 simpleexpr 소관이지 number 가 아님
        "1_000",

        # 어느 규칙에도 안 맞음
        "\\", "\\x41", "$", "`", "~", "?",
    ]


# --------------------------------------------------------------------------

def build(args, rng):
    """(입력, 기대결과) 쌍의 목록을 만든다."""
    cases = []
    want = set(args.rules)

    if args.invalid:
        return tag(invalid_cases(), REJECT)

    if "_char" in want:
        cases += tag(corner_char(args.quote_in_string), "_char")
        cases += tag([random_char(rng, args.quote_in_string)
                      for _ in range(args.n)], "_char")
    if "_string" in want:
        cases += tag(corner_string(args.quote_in_string), "_string")
        cases += tag([random_string(rng, args.quote_in_string, args.max_len)
                      for _ in range(args.n)], "_string")
    if "_ident" in want:
        cases += tag(corner_ident(), "_ident")
        cases += tag([random_ident(rng, args.max_len)
                      for _ in range(args.n)], "_ident")
    if "_number" in want:
        cases += tag(corner_number(), "_number")
        cases += tag([random_number(rng, args.max_len)
                      for _ in range(args.n)], "_number")

    return cases


def main():
    ap = argparse.ArgumentParser(
        description="snupl2 lexical 규칙에 맞는 .tc 테스트케이스 생성기",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("-n", type=int, default=200,
                    help="규칙당 무작위 케이스 개수 (기본 200)")
    ap.add_argument("-o", "--out", default="generated.tc",
                    help="출력 파일, '-' 이면 stdout (기본 generated.tc)")
    ap.add_argument("-r", "--rules", default=",".join(RULES),
                    help="생성할 규칙, 쉼표 구분 (기본 전부)")
    ap.add_argument("--seed", type=int, default=None,
                    help="난수 시드 (재현용)")
    ap.add_argument("--max-len", type=int, default=16,
                    help="무작위 토큰의 최대 길이 (기본 16)")
    ap.add_argument("--invalid", action="store_true",
                    help="유효 토큰 대신 거부되어야 할 입력을 생성")
    ap.add_argument("--quote-in-string", action="store_true",
                    help='__character 의 """ 대안(맨따옴표)을 사용. 문법상 유효하지만 '
                         '문자열 경계가 모호해지므로 기본 꺼짐')
    ap.add_argument("--shuffle", action="store_true",
                    help="케이스 순서를 섞음 (쌍은 유지)")
    ap.add_argument("--tokens-only", action="store_true",
                    help="정답 줄 없이 입력만 출력")
    args = ap.parse_args()

    args.rules = [r.strip() for r in args.rules.split(",") if r.strip()]
    bad = [r for r in args.rules if r not in RULES]
    if bad:
        ap.error("알 수 없는 규칙: %s (가능: %s)" % (", ".join(bad), ", ".join(RULES)))

    rng = random.Random(args.seed)
    cases = build(args, rng)
    if args.shuffle:
        rng.shuffle(cases)

    lines = []
    for tok, expected in cases:
        lines.append(tok)
        if not args.tokens_only:
            lines.append(expected)

    text = "\n".join(lines) + "\n"
    if args.out == "-":
        sys.stdout.write(text)
    else:
        with open(args.out, "w") as f:
            f.write(text)
        kind = "거부 대상" if args.invalid else "유효"
        shape = "입력만" if args.tokens_only else "입력/정답 두 줄씩"
        print("%s → 케이스 %d개, %d줄 (%s, %s, 규칙: %s)"
              % (args.out, len(cases), len(lines), kind, shape,
                 ", ".join(args.rules)),
              file=sys.stderr)


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
'''
문법(.gm)과 그 문법에 맞는 프로그램(.spl)을 한 쌍으로 생성한다.

동작 원리
  1. 임의의 lexical 규칙 트리를 만든다 (리터럴/범위/선택/연접/반복/옵션/참조)
  2. 그 트리를 .gm 텍스트로 렌더링한다
  3. 같은 트리에서 문자열을 샘플링해 .spl 을 만든다
     -> 생성한 프로그램이 생성한 문법에 맞는 것이 구조적으로 보장된다
  4. 같은 트리를 Python re 로도 변환해 레퍼런스 렉서(오라클)를 만들고,
     maximal munch 로 기대 토큰열을 계산해 .expected 에 쓴다

출력
  BASE.gm        문법
  BASE.spl       그 문법에 맞는 토큰열
  BASE.expected  한 줄에 "lexeme<TAB>규칙이름", 기대 결과

사용법
  ./gen_pair.py                        # t.gm / t.spl / t.expected
  ./gen_pair.py -o case01 --seed 7
  ./gen_pair.py --rules 8 --tokens 300 --depth 4
  ./gen_pair.py --no-space             # 토큰 사이 공백 없음 (maximal munch 스트레스)
  ./gen_pair.py --keywords 5           # 키워드 리터럴 규칙도 생성
'''

import argparse
import random
import re
import string
import sys

# .gm 리터럴에 넣어도 안전한 문자.
#   - 공백/탭/개행: .spl 의 토큰 구분자라 제외
#   - '"' : 리터럴을 닫아버려서 제외
#   - '\\' : 이스케이프 혼동을 피하려고 제외
SAFE = (string.ascii_letters + string.digits +
        "!#$%&()*+,-./:;<=>?@[]^_{|}~'")

RANGE_BANDS = [("a", "z"), ("A", "Z"), ("0", "9")]


# --------------------------------------------------------------------------
# 규칙 트리
#   ('lit', s) ('range', lo, hi) ('alt', [n]) ('cat', [n])
#   ('rep', n) ('opt', n) ('ref', name)
# --------------------------------------------------------------------------

def gen_node(rng, depth, refs):
    '''임의의 규칙 본문 하나.'''
    if depth <= 0:
        return gen_leaf(rng, refs)

    kind = rng.choices(
        ["lit", "range", "ref", "alt", "cat", "rep", "opt"],
        weights=[25, 15, 10, 15, 20, 8, 7],
    )[0]

    if kind in ("lit", "range", "ref"):
        return gen_leaf(rng, refs)
    if kind == "alt":
        n = rng.randint(2, 3)
        return ("alt", [gen_node(rng, depth - 1, refs) for _ in range(n)])
    if kind == "cat":
        n = rng.randint(2, 3)
        return ("cat", [gen_node(rng, depth - 1, refs) for _ in range(n)])
    if kind == "rep":
        return ("rep", gen_node(rng, depth - 1, refs))
    return ("opt", gen_node(rng, depth - 1, refs))


def gen_leaf(rng, refs):
    choices = ["lit", "range"]
    if refs:
        choices.append("ref")
    kind = rng.choice(choices)
    if kind == "lit":
        n = rng.randint(1, 3)
        return ("lit", "".join(rng.choice(SAFE) for _ in range(n)))
    if kind == "range":
        lo, hi = rng.choice(RANGE_BANDS)
        a = rng.randint(ord(lo), ord(hi))
        b = rng.randint(a, ord(hi))
        return ("range", chr(a), chr(b))
    return ("ref", rng.choice(refs))


# --------------------------------------------------------------------------

def nullable(node, rules):
    k = node[0]
    if k == "lit":
        return len(node[1]) == 0
    if k == "range":
        return False
    if k in ("rep", "opt"):
        return True
    if k == "alt":
        return any(nullable(c, rules) for c in node[1])
    if k == "cat":
        return all(nullable(c, rules) for c in node[1])
    if k == "ref":
        return nullable(rules[node[1]], rules)
    raise AssertionError(k)


def render(node):
    '''규칙 트리 -> .gm 텍스트.'''
    k = node[0]
    if k == "lit":
        return '"%s"' % node[1]
    if k == "range":
        return '"%s" ~ "%s"' % (node[1], node[2])
    if k == "ref":
        return node[1]
    if k == "alt":
        return "( " + " | ".join(render(c) for c in node[1]) + " )"
    if k == "cat":
        return "( " + " , ".join(render(c) for c in node[1]) + " )"
    if k == "rep":
        return "{ " + render(node[1]) + " }"
    if k == "opt":
        return "[ " + render(node[1]) + " ]"
    raise AssertionError(k)


def to_regex(node, rules):
    '''규칙 트리 -> Python re 패턴 (레퍼런스 의미론).'''
    k = node[0]
    if k == "lit":
        return re.escape(node[1])
    if k == "range":
        return "[%s-%s]" % (re.escape(node[1]), re.escape(node[2]))
    if k == "ref":
        return "(?:%s)" % to_regex(rules[node[1]], rules)
    if k == "alt":
        return "(?:" + "|".join(to_regex(c, rules) for c in node[1]) + ")"
    if k == "cat":
        return "".join("(?:%s)" % to_regex(c, rules) for c in node[1])
    if k == "rep":
        return "(?:%s)*" % to_regex(node[1], rules)
    if k == "opt":
        return "(?:%s)?" % to_regex(node[1], rules)
    raise AssertionError(k)


def sample(node, rules, rng, budget):
    '''규칙 트리에서 문자열 하나를 뽑는다.'''
    if budget[0] <= 0:
        return ""
    budget[0] -= 1
    k = node[0]
    if k == "lit":
        return node[1]
    if k == "range":
        return chr(rng.randint(ord(node[1]), ord(node[2])))
    if k == "ref":
        return sample(rules[node[1]], rules, rng, budget)
    if k == "alt":
        return sample(rng.choice(node[1]), rules, rng, budget)
    if k == "cat":
        return "".join(sample(c, rules, rng, budget) for c in node[1])
    if k == "rep":
        return "".join(sample(node[1], rules, rng, budget)
                       for _ in range(rng.randint(0, 3)))
    if k == "opt":
        return sample(node[1], rules, rng, budget) if rng.random() < 0.5 else ""
    raise AssertionError(k)


# --------------------------------------------------------------------------

def build_grammar(args, rng):
    '''helper(__h*) 와 token(_t*) 규칙을 만든다.'''
    rules = {}          # 이름 -> 트리
    order = []

    for i in range(args.helpers):
        name = "__h%d" % i
        rules[name] = gen_node(rng, args.depth - 1, list(rules))
        order.append(name)

    helpers = [n for n in order]
    tokens = []
    for i in range(args.rules):
        name = "_t%d" % i
        # nullable 이면 토큰이 빈 문자열이 될 수 있으므로 필수 리터럴을 앞에 붙인다
        for _ in range(20):
            body = gen_node(rng, args.depth, helpers)
            if not nullable(body, rules):
                break
        else:
            body = ("lit", rng.choice(SAFE))
        rules[name] = body
        order.append(name)
        tokens.append(name)

    keywords = []
    if args.keywords:
        for _ in range(args.keywords):
            n = rng.randint(2, 6)
            keywords.append("".join(rng.choice(string.ascii_lowercase)
                                    for _ in range(n)))
        keywords = sorted(set(keywords))

    return rules, order, tokens, keywords


def emit_gm(rules, order, keywords):
    lines = []
    for name in order:
        lines.append("%s = %s ;" % (name, render(rules[name])))
    if keywords:
        lines.append("")
        lines.append("keyword = " + " | ".join('"%s"' % k for k in keywords) + " ;")
    return "\n".join(lines) + "\n"


class Oracle:
    '''생성한 규칙들에 대한 레퍼런스 렉서. 최장일치, 동률은 규칙 순서.'''

    def __init__(self, rules, tokens, keywords):
        self.pats = []
        for name in tokens:
            self.pats.append((name, re.compile(to_regex(rules[name], rules))))
        for k in keywords:
            self.pats.append((k, re.compile(re.escape(k))))

    def match_at(self, text, pos):
        best = (0, None)
        for name, pat in self.pats:
            m = pat.match(text, pos)
            if m and m.end() - pos > best[0]:
                best = (m.end() - pos, name)
        return best

    def lex(self, text):
        '''(lexeme, 규칙이름) 목록. 실패하면 예외.'''
        out = []
        pos = 0
        n = len(text)
        while pos < n:
            while pos < n and text[pos] in " \t\n":
                pos += 1
            if pos >= n:
                break
            ln, name = self.match_at(text, pos)
            if ln == 0:
                raise ValueError("오라클이 위치 %d 에서 막힘: %r"
                                 % (pos, text[pos:pos + 20]))
            out.append((text[pos:pos + ln], name))
            pos += ln
        return out


def main():
    ap = argparse.ArgumentParser(
        description=".gm 문법과 그에 맞는 .spl 프로그램을 함께 생성",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("-o", "--out", default="t", help="출력 파일 접두사 (기본 t)")
    ap.add_argument("--seed", type=int, default=None, help="난수 시드")
    ap.add_argument("--rules", type=int, default=5, help="토큰 규칙(_t*) 개수")
    ap.add_argument("--helpers", type=int, default=3, help="헬퍼 규칙(__h*) 개수")
    ap.add_argument("--tokens", type=int, default=120, help=".spl 에 넣을 토큰 수")
    ap.add_argument("--depth", type=int, default=3, help="규칙 트리 최대 깊이")
    ap.add_argument("--no-space", action="store_true",
                    help="토큰 사이 공백 없이 붙임 (maximal munch 스트레스)")
    ap.add_argument("--keywords", type=int, default=0,
                    help="키워드 리터럴 개수 (0 이면 생성 안 함)")
    ap.add_argument("--tries", type=int, default=50,
                    help="오라클과 어긋나면 문법을 다시 뽑는 최대 횟수")
    args = ap.parse_args()

    rng = random.Random(args.seed)

    for attempt in range(args.tries):
        rules, order, tokens, keywords = build_grammar(args, rng)
        oracle = Oracle(rules, tokens, keywords)

        # 토큰 샘플링
        picked = []
        for _ in range(args.tokens):
            name = rng.choice(tokens)
            s = sample(rules[name], rules, rng, [200])
            if s:
                picked.append((s, name))
        if not picked:
            continue

        sep = "" if args.no_space else " "
        text = sep.join(s for s, _ in picked)

        # 오라클로 다시 렉싱해서 기대 결과를 확정한다.
        # (모호한 문법이면 여기서 원래 뽑은 것과 달라질 수 있는데,
        #  그때는 오라클 결과가 정답이다 -- maximal munch 가 그렇게 정의되므로)
        try:
            expected = oracle.lex(text)
        except ValueError:
            continue
        break
    else:
        print("문법 생성 실패: --tries 를 늘리거나 --depth 를 줄여봐", file=sys.stderr)
        return 1

    gm_path = args.out + ".gm"
    spl_path = args.out + ".spl"
    exp_path = args.out + ".expected"

    with open(gm_path, "w") as f:
        f.write(emit_gm(rules, order, keywords))
    with open(spl_path, "w") as f:
        f.write(text + "\n")
    with open(exp_path, "w") as f:
        for lex, name in expected:
            f.write("%s\t%s\n" % (lex, name))

    drift = sum(1 for a, b in zip(picked, expected) if a[0] != b[0])
    print("%s / %s / %s" % (gm_path, spl_path, exp_path), file=sys.stderr)
    print("  규칙 %d(토큰) + %d(헬퍼) + %d(키워드), 문자 %d, 기대 토큰 %d개"
          % (len(tokens), args.helpers, len(keywords), len(text), len(expected)),
          file=sys.stderr)
    if len(expected) != len(picked) or drift:
        print("  주의: 뽑은 토큰 %d개 -> 오라클 재렉싱 %d개 (경계가 재조합됨). "
              "모호한 문법이라는 뜻이고, .expected 가 정답이다."
              % (len(picked), len(expected)), file=sys.stderr)
    return 0


if __name__ == "__main__":
    sys.exit(main())

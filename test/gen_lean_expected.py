#!/usr/bin/env python3
'''
test/lean.gm 의 어휘 규칙에 대한 레퍼런스 렉서.
test/lean.lean 을 토큰화해서 test/lean.expected 를 만든다.

출력 형식 (탭 구분, gen_pair.py 와 동일한 관례)
    lexeme <TAB> 규칙이름 <TAB> 바이트수 <TAB> 코드포인트수 <TAB> 줄 <TAB> 열(코드포인트)

규칙
  - maximal munch (가장 길게 매칭하는 것)
  - 길이가 같으면 키워드/연산자(T_CONST)가 식별자(T_VAR)를 이긴다
  - 공백과 주석은 토큰을 만들지 않고 소비된다
'''

import re
import sys
import os

HERE = os.path.dirname(os.path.abspath(__file__))

KEYWORDS = """def theorem lemma example abbrev structure inductive instance class where
namespace section end open variable universe fun let have show from by calc
match with do if then else return exact intro apply simp rfl sorry
Type Prop List Set Nat""".split()

# 긴 것부터: maximal munch 는 아래에서 길이로 처리하지만 가독성상 정렬해둔다
OPERATORS = [
    "<->", ":=", "=>", "->", "<-", "::", "..", "==", "!=", "<=", ">=",
    "&&", "||", "|>", "<|",
    "(", ")", "[", "]", "{", "}",
    ":", ";", ",", ".", "|", "@", "$", "_",
    "+", "-", "*", "/", "%", "^", "<", ">", "=", "!", "?", "~",
    "→", "←", "↔", "⇒", "↦", "⊢", "≡",
    "∀", "∃", "λ", "Σ", "Π",
    "∧", "∨", "¬", "≠", "≤", "≥",
    "∈", "∉", "⊆", "⊂", "∩", "∪", "∅",
    "×", "∘", "·", "∑", "∏", "√",
    "⟨", "⟩", "⟦", "⟧",
]

# lean.gm 의 __id_begin / __id_rest 와 같은 집합
ID_BEGIN = r"[a-zA-Z_\u03b1-\u03c9\u0391-\u03a1\u03a3-\u03a9\u2100-\u214f\U0001d55c-\U0001d56b]"
ID_REST = (r"[a-zA-Z_0-9'!?\u03b1-\u03c9\u0391-\u03a1\u03a3-\u03a9"
           r"\u2100-\u214f\u2080-\u2089\U0001d55c-\U0001d56b]")

ESCAPE = r'\\(?:[ntr0"\'\\]|x[0-9A-Fa-f]{2})'
STRCHAR = (r'(?:[\x20\x21\x23-\x5b\x5d-\x7e\xa1-\xac\xae-\xff'
           r'\u03b1-\u03c9\u0391-\u03a1\u03a3-\u03a9\u2100-\u214f'
           r'\u2190-\u21ff\u2200-\u22ff\u27e6-\u27eb\U0001d55c-\U0001d56b]|'
           + ESCAPE + r')')
CHARCHAR = (r'(?:[\x20-\x26\x28-\x5b\x5d-\x7e\xa1-\xac\xae-\xff'
            r'\u03b1-\u03c9\u2100-\u214f\u2190-\u21ff\u2200-\u22ff'
            r'\U0001d55c-\U0001d56b]|' + ESCAPE + r')')

# (규칙이름, 패턴, T_CONST 인가)
PATTERNS = [
    ("_comment", re.compile(r"--[^\n]*"), False),
    ("_string", re.compile(r'"' + STRCHAR + r'*"'), False),
    ("_char", re.compile(r"'" + CHARCHAR + r"'"), False),
    ("_ident", re.compile(ID_BEGIN + ID_REST + r"*"), False),
    ("_number", re.compile(r"[0-9]+"), False),
]
PATTERNS += [("keyword", re.compile(re.escape(k)), True) for k in KEYWORDS]
PATTERNS += [("operator", re.compile(re.escape(o)), True) for o in OPERATORS]

SKIP = " \t\r\n"


def lex(text):
    out = []
    pos, line, line_start = 0, 1, 0
    n = len(text)
    while pos < n:
        while pos < n and text[pos] in SKIP:
            if text[pos] == "\n":
                line += 1
                line_start = pos + 1
            pos += 1
        if pos >= n:
            break

        best_len, best_name, best_const = 0, None, False
        for name, pat, is_const in PATTERNS:
            m = pat.match(text, pos)
            if not m:
                continue
            ln = m.end() - pos
            # 더 길면 교체, 같은 길이면 T_CONST 가 이긴다
            if ln > best_len or (ln == best_len and is_const and not best_const):
                best_len, best_name, best_const = ln, name, is_const

        if best_len == 0:
            raise SystemExit("렉싱 실패: %d행 %r" % (line, text[pos:pos + 20]))

        lexeme = text[pos:pos + best_len]
        if best_name != "_comment":          # 주석은 토큰을 만들지 않는다
            out.append((lexeme, best_name, line, pos - line_start + 1))
        pos += best_len
    return out


def main():
    src = os.path.join(HERE, "lean.lean")
    dst = os.path.join(HERE, "lean.expected")
    text = open(src, encoding="utf-8").read()
    toks = lex(text)

    with open(dst, "w", encoding="utf-8") as f:
        for lexeme, name, line, col in toks:
            f.write("%s\t%s\t%d\t%d\t%d\t%d\n"
                    % (lexeme, name, len(lexeme.encode("utf-8")),
                       len(lexeme), line, col))

    dist = {}
    for ch in text:
        if ch not in SKIP:
            w = len(ch.encode("utf-8"))
            dist[w] = dist.get(w, 0) + 1
    multi = [t for t in toks if len(t[0].encode("utf-8")) != len(t[0])]

    print("%s -> 토큰 %d개" % (dst, len(toks)), file=sys.stderr)
    print("  코드포인트 %d / 바이트 %d" % (len(text), len(text.encode("utf-8"))),
          file=sys.stderr)
    print("  UTF-8 바이트폭 분포: "
          + ", ".join("%d바이트 %d자" % (w, dist[w]) for w in sorted(dist)),
          file=sys.stderr)
    print("  바이트수 != 코드포인트수 인 토큰: %d개" % len(multi), file=sys.stderr)


if __name__ == "__main__":
    main()

#!/usr/bin/env bash
# *.c / *.h 파일의 줄 수를 세서 파일별 + 합계로 보여준다.
# 사용법: ./count.sh [디렉토리]   (기본값: 이 스크립트가 있는 디렉토리)

set -u

dir="${1:-$(dirname "$(readlink -f "$0")")}"

if [ ! -d "$dir" ]; then
    echo "디렉토리가 아님: $dir" >&2
    exit 1
fi

rows=$(
    find "$dir" -type f \( -name '*.c' -o -name '*.h' \) -print0 2>/dev/null \
    | xargs -0 -r awk '
        { total[FILENAME]++; if ($0 ~ /[^[:space:]]/) code[FILENAME]++ }
        END { for (f in total) printf "%d\t%d\t%s\n", total[f], code[f]+0, f }
    ' \
    | sort -rn
)

if [ -z "$rows" ]; then
    echo "$dir 아래에 .c / .h 파일이 없음"
    exit 0
fi

printf '%s\n' "$rows" | awk -F'\t' -v base="$dir" '
    # 한글은 UTF-8에서 3바이트인데 화면 폭은 2칸이라 %-*s 패딩이 어긋난다.
    # 라벨은 폭을 직접 알려주고 공백으로 채운다.
    function padlab(s, dw, width,   i, r) {
        r = s
        for (i = dw; i < width; i++) r = r " "
        return r
    }
    { t[NR]=$1; c[NR]=$2; f[NR]=$3; n=NR
      if ($1 > max) max = $1
      sum_t += $1; sum_c += $2
      name = $3
      sub("^" base "/?", "", name)
      nm[NR] = name
      if (length(name) > w) w = length(name)
    }
    END {
        if (w < 12) w = 12
        bar_w = 24

        printf "\n  %s    전체    코드   비율\n", padlab("파일", 4, w)
        line = ""
        for (i = 0; i < w + 26 + bar_w; i++) line = line "-"
        printf "  %s\n", line

        for (i = 1; i <= n; i++) {
            len = (max > 0) ? int(t[i] * bar_w / max + 0.5) : 0
            bar = ""
            for (j = 0; j < len; j++) bar = bar "#"
            printf "  %-*s %7d %7d   %s\n", w, nm[i], t[i], c[i], bar
        }

        printf "  %s\n", line
        printf "  %s %7d %7d   (파일 %d개)\n\n", padlab("합계", 4, w), sum_t, sum_c, n
    }
'

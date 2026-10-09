#!/usr/bin/env bash
# mathlib4 의 Mathlib/ 트리만 받아온다 (벤치마크 코퍼스).
# blob:none + sparse-checkout 으로 히스토리와 나머지 디렉토리는 안 받는다.
set -euo pipefail

here="$(dirname "$(readlink -f "$0")")"
dest="$here/corpus/mathlib4"

if [ -d "$dest/.git" ]; then
    echo "이미 있다: $dest"
else
    mkdir -p "$here/corpus"
    git clone --depth 1 --filter=blob:none --sparse \
        https://github.com/leanprover-community/mathlib4.git "$dest"
    git -C "$dest" sparse-checkout set Mathlib
fi

n=$(find "$dest/Mathlib" -name '*.lean' | wc -l)
b=$(find "$dest/Mathlib" -name '*.lean' -print0 | xargs -0 cat | wc -c)
printf '%s: .lean %s 개, %s 바이트\n' "$dest/Mathlib" "$n" "$b"

#!/usr/bin/env bash
# ---------------------------------------------------------------
# 本地快照：把当前源码打成一个带时间戳的压缩包，存在 .build/snapshots/
#
# 这个工程不做 git 提交（改一次提交一次太麻烦，且访问远端很慢），
# 所以回退只能靠本地快照。脚本只读源码、只写 .build/snapshots/，
# 不修改任何源码或运行目录。
#
# 用法:
#   bash tools/snapshot.sh                # 打一个快照
#   bash tools/snapshot.sh 说明文字        # 快照名带备注
#   bash tools/snapshot.sh --list         # 列出已有快照
#   bash tools/snapshot.sh --prune 10     # 只保留最近 10 个
# ---------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SNAP_DIR="$ROOT/.build/snapshots"
PROJ="$ROOT/RDR2-Bounties-Expansion"

mkdir -p "$SNAP_DIR"

if [ "${1:-}" = "--list" ]; then
  ls -lt "$SNAP_DIR"/*.tar.gz 2>/dev/null | awk '{printf "%8.1f KB  %s\n", $5/1024, $9}' || echo "还没有快照"
  exit 0
fi

if [ "${1:-}" = "--prune" ]; then
  keep="${2:-10}"
  ls -1t "$SNAP_DIR"/src-*.tar.gz 2>/dev/null | tail -n +$((keep+1)) | while read -r f; do
    rm -f "$f" && echo "已删除旧快照: $(basename "$f")"
  done
  exit 0
fi

NOTE="${1:-}"
TS="$(date +%Y%m%d-%H%M%S)"
CLEAN_NOTE="$(printf '%s' "$NOTE" | tr -c '0-9A-Za-z_\-\x80-\xff' '_')"
NAME="src-${TS}${CLEAN_NOTE:+_$CLEAN_NOTE}"
OUT="$SNAP_DIR/$NAME.tar.gz"

cd "$ROOT"
tar -czf "$OUT" \
  --exclude='*.bak-*' --exclude='*.obj' --exclude='*.vcxproj.user' \
  RDR2-Bounties-Expansion/src/src/*.cpp RDR2-Bounties-Expansion/src/src/*.h \
  RDR2-Bounties-Expansion/src/src/*.vcxproj RDR2-Bounties-Expansion/src/src/*.filters \
  RDR2-Bounties-Expansion/lang \
  RDR2-Bounties-Expansion/BountiesExpansion.ini \
  RDR2-Bounties-Expansion/BountiesExpansion.dat \
  tools/build_be.sh tools/snapshot.sh 2>/dev/null

# 清单：便于事后确认快照对应的构建
MAN="$SNAP_DIR/$NAME.manifest.txt"
{
  echo "# snapshot $TS ${NOTE}"
  [ -f "$ROOT/.build/BountiesExpansion.asi" ] && \
    md5sum "$ROOT/.build/BountiesExpansion.asi" | sed "s#$ROOT/##" && \
    grep -a -o "build: [0-9-]* v[0-9.]* step[0-9]* ([^)]*)" "$ROOT/.build/BountiesExpansion.asi" | head -1
  md5sum "$PROJ/lang/Lang.dat" "$PROJ/BountiesExpansion.ini" 2>/dev/null | sed "s#$ROOT/##"
} > "$MAN"

echo "快照: $OUT"
echo "      $(stat -c%s "$OUT") 字节, $(tar -tzf "$OUT" | wc -l) 个文件"
echo "清单: $MAN"
echo
echo "回退到该快照:"
echo "  tar -xzf \"$OUT\" -C \"$ROOT\""

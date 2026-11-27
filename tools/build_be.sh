#!/usr/bin/env bash
# ---------------------------------------------------------------
# RDR2-Bounties-Expansion 便携构建脚本
#   - 所有路径由脚本自身位置推导，工程整体移动到任意盘符/目录都可直接用
#   - 自动通过 vswhere 定位 MSVC 与 Windows SDK，不写死 VS 版本
#   - 编译产物进 <repo>/.build/，不污染源码目录
#
# 用法:
#   bash tools/build_be.sh                        # 编译 + 链接
#   bash tools/build_be.sh -d "D:/Games/RDR2-1"   # 编译后只替换 asi（ini/语言包/进度一律不动）
#   bash tools/build_be.sh -d "..." -D            # 同时用源码里的 ini 与语言包覆盖运行目录
#   bash tools/build_be.sh -c                     # 先清理对象再全量编译
# ---------------------------------------------------------------
set -euo pipefail

# Git Bash 会把 /I /Fo /LIBPATH 之类参数当路径改写，必须关掉
export MSYS_NO_PATHCONV=1
export MSYS2_ARG_CONV_EXCL='*'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJ="$ROOT/RDR2-Bounties-Expansion"
SRC="$PROJ/src/src"
INC="$PROJ/inc"
LIB="$PROJ/lib"
BUILD="$ROOT/.build/be"
OUT="$ROOT/.build/BountiesExpansion.asi"

CLEAN=0
DEPLOY_DIR=""
WITH_DATA=0
while [ $# -gt 0 ]; do
  case "$1" in
    -c|--clean) CLEAN=1; shift ;;
    -d|--deploy) DEPLOY_DIR="${2:-}"; shift 2 ;;
    -D|--with-data) WITH_DATA=1; shift ;;
    *) echo "未知参数: $1"; exit 2 ;;
  esac
done

[ -d "$SRC" ] || { echo "找不到源码目录: $SRC"; exit 1; }
[ -d "$INC/MinHook" ] || { echo "找不到 MinHook: $INC/MinHook"; exit 1; }
[ -f "$LIB/ScriptHookRDR2.lib" ] || { echo "找不到 $LIB/ScriptHookRDR2.lib"; exit 1; }

# 本工程不做 git 提交，回退只能靠本地快照：每次编译前自动存一份。
# 关掉：BE_NO_SNAPSHOT=1 bash tools/build_be.sh
if [ "${BE_NO_SNAPSHOT:-0}" != "1" ] && [ -x "$SCRIPT_DIR/snapshot.sh" -o -f "$SCRIPT_DIR/snapshot.sh" ]; then
  bash "$SCRIPT_DIR/snapshot.sh" "before-build" >/dev/null 2>&1 || true
fi

# ---------- 定位 MSVC ----------
VSWHERE="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
[ -x "$VSWHERE" ] || { echo "找不到 vswhere.exe，请确认已安装 Visual Studio (C++ 桌面开发)"; exit 1; }

VSPATH="$("$VSWHERE" -latest -products '*' \
  -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 \
  -property installationPath | head -1 | tr -d '\r')"
[ -n "$VSPATH" ] || { echo "未找到带 C++ 工具集的 VS 安装"; exit 1; }
VSPATH_UNIX="$(cygpath -u "$VSPATH")"

MSVC_ROOT="$VSPATH_UNIX/VC/Tools/MSVC"
MSVC_VER="$(ls -1 "$MSVC_ROOT" | sort -V | tail -1)"
# 需要与线上产物保持一致时可指定版本，例如 BE_MSVC_VER=14.44.35207 bash tools/build_be.sh
if [ -n "${BE_MSVC_VER:-}" ]; then MSVC_VER="$BE_MSVC_VER"; fi
[ -n "$MSVC_VER" ] || { echo "MSVC 工具集目录为空: $MSVC_ROOT"; exit 1; }
MSVC="$MSVC_ROOT/$MSVC_VER"

SDK_ROOT="/c/Program Files (x86)/Windows Kits/10"
SDK_VER="$(ls -1 "$SDK_ROOT/Include" | grep -E '^10\.' | sort -V | tail -1)"
[ -n "$SDK_VER" ] || { echo "找不到 Windows 10/11 SDK"; exit 1; }
SDK_INC="$SDK_ROOT/Include/$SDK_VER"
SDK_LIB="$SDK_ROOT/Lib/$SDK_VER"

CL="$MSVC/bin/Hostx64/x64/cl.exe"
LINK="$MSVC/bin/Hostx64/x64/link.exe"
[ -x "$CL" ] && [ -x "$LINK" ] || { echo "找不到 x64 编译/链接器: $MSVC/bin/Hostx64/x64"; exit 1; }

# 传给 cl/link 的路径一律用 Windows 形式（C:/...），否则 MSVC 认不出 /c/... 或 /d/...
W_MSVC="$(cygpath -m "$MSVC")"
W_SDKINC="$(cygpath -m "$SDK_INC")"
W_SDKLIB="$(cygpath -m "$SDK_LIB")"
W_SRC="$(cygpath -m "$SRC")"
W_INC="$(cygpath -m "$INC")"
W_LIB="$(cygpath -m "$LIB")"
W_BUILD="$(cygpath -m "$BUILD")"
W_OUT="$(cygpath -m "$OUT")"

INCS=( "/I$W_MSVC/include" "/I$W_SDKINC/ucrt" "/I$W_SDKINC/um"
       "/I$W_SDKINC/shared" "/I$W_SDKINC/winrt" "/I$W_SRC" "/I$W_INC" )
LIBS=( "/LIBPATH:$W_MSVC/lib/x64" "/LIBPATH:$W_SDKLIB/ucrt/x64" "/LIBPATH:$W_SDKLIB/um/x64" )

echo "---------------------------------------------"
echo "工程根目录 : $ROOT"
echo "MSVC       : $MSVC_VER"
echo "Windows SDK: $SDK_VER"
echo "产出       : $OUT"
echo "---------------------------------------------"

[ "$CLEAN" = "1" ] && rm -rf "$BUILD"
mkdir -p "$BUILD"

LOG="$BUILD/compile.log"
: > "$LOG"

# 单个文件编译；工作目录固定为 src/src，
# 因为 Main.h 里 #pragma comment(lib,"../../inc/MinHook/...") 依赖相对路径
# 依赖判断用的时间基准：任一 .h 变了，所有对象都该重编（工程里没有 pch）
NEWEST_H="$(ls -t "$SRC"/*.h 2>/dev/null | head -1)"

compile_one() {
  local f="$1"
  local name obj
  name="$(basename "$f" .cpp)"
  obj="$(cygpath -m "$BUILD/$name.obj")"
  if [ -f "$BUILD/$name.obj" ] && [ "$BUILD/$name.obj" -nt "$f" ] \
     && { [ -z "$NEWEST_H" ] || [ "$BUILD/$name.obj" -nt "$NEWEST_H" ]; }; then
    return 0
  fi
  cd "$SRC" || return 1
  # 标志与旧机 build_be.sh（C:/Temp）保持一致：/O2 /MT /fp:fast /EHsc /W3 /utf-8
  # /EHsc 不能省：ModProgress::parseToCache 用了 try/catch，缺它时栈展开语义不同。
  "$CL" /nologo /c /utf-8 /O2 /MT /fp:fast /EHsc /W3 "/Fo$obj" \
    /DWIN32 /DNDEBUG /D_WINDOWS /D_USRDLL /DNativeTrainer_EXPORTS \
    "${INCS[@]}" "$(basename "$f")" >> "$LOG" 2>&1
}

# 两个历史遗留文件（Hooking.cpp / Model.cpp）不在 BountiesExpansion.vcxproj 的
# ClCompile 列表里，且单独编译会报未定义标识符，属于其它 mod 的残留，必须排除。
EXCLUDE="Hooking Model"

for f in "$SRC"/*.cpp; do
  base="$(basename "$f" .cpp)"
  case " $EXCLUDE " in *" $base "*) continue ;; esac
  compile_one "$f" &
  while [ "$(jobs -rp | wc -l)" -ge 8 ]; do wait -n || true; done
done
wait || true

if grep -aqE "error C[0-9]{4}|fatal error" "$LOG"; then
  echo "编译失败，错误摘要："
  grep -aE "error C[0-9]{4}|fatal error" "$LOG" | head -40
  exit 1
fi

# 静默失败兜底：源文件存在但对象没生成，同样视为失败
MISSING=0
for f in "$SRC"/*.cpp; do
  base="$(basename "$f" .cpp)"
  case " $EXCLUDE " in *" $base "*) continue ;; esac
  if [ ! -f "$BUILD/$base.obj" ]; then echo "缺少对象: $base.obj"; MISSING=$((MISSING+1)); fi
done
[ "$MISSING" = "0" ] || { echo "有 $MISSING 个对象未生成，编译不完整"; exit 1; }

echo "编译完成，对象数: $(ls -1 "$BUILD"/*.obj 2>/dev/null | wc -l)"

# ---------- 链接 ----------
cd "$SRC"
"$LINK" /nologo /DLL /SUBSYSTEM:WINDOWS /OPT:REF /OPT:ICF /INCREMENTAL:NO \
  "/OUT:$W_OUT" "${LIBS[@]}" "/LIBPATH:$W_LIB" \
  "$W_BUILD"/*.obj "$W_LIB/ScriptHookRDR2.lib" >> "$LOG" 2>&1 || {
    echo "链接失败，错误摘要："
    grep -aE "error LNK[0-9]+|fatal error" "$LOG" | head -40
    exit 1
  }

echo "链接成功: $OUT ($(stat -c%s "$OUT") 字节)"

# ---------- 可选部署 ----------
# 默认只替换 asi。ini / Lang*.dat 属于「玩家当前正在用的运行时配置」，
# 游戏目录里那份往往比源码新（改过参数、补过键），无条件覆盖会把改动冲掉；
# BountiesExpansion.dat 是存档进度，任何情况下都不覆盖。
if [ -n "$DEPLOY_DIR" ]; then
  [ -d "$DEPLOY_DIR" ] || { echo "部署目录不存在: $DEPLOY_DIR"; exit 1; }
  STAMP="$(date +%Y%m%d-%H%M%S)"
  if [ -f "$DEPLOY_DIR/BountiesExpansion.asi" ]; then
    cp "$DEPLOY_DIR/BountiesExpansion.asi" "$DEPLOY_DIR/BountiesExpansion.asi.bak-$STAMP"
  fi
  cp "$OUT" "$DEPLOY_DIR/BountiesExpansion.asi"
  echo "已部署 asi: $DEPLOY_DIR/BountiesExpansion.asi"

  if [ "$WITH_DATA" = "1" ]; then
    DATA_DIR="$DEPLOY_DIR/BountiesExpansion"
    mkdir -p "$DATA_DIR"
    for f in BountiesExpansion.ini; do
      if [ -f "$DATA_DIR/$f" ]; then cp "$DATA_DIR/$f" "$DATA_DIR/$f.bak-$STAMP"; fi
      cp "$PROJ/$f" "$DATA_DIR/$f"
    done
    for f in Lang.dat Lang.en.dat; do
      if [ -f "$PROJ/lang/$f" ]; then
        if [ -f "$DATA_DIR/$f" ]; then cp "$DATA_DIR/$f" "$DATA_DIR/$f.bak-$STAMP"; fi
        cp "$PROJ/lang/$f" "$DATA_DIR/$f"
      fi
    done
    echo "已同步 ini/语言包: $DATA_DIR（进度文件未改动）"
  else
    echo "提示：ini 与语言包未改动；确需覆盖请加 -D"
  fi
fi

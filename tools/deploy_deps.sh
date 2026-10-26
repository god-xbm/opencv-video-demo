#!/bin/bash
# ============================================================================
#  tools/deploy_deps.sh —— 把 exe 变成"双击就能跑"的发布包
#
#  为什么需要它？
#  MSYS2 编译出来的 exe 是动态链接的，运行时要依赖 C:\msys64\ucrt64\bin 下
#  几十个 DLL（OpenCV 的 57 个模块 + ffmpeg + Qt6 + openblas + tbb ...）。
#  直接把这个 exe 发给别人（或拷到没装 MSYS2 的电脑上），一定会报
#  "找不到 libopencv_core-500.dll"。
#
#  这个脚本做三件事：
#    1. 用 ldd 递归收集所有依赖的 DLL
#    2. 把它们连同 exe 一起拷到 dist/
#    3. 额外拷入 Qt 的平台插件（少了 qwindows.dll，窗口会报
#       "could not find or load the Qt platform plugin windows"）
#
#  用法（在工程根目录、MSYS2 UCRT64 终端里）：
#      bash tools/deploy_deps.sh
#      bash tools/deploy_deps.sh build/cv_hw.exe dist
# ============================================================================
set -u

EXE="${1:-build/cv_hw.exe}"
DEST="${2:-dist}"
MINGW_ROOT="/ucrt64"

if [ ! -f "$EXE" ]; then
    echo "错误：找不到 $EXE"
    echo "      请先构建：cmake --build build"
    exit 1
fi

echo "源程序 : $EXE"
echo "输出到 : $DEST"
mkdir -p "$DEST"
cp -f "$EXE" "$DEST/"

# ---------------------------------------------------------------------------
#  第 1 步：递归收集依赖
#
#  算法是"滚雪球"：每一轮只对上一轮**新增**的 DLL 跑 ldd，
#  而不是每轮都重扫全部文件。
#
#  这一点很关键：如果每轮都扫全部（几百个文件 × 十几轮），
#  在 Windows 上意味着几千次进程启动，会慢到几分钟。
#  只扫新增的话，总次数约等于依赖总数（100 多个），几十秒即可。
# ---------------------------------------------------------------------------
ALL_FILE="$(mktemp)"      # 累计的全部文件
FRONT_FILE="$(mktemp)"    # 本轮待扫描的 frontier
NEW_FILE="$(mktemp)"      # 本轮扫描结果

printf '%s\n' "$EXE" > "$ALL_FILE"
cp "$ALL_FILE" "$FRONT_FILE"

for round in 1 2 3 4 5 6 7 8; do
    : > "$NEW_FILE"

    while IFS= read -r target; do
        [ -f "$target" ] || continue
        # ldd 输出格式：  name => /path/to/xxx.dll (0x...)
        # 第三列才是真实路径；系统 DLL 的路径形如 /c/WINDOWS/system32/...，
        # 后面拷贝时会按前缀过滤掉。
        ldd "$target" 2>/dev/null | awk '$3 ~ /^\//{print $3}' >> "$NEW_FILE"
    done < "$FRONT_FILE"

    sort -u "$NEW_FILE" -o "$NEW_FILE"
    sort -u "$ALL_FILE" -o "$ALL_FILE"

    # 只保留"还没在 ALL 里出现过"的，作为下一轮的 frontier
    comm -13 "$ALL_FILE" "$NEW_FILE" > "$FRONT_FILE"

    if [ ! -s "$FRONT_FILE" ]; then
        echo "依赖解析完成（共 $round 轮）"
        break
    fi

    echo "  第 $round 轮：新增 $(wc -l < "$FRONT_FILE") 个依赖"
    cat "$FRONT_FILE" >> "$ALL_FILE"
    sort -u "$ALL_FILE" -o "$ALL_FILE"
done

# ---------------------------------------------------------------------------
#  第 2 步：按前缀过滤后拷贝
#
#  只拷 /ucrt64 和 /usr/bin 下的（那是 MSYS2 提供的运行时）；
#  /c/WINDOWS/system32 之类的系统 DLL 由 Windows 自带，不要拷。
# ---------------------------------------------------------------------------
dep_count=0
while IFS= read -r dll; do
    [ -f "$dll" ] || continue
    case "$dll" in
        ${MINGW_ROOT}/*|/usr/bin/*)
            if cp -n "$dll" "$DEST/" 2>/dev/null; then
                dep_count=$((dep_count + 1))
            fi
            ;;
        *) : ;;
    esac
done < "$ALL_FILE"

rm -f "$ALL_FILE" "$FRONT_FILE" "$NEW_FILE"
echo "已拷贝依赖 DLL：$dep_count 个"

# ---------------------------------------------------------------------------
#  第 3 步：Qt 平台插件
#
#  Qt 的窗口系统不是编译进 exe 的，而是运行时按插件找。
#  win32 平台插件就是 platforms/qwindows.dll，缺了就会报：
#      This application failed to start because no Qt platform plugin could be initialized
#  所以必须连目录一起拷，并保持 platforms/ 这个目录名不变。
# ---------------------------------------------------------------------------
QT_PLUGINS="${MINGW_ROOT}/share/qt6/plugins"
if [ -d "$QT_PLUGINS/platforms" ]; then
    mkdir -p "$DEST/platforms"
    cp -f "$QT_PLUGINS/platforms/"*.dll "$DEST/platforms/" 2>/dev/null
    echo "已拷贝 Qt 平台插件 → $DEST/platforms/"
else
    echo "提示：未找到 Qt 插件目录（$QT_PLUGINS）；如果不需要窗口可以忽略。"
fi

if [ -d "$QT_PLUGINS/imageformats" ]; then
    mkdir -p "$DEST/imageformats"
    cp -f "$QT_PLUGINS/imageformats/"*.dll "$DEST/imageformats/" 2>/dev/null
fi

# ---------------------------------------------------------------------------
#  第 4 步：素材与说明
# ---------------------------------------------------------------------------
if [ -d "assets" ]; then
    mkdir -p "$DEST/assets"
    for f in assets/*.png assets/*.jpg assets/*.mp4 assets/*.avi; do
        [ -f "$f" ] && cp -f "$f" "$DEST/assets/" 2>/dev/null
    done
    echo "已拷贝素材 → $DEST/assets/"
fi

[ -f "README.md" ] && cp -f README.md "$DEST/"
mkdir -p "$DEST/output"

# 双击即用的启动器（内容全是 ASCII，中文只出现在文件名上，cmd 解析不受影响）
if [ -d "launchers" ]; then
    for f in launchers/*.bat; do
        [ -f "$f" ] && cp -f "$f" "$DEST/" 2>/dev/null
    done
    echo "已拷贝启动器 → $DEST/*.bat"
fi

echo ""
echo "完成。发布包在：$DEST"
echo "验证方式（清空 PATH 模拟没装 MSYS2 的电脑）："
echo "    cd $DEST && PATH=/usr/bin:/bin ./cv_hw.exe img assets/calib_bgr.png"

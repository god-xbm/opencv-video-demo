# 第一周综合练习作业 —— 视觉素材查看与标注程序（cv_hw）

用 **CMake** 管理、**C++17 + OpenCV** 实现的命令行程序，含图片模式与视频模式，能把处理结果（带帧号/分辨率/FPS 叠加）保存到 `output/` 目录。

一句话验收：**双击 `dist\` 里任一 `.bat`，或按本文「编译」「运行」两节从零跑通。**

---

## 环境

| 项目 | 本机实际版本 |
|---|---|
| 操作系统 | Windows 11（`MINGW64_NT-10.0-26200`） |
| g++ 版本 | **16.2.0**（MSYS2 UCRT64，`g++.exe (Rev6, Built by MSYS2 project)`） |
| CMake 版本 | **4.4.4**（生成器 Ninja 1.13.2，C++17） |
| OpenCV 版本 | **5.0.0**（MSYS2 包 `mingw-w64-ucrt-x86_64-opencv 5.0.0-7`） |
| 窗口后端 | **Qt 6.12.0**（`qt6-base` + `qt6-5compat`，OpenCV 内部构建信息 `GUI: QT6`） |
| 视频编解码 | FFmpeg（OpenCV 内置，avcodec / avformat 63.1.102，含 libx264 / xvidcore / x265 / libvpx） |
| 工具链根目录 | `D:/msys64`（对应终端里的 `/d/msys64`） |

> **说明**：作业示例环境写的是 Ubuntu 22.04 + `make`，本机实际是 **Windows 11 + MSYS2**，
> 所以下文的命令按 Windows/MSYS2 写。`CMakeLists.txt` 里没有写死任何路径或编译器，
> 换到 Linux 上把 `-G Ninja` 换成默认生成器即可，源码一行都不用改（见「编译」最后一小节）。

### 环境准备（一条命令装齐）

打开开始菜单的 **`MSYS2 UCRT64`**，执行：

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-opencv \
          mingw-w64-ucrt-x86_64-qt6-base \
          mingw-w64-ucrt-x86_64-qt6-5compat
```

**`qt6-5compat` 必须显式装**——它在 OpenCV 包里只是 optdepends（可选依赖），pacman 不会自动带。
少了它的现象极其隐蔽：**编译顺利通过 → 运行不报任何错 → 但窗口根本不出来**。

### 为什么选这套环境

本机原本只有 Dev-C++ 自带的 TDM-GCC 4.9.2（2014 年）：没有 `<filesystem>`、不支持 CMake 工程、
也链接不了 OpenCV 5.x；而 OpenCV 官方的 Windows 预编译包是 **MSVC ABI**，MinGW 无法直接链接。
所以选 MSYS2——用 pacman 一条命令装齐「现代 GCC + CMake + Ninja + 预编译 OpenCV」，
且 OpenCV 与编译器出自同一套工具链，不存在 ABI 不匹配问题。

---

## 编译

### 前置：必须在 UCRT64 环境里操作

用开始菜单的 **`MSYS2 UCRT64`** 终端（或其它终端里先 `export MSYSTEM=UCRT64`）。
这样 `PATH` 中才有 `D:/msys64/ucrt64/bin`，CMake 才能找到 OpenCV，运行时也才能找到 DLL。

### 步骤

```bash
cd /c/Users/Lenovo/Desktop/xieboming_homework_1/第一周作业/作业

# 1) 配置（Release 必须显式指定：Ninja 单配置生成器不指定就没有优化，视频帧率会低到误以为代码有问题）
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2) 构建主程序
cmake --build build --parallel

# 3) 构建两个辅助工具（可选，但建议跑，第 0 步环境自检要用）
cmake --build build --target env_check
cmake --build build --target make_assets
```

### 与作业示例命令的对应关系

| 示例（Ubuntu） | 本机（Windows + MSYS2） | 说明 |
|---|---|---|
| `cmake -S . -B build` | `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release` | 显式选生成器与构建类型 |
| `cmake --build build --parallel "$(nproc)"` | `cmake --build build --parallel` | Ninja 默认已按 CPU 核数并行，无需手算 `nproc` |

也可以用固化的预设一键构建（VS Code 的 CMake Tools 会自动识别 `CMakePresets.json`）：

```bash
cmake --preset ucrt64-release && cmake --build --preset release
```

### 构建产物

| 文件 | 作用 |
|---|---|
| `build/cv_hw.exe` | **主程序**（图片模式 / 视频模式） |
| `build/env_check.exe` | 环境自检：版本 / GUI 后端 / 视频编解码 / 弹窗测试 |
| `build/make_assets.exe` | 生成测试素材（三色块校准图、彩条图、运动视频） |

`-Wall -Wextra` 下 **11 个编译单元零错误零警告**。

### 如果 CMake 报找不到 OpenCV

```text
Could not find a package configuration file provided by "OpenCV" ...
```

手动指定 OpenCV 的 config 目录即可：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DOpenCV_DIR=D:/msys64/ucrt64/lib/cmake/opencv5
```

### 换到 Linux 或 Visual Studio 构建

`CMakeLists.txt` 不依赖任何 MSYS2 特有路径：

- **Linux（Ubuntu 22.04）**：装好 `libopencv-dev`、`g++`、`cmake` 后，直接
  `cmake -S . -B build && cmake --build build --parallel "$(nproc)"`
- **Visual Studio + OpenCV 4.x**（国内机房最常见）：
  `cmake -S . -B build -DOpenCV_DIR=C:/opencv/build && cmake --build build --config Release`

源码一行都不用改。`find_package(OpenCV ...)` 故意不写版本号，4.x 与 5.x 都能匹配（原因见「调试问题」第 3 条）。

---

## 运行

### 第 0 步：环境自检（强烈建议先跑）

```bash
./build/env_check.exe
```

预期输出：`OpenCV 版本 : 5.0.0`、`GUI: QT6`、`QT: YES`、`FFMPEG: YES`，
并且**弹出一个写着 `GUI OK` 的绿色窗口**，按任意键关闭。窗口不出来 → 见「调试问题」第 1 条。

### 第 1 步：图片模式

```bash
./build/cv_hw.exe img assets/calib_bgr.png    # 三色块校准图，用来验证"点哪读哪"
./build/cv_hw.exe img assets/photo1.jpg       # 3840x2400 真实照片，测大图
```

操作：控制台打印 **宽 / 高 / 通道数**；**鼠标左键点击画面**打印该点坐标与 BGR 值；**按任意键退出**。

### 第 2 步：视频模式

```bash
./build/cv_hw.exe video assets/motion.mp4     # 1280x720@30fps 6s
./build/cv_hw.exe video assets/motion2.avi    # 640x480@25fps 5s
```

操作：画面左上角实时显示 `Frame / Size / FPS` 三行信息；**先点一下窗口给它焦点，再按 Esc** 可提前退出；
播完自动退出。结果自动保存到 `./output/`。

### 第 3 步：检查输出

```bash
ls -lh output/
ffprobe -v error -select_streams v:0 \
        -show_entries stream=width,height,r_frame_rate,nb_frames,duration \
        -of default=noprint_wrappers=1 output/motion.mp4
```

预期：`width=1280`、`height=720`、`nb_frames=180`、`duration=6.000000`——与输入一致。

### 备选：双击启动器（不需要命令行，也不需要 MSYS2 环境）

`dist\` 目录里有 5 个自包含的 `.bat`（153 个 DLL + Qt 插件已就位，拷到任何 Windows 机器都能直接双击）：

| 文件 | 作用 |
|---|---|
| `1-图片取色.bat` | 打开三色块校准图，点色块核对 BGR |
| `2-图片模式-照片.bat` | 打开 1920×1200 真实照片 |
| `3-视频模式-720p.bat` | 播放 1280×720@30，验证 Esc 退出与保存 |
| `4-视频模式-小视频.bat` | 播放 640×480@25，验证换素材可用 |
| `5-错误提示演示.bat` | 连跑 4 种错误情况，核对退出码 2/3/4/2 |

逐项"点哪里、应看到什么"的对照见 `docs/操作手册.html`。
需要重新生成 `dist\`（例如换了电脑打包）：

```bash
bash tools/deploy_deps.sh
```

---

## 程序实现思路与核心代码解释

### 1. 整体数据流

```text
main.cpp（只做参数解析 + 分派，不含业务逻辑）
   ├── img   <路径>  →  runImageMode()   →  loadImageStrict → imshow → waitKey → onMouse 回调
   └── video <路径>  →  runVideoMode()   →  VideoReader（唯一碰 VideoCapture 的类）
                                          →  FpsMeter 统计
                                          →  drawOverlay（唯一往画面上写字的地方）
                                          →  ├─ VideoSink（唯一碰 VideoWriter 的类）→ output/
                                             └─ cv::imshow
```

设计主线只有一条：**每个"会单独失败"或"被多处共用"的能力都下沉成独立模块，主程序只负责编排。**

### 2. 图片模式：四个评分点怎么落实

| 要求 | 实现 |
|---|---|
| 判断图片是否读取成功 | `loadImageStrict()`：文件存在性检查 → `imread` 判空 → `ifstream + imdecode` 兜底（三层） |
| 显示图片，按键正常退出 | `namedWindow` + `imshow` + `waitKey(0)` |
| 输出宽、高、通道数 | 打印 `cols / rows / channels()` 并附中文释义 |
| 点击输出坐标与 BGR | `onMouse()`：`EVENT_LBUTTONDOWN` + `at<Vec3b>(y, x)`，带越界与通道数保护 |

其中 `imdecode` 兜底这一层是为 Windows 中文路径准备的：OpenCV 内部用窄字符 `fopen`，非 ANSI 路径不可靠。

### 3. 视频模式主循环（核心代码）

```cpp
while (true) {
    if (!reader.readNext(frame)) break;      // ① 自然结束：读完或解码失败
    fpsMeter.tick();                         // ② 统计实际处理吞吐
    drawOverlay(frame, info);                // ③ 原地叠加，全流程只调用这一次
    sink.write(frame);                       // ④ 写盘
    cv::imshow(kWindowName, frame);          // ⑤ 显示 —— 与④同一块内存
    if (cv::waitKey(1) == 27) {              // ⑥ Esc 提前退出
        escPressed = true;
        break;
    }
}
```

三个关键点：

1. **两个退出出口**：`readNext()` 返回 false（自然结束）与 `waitKey()` 返回 27（Esc）。两者分别置不同的结束标志，收尾时打印到底是哪一种。
2. **帧号只在成功后推进**：`readNext()` 里 `if (!ok) return false;` 之后才 `++m_frameNumber`，失败不改状态、不交付脏帧。所以收尾的"共处理 N 帧"永远不会比总帧数多 1。
3. **叠加只画一次**：`drawOverlay()` 在此处调用一次，写盘和显示消费同一块 `cv::Mat`，物理上不可能不一致。

**容易踩的错法（会扣分）**：先 `imshow` 再 `drawOverlay` 再 `write`；或显示用一套字号、写盘用另一套参数。

### 4. `VideoReader` 类设计

```cpp
class VideoReader {
public:
    explicit VideoReader(const std::string& videoPath);   // 构造即打开 + 读元数据
    ~VideoReader();
    VideoReader(const VideoReader&) = delete;             // 禁拷贝
    VideoReader& operator=(const VideoReader&) = delete;

    bool     isOpened()    const;   // 是否成功打开
    int      frameNumber() const;   // 当前帧号（1 基）
    int      totalFrames() const;   // 总帧数（未知返回 -1）
    double   fps()         const;   // 帧率（未知返回 -1.0）
    cv::Size frameSize()   const;   // 分辨率（未知返回 0x0）

    bool readNext(cv::Mat& frame);  // 读下一帧，成功才推进帧号
private:
    void readMetadata();
    std::unique_ptr<cv::VideoCapture> m_capture;   // 视频对象私有持有
    // ... 其余 6 个成员全部 private
};
```

| 评分点 | 落实方式 |
|---|---|
| 私有状态 | 7 个成员全在 `private`，无 public 成员变量；读元数据本身也是私有方法 |
| 构造接路径并打开 | `explicit` 构造，成员初始化列表全部赋初值（杜绝未初始化），失败不抛异常只置标志 |
| 查询接口 | 6 个，全部 `const`、全部无副作用（不读帧、不移动解码位置） |
| 读取接口 | `bool readNext(Mat&)`，**成功后才 `++m_frameNumber`** |
| 主程序不碰 `VideoCapture` | 见下一条，**编译期强制**，不靠自觉 |

**帧号为什么用 1 基**：`frameNumber()` 定义为"已成功读取的帧数"，第一帧就是 1，屏显 `Frame 1 / 750` 无需换算，
与观众直觉一致。

### 5. 如何保证主程序不直接操作 `VideoCapture`（编译期强制）

不是靠"记得别这么写"，而是让编译器拦住：

- `video_reader.h` 里 `cv::VideoCapture` **只有前置声明**：`namespace cv { class VideoCapture; }`
- 真正的 `#include <opencv2/videoio.hpp>` 只出现在 `video_reader.cpp` 内部
- 私有成员用 `std::unique_ptr<cv::VideoCapture>` 持有，析构函数在 `.cpp` 里定义

于是 `video_mode.cpp`（主程序）**根本拿不到 `cv::VideoCapture` 这个完整类型**。
谁写一句 `cv::VideoCapture cap;`，编译直接失败（incomplete type）。

静态验证（预期无输出）：

```bash
grep -n "VideoCapture" src/video_mode.cpp | grep -v "//"
```

### 6. 两个 FPS 的区别（重要）

| 名称 | 含义 | 用途 |
|---|---|---|
| **屏显 FPS** | `FpsMeter` 实测的处理吞吐 | 画在画面上，反映真实处理速度，不美化 |
| **容器帧率** | 输入视频的 `CAP_PROP_FPS` | 传给 `VideoWriter`，决定输出视频时长 |

题面"输出视频中显示的 FPS 与处理画面一致"指**叠加内容一致**，不是"容器帧率等于实测吞吐"。
若把实测吞吐当容器帧率传进去，输出视频就会快进或慢放。

算法：最近 30 帧滑动窗口求平均 + 指数平滑（新值权重 0.2）；样本不足 2 个时返回 0，
屏显渲染成 `--`，避免首帧闪一个假的 `0.0`。

### 7. 编码器降级链

```text
avc1 / .mp4  →  mp4v / .mp4  →  XVID / .avi  →  MJPG / .avi  →  FFV1 / .avi
```

逐个尝试，全部失败才报错，并列出**试过哪些组合**。奇数宽高（如 641×481）会让 `yuv420p` 编码失败，
此时自动落到支持任意尺寸的 FFV1。

`VideoSink` 的析构函数强制 `release()`——mp4 的 moov 索引是在 release 时写入的，
不释放就会得到 0 字节文件。用 RAII 保证，不依赖调用方记得。

### 8. 模块职责划分（题面要求 ≥2 个函数，实际拆出 7 个）

| 函数 / 方法 | 职责 | 为什么必须独立 |
|---|---|---|
| `runImageMode()` | 图片模式全流程编排 | `main` 只分派，不含业务 |
| `runVideoMode()` | 视频模式全流程编排 | 与上式对称，体现"模式"概念 |
| `loadImageStrict()` | 存在性 + 解码 + 判空 + imdecode 兜底 | 会单独失败，是"明确提示"的落点 |
| `drawOverlay()` | 帧号 / 分辨率 / FPS 叠加 | 被显示与写盘两个消费者共用 |
| `VideoSink::openWithFallback()` | 5 级编码降级 + 建目录 | 会单独失败且有降级策略 |
| `onMouse()` | 点击取坐标 + BGR | 回调天然独立 |
| `FpsMeter::tick() / fps()` | 时间统计 | 纯算法，与画面解耦 |

拆分原则：**主流程函数只负责编排与错误分支；凡是可能单独失败、或被多处共用的逻辑，都下沉为独立函数。**
不是为了凑数把 5 行代码包一层。

### 9. 目录结构

```text
作业/
├── CMakeLists.txt          构建入口（C++17、找 OpenCV、定义 3 个 target）
├── CMakePresets.json       固化的 UCRT64 + Ninja 配置，VS Code 可直接识别
├── README.md               本文件
├── include/                头文件（声明）
│   ├── app_common.h        退出码、UTF-8 控制台、统一提示、建目录、文件检查
│   ├── video_reader.h      ★ VideoReader 声明（唯一出现 VideoCapture 的头文件）
│   ├── video_writer.h      VideoSink 声明（编码降级 + 自动建目录）
│   ├── fps_meter.h         FpsMeter 声明（滑动窗口 + 指数平滑）
│   ├── overlay.h           OverlayInfo 结构 + drawOverlay 声明（唯一叠加入口）
│   ├── image_mode.h        runImageMode 声明
│   └── video_mode.h        runVideoMode 声明
├── src/                    实现
│   ├── main.cpp            参数解析 + 分派，不含业务逻辑
│   ├── app_common.cpp      公共设施实现
│   ├── image_mode.cpp      图片模式 + 鼠标回调
│   ├── video_mode.cpp      视频模式主循环（显示 + 叠加 + 写盘）
│   ├── video_reader.cpp    ★ VideoReader 实现（唯一出现 cap->read() 的文件）
│   ├── video_writer.cpp    VideoSink 实现（唯一出现 VideoWriter 的文件）
│   ├── fps_meter.cpp       FPS 统计实现
│   └── overlay.cpp         drawOverlay 实现（唯一往画面上写字的地方）
├── tools/                  辅助工具（不参与主程序）
│   ├── env_check.cpp       环境自检：版本 / GUI 后端 / 编解码 / 弹窗测试
│   ├── make_assets.cpp     生成测试素材
│   └── deploy_deps.sh      收集依赖 DLL，生成可双击的 dist/
├── assets/                 测试素材（校准图、彩条图、照片、两套视频）
├── output/                 运行产物（程序自动创建）
├── launchers/              5 个启动器的源码备份
├── docs/                   操作手册与讲解
└── dist/                   自包含发布包（不提交，用 deploy_deps.sh 生成）
```

---

## 调试时遇到的问题与解决方法

按「现象 → 原因 → 解决」记录，共 11 条，全部为本工程实际踩过的坑。

### 1. 程序跑起来没有窗口，也不报任何错

**原因**：没装 Qt6 运行库。MSYS2 的 OpenCV 编译时启用了 `-DWITH_QT=ON`（HighGUI 用 Qt 做窗口后端），
但 `qt6-5compat` 在 OpenCV 包里只是 optdepends，pacman 不会自动安装。

**解决**：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat
```

验证：`pacman -Q | grep qt6` 应能看到 `qt6-base` 与 `qt6-5compat`。

**这是本工程最花时间的一个坑**——现象完全不报错，很容易误判成代码里 `imshow` 写错了。

### 2. 报 `Could not find or load the Qt platform plugin "windows"`

**原因**：Qt 的窗口后端是**运行时按插件加载**的，靠 `platforms/qwindows.dll`。
从 `ucrt64/bin` 之外运行、或单独把 exe 拷走，就找不到它。

**解决**（三选一）：
- 在 **UCRT64 终端**里运行（推荐）
- 或 `export QT_QPA_PLATFORM_PLUGIN_PATH=/ucrt64/share/qt6/plugins/platforms`
- 或 `bash tools/deploy_deps.sh` 打包，插件会一并拷进 `dist/`

### 3. `find_package(OpenCV 4 ...)` 被拒绝

```text
Could not find a configuration file for package "OpenCV" that is compatible
with requested version "4".
  D:/msys64/ucrt64/lib/cmake/opencv5/OpenCVConfig.cmake, version: 5.0.0
    The version found is not compatible with the version requested.
```

**原因**：直觉以为 `find_package(OpenCV 4 ...)` 表示"版本 ≥ 4"，其实不是。CMake 的版本参数是
"请求 4.x 的**兼容**版本"，而 OpenCV 的 config 文件用 **SameMajorVersion** 策略——主版本号必须相同，
所以它会直接拒绝 5.0.0。

**解决**：**不写版本号**——`find_package(OpenCV REQUIRED COMPONENTS ...)`。
这样 4.x 和 5.x 都能匹配，换到装了 4.x 的教学机上也能构建。

### 4. 报找不到 `libopencv_core-500.dll`

**原因**：`PATH` 里没有 `D:/msys64/ucrt64/bin`。

**解决**：在 UCRT64 终端里运行；或用 `dist/` 里打包好的版本。

### 5. 按 Esc 没反应

**原因**：Qt 后端的窗口必须**先获得焦点**才能收到键盘事件。

**解决**：运行时先用鼠标点一下画面，再按 Esc。

### 6. 处理 FPS 偏低（约 10–20）

**现象**：`motion.mp4`（1280×720@30、180 帧）跑完约 15 秒，屏显 FPS 约 11.8。

**原因**：这是**真实测量值**，主要开销在 H.264 软件编码（1280×720 时约 12 fps），不是 bug。

**说明**：
- 屏显 FPS 按设计就是"实际处理吞吐"，不美化
- 容器帧率仍取源帧率 30，所以**输出视频的时长与播放速度完全正确**（实测 `duration=6.000000`）
- 想让演示更流畅：把 `video_writer.cpp` 候选列表里的 `mp4v` 提到 `avc1` 之前，编码快数倍，代价是文件更大

### 7. 输出 mp4 是 0 字节 / 播放器打不开

**原因**：`VideoWriter::release()` 没被执行——mp4 的 moov 索引在 release 时写入。

**解决**：`VideoSink` 析构函数强制 `release()`，RAII 保证，正常不会出现。

### 8. 文件名里有中文，图片读不到

**原因**：OpenCV 内部用窄字符 `fopen`，在 Windows 上对非 ANSI 路径不可靠。

**解决**：`loadImageStrict()` 内置 `ifstream + imdecode` 兜底；但仍**推荐把素材放在纯英文路径下**。

### 9. 控制台中文乱码

**解决**：`main()` 第一行调用 `SetConsoleOutputCP(CP_UTF8)`，正常不会乱码。
老终端若仍有问题，可临时 `chcp 65001`。

### 10. 超大图片的窗口超出屏幕

**原因**：图片模式用 `WINDOW_AUTOSIZE`（1:1 显示，不缩放），保证坐标换算绝对准确、"点哪读哪"可肉眼验证。
代价是 4K 图（`photo1.jpg` 3840×2400）的窗口会超出屏幕。

**说明**：点击可见区域仍然能正确显示坐标与 BGR，属预期行为。

### 11. 在 Windows 上 `pacman-key --init` 卡住（环境搭建阶段）

**原因**：某些 Windows 环境里这一步会卡在 GPG 生成密钥，CPU 长时间空转。

**解决**：把 `/etc/pacman.conf` 的 `SigLevel` 设为 `Never` 后直接同步数据库
（只影响签名校验，包体仍来自官方镜像）。

---

## 附录

### A. 退出码约定

| 码 | 含义 | 触发场景 |
|---|---|---|
| 0 | 成功 | — |
| 2 | 参数错误 | 参数个数不对、未知子命令 |
| 3 | 输入文件不存在 | 路径写错、文件被删 |
| 4 | 图片解码失败 | 文件在但内容损坏 / 格式不支持 |
| 5 | 视频打不开 | 文件损坏 / 编解码器不支持 |
| 6 | 首帧读取失败 | 视频能打开但读不出画面 |
| 7 | 输出目录创建失败 | 无写权限 / 磁盘满 |
| 8 | 输出视频创建失败 | 所有编码组合均不可用 |
| 9 | 未捕获的 OpenCV 异常 | GUI 后端缺失等 |

实测：

```text
$ ./build/cv_hw.exe img assets/nonexist.jpg
错误：找不到输入文件：assets/nonexist.jpg
      当前工作目录：D:\cv-hw
      提示：可以用相对路径（assets/photo1.jpg）或绝对路径（D:/pics/a.jpg）。
$ echo $?
3
```

### B. 验收清单

| # | 操作 | 预期 | 对应评分 |
|---|---|---|---|
| 1 | `./build/env_check.exe` | 弹出绿窗 + `GUI: QT6` + `FFMPEG: YES` | 环境 |
| 2 | `./build/cv_hw.exe` | 打印用法，退出码 2 | 工程 |
| 3 | `./build/cv_hw.exe img 不存在.jpg` | 中文提示 + 退出码 3 | 明确提示 |
| 4 | `./build/cv_hw.exe img assets/photo1.jpg` | 打印 3840/2400/3，弹窗，按键退出 | 图片 30 分 |
| 5 | 在上一步弹窗里点红色块 | 控制台输出 `B=0 G=0 R=255` | 鼠标 BGR |
| 6 | `./build/cv_hw.exe video assets/motion.mp4` | 显示帧号/分辨率/FPS；自然结束，共 180 帧 | 视频 35 分 |
| 7 | 同上，中途按 Esc | 提前退出并打印"用户按 Esc 提前退出" | Esc 退出 |
| 8 | `ls output/` | 生成同名视频，体积非 0 | 保存 20 分 |
| 9 | 播放 `output/motion.mp4` | 能播；左上角三行信息与运行时逐字相同 | 一致性 |
| 10 | `./build/cv_hw.exe video assets/motion2.avi` | 640×480@25，共 125 帧 | 复测 |
| 11 | `grep -n "VideoCapture" src/video_mode.cpp \| grep -v "//"` | 无输出 | VideoReader 设计 |

### C. 实测结果

**环境自检**

```text
OpenCV 版本 : 5.0.0
GUI:            QT6
  QT:           YES (ver 6.11.2)
  Win32 UI:     YES
FFMPEG:         YES (prebuilt binaries)
  avcodec:      YES (63.1.102)
  avformat:     YES (63.1.102)
```

> `QT: YES (ver 6.11.2)` 是 OpenCV 包**编译时**链接的 Qt 版本；本机运行时装的是 Qt 6.12.0，
> 两者 ABI 兼容。`qmake6 --version` 可确认运行时版本。

**视频模式（motion.mp4）**

```text
分辨率   : 1280 x 720
源帧率   : 30 fps
总帧数   : 180
输出文件 : output/motion.mp4
编码格式 : H.264 (avc1) / .mp4
容器帧率 : 30 fps
结束方式 : 视频自然播放结束
处理帧数 : 180
```

**输出视频属性（ffprobe）**

```text
codec_name=h264
width=1280
height=720
r_frame_rate=30/1
nb_frames=180
duration=6.000000
```

**叠加信息一致性**：从 `output/motion.mp4` 抽取第 100 帧，画面左上角实际显示

```text
Frame : 100 / 180
Size  : 1280 x 720
FPS   : 11.8
```

帧号 `100` 与抽帧位置完全吻合，证明输出视频与处理画面使用的是同一份叠加内容。

**第二套素材（motion2.avi）**：640×480@25fps，共 125 帧，处理 125 帧，全部通过。

# 第一周综合练习作业 —— 视觉素材查看与标注程序（cv_hw）

用 CMake 管理、C++17 + OpenCV 实现的命令行程序，含图片模式与视频模式，可将带帧号/分辨率/FPS 叠加的处理结果保存到 `output/`。

## 环境

- 操作系统：Windows 11（`MINGW64_NT-10.0-26200`），工具链为 MSYS2 UCRT64，根目录 `D:/msys64`
- g++ 版本：16.2.0（`g++.exe (Rev6, Built by MSYS2 project)`）
- CMake 版本：4.4.4（生成器 Ninja 1.13.2，C++17）
- OpenCV 版本：5.0.0（MSYS2 包 `mingw-w64-ucrt-x86_64-opencv 5.0.0-7`）
- 窗口后端：Qt 6.12.0（`qt6-base` + `qt6-5compat`，OpenCV 内部构建信息 `GUI: QT6`）
- 视频编解码：FFmpeg（OpenCV 内置，avcodec / avformat 63.1.102）

> 作业示例环境为 Ubuntu 22.04；本机实际为 Windows 11 + MSYS2，故命令按 Windows 书写，源码不依赖任何平台特有路径。

在 MSYS2 UCRT64 终端装齐依赖：

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
          mingw-w64-ucrt-x86_64-cmake \
          mingw-w64-ucrt-x86_64-ninja \
          mingw-w64-ucrt-x86_64-opencv \
          mingw-w64-ucrt-x86_64-qt6-base \
          mingw-w64-ucrt-x86_64-qt6-5compat
```

`qt6-5compat` 必须显式安装：它在 OpenCV 包里只是 optdepends，pacman 不会自动带。缺了它编译通过、运行不报错，但窗口不出来。

## 编译

必须在 UCRT64 环境（开始菜单的 `MSYS2 UCRT64` 终端）中操作，这样 `PATH` 里才有 `D:/msys64/ucrt64/bin`。

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

`CMAKE_BUILD_TYPE=Release` 不可省略：Ninja 是单配置生成器，不指定就没有优化，视频帧率会低到误以为代码有问题。`--parallel` 无需传核数，Ninja 默认按 CPU 核数并行。

两个辅助工具单独构建：

```bash
cmake --build build --target env_check
cmake --build build --target make_assets
```

产物为 `build/cv_hw.exe`（主程序）、`build/env_check.exe`（环境自检）、`build/make_assets.exe`（素材生成）。
`-Wall -Wextra` 下 11 个编译单元零错误零警告。

若报 `Could not find a package configuration file provided by "OpenCV"`，手动指定 config 目录：

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DOpenCV_DIR=D:/msys64/ucrt64/lib/cmake/opencv5
```

## 运行

```bash
./build/env_check.exe                          # 环境自检：应弹绿窗，显示 GUI: QT6 / FFMPEG: YES
./build/cv_hw.exe img assets/photo1.jpg        # 图片模式（3840x2400）
./build/cv_hw.exe img assets/calib_bgr.png     # 图片模式（三色块校准图）
./build/cv_hw.exe video assets/motion.mp4      # 视频模式（1280x720@30，6s）
./build/cv_hw.exe video assets/motion2.avi     # 视频模式（640x480@25，5s）
```

图片模式：控制台打印宽/高/通道数；鼠标左键点击画面打印该点坐标与 BGR；按任意键退出。

视频模式：左上角实时显示 `Frame / Size / FPS`；先点一下窗口给它焦点，再按 Esc 可提前退出；播完自动退出；结果保存到 `./output/`。

检查输出：

```bash
ls -lh output/
ffprobe -v error -select_streams v:0 \
        -show_entries stream=width,height,r_frame_rate,nb_frames,duration \
        -of default=noprint_wrappers=1 output/motion.mp4
```

预期 `width=1280`、`height=720`、`nb_frames=180`、`duration=6.000000`，与输入一致。

## 程序实现思路与核心代码解释

### 1. 整体数据流

```text
main.cpp（只做参数解析 + 分派，不含业务逻辑）
   ├── img   <路径>  →  runImageMode()  →  loadImageStrict → imshow → waitKey → onMouse 回调
   └── video <路径>  →  runVideoMode()  →  VideoReader（唯一碰 VideoCapture 的类）
                                        →  FpsMeter 统计
                                        →  drawOverlay（唯一往画面上写字的地方）
                                        →  ├─ VideoSink（唯一碰 VideoWriter 的类）→ output/
                                           └─ cv::imshow
```

设计主线：**每个"会单独失败"或"被多处共用"的能力都下沉成独立模块，主程序只负责编排。**

### 2. 图片模式：四个评分点

| 要求 | 实现 |
|---|---|
| 判断图片是否读取成功 | `loadImageStrict()`：文件存在性检查 → `imread` 判空 → `ifstream + imdecode` 兜底（三层） |
| 显示图片，按键正常退出 | `namedWindow` + `imshow` + `waitKey(0)` |
| 输出宽、高、通道数 | 打印 `cols / rows / channels()` 并附中文释义 |
| 点击输出坐标与 BGR | `onMouse()`：`EVENT_LBUTTONDOWN` + `at<Vec3b>(y, x)`，带越界与通道数保护 |

`imdecode` 兜底这一层是为 Windows 中文路径准备的：OpenCV 内部用窄字符 `fopen`，非 ANSI 路径不可靠。

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

1. **两个退出出口**：`readNext()` 返回 false（自然结束）与 `waitKey()` 返回 27（Esc）。两者分别置不同结束标志，收尾时打印到底是哪一种。
2. **帧号只在成功后推进**：`readNext()` 里 `if (!ok) return false;` 之后才 `++m_frameNumber`，失败不改状态、不交付脏帧，所以收尾的"共处理 N 帧"不会比总帧数多 1。
3. **叠加只画一次**：`drawOverlay()` 在此处调用一次，写盘与显示消费同一块 `cv::Mat`，物理上不可能不一致。

容易踩的错法（会扣分）：先 `imshow` 再 `drawOverlay` 再 `write`；或显示与写盘用两套字号/颜色。

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
    // ... 其余成员全部 private
};
```

| 评分点 | 落实方式 |
|---|---|
| 私有状态 | 7 个成员全在 `private`，无 public 成员变量；读元数据本身也是私有方法 |
| 构造接路径并打开 | `explicit` 构造，成员初始化列表全部赋初值，失败不抛异常只置标志 |
| 查询接口 | 6 个，全部 `const`、全部无副作用（不读帧、不移动解码位置） |
| 读取接口 | `bool readNext(Mat&)`，成功后才 `++m_frameNumber` |
| 主程序不碰 `VideoCapture` | 见下条，编译期强制 |

帧号用 1 基：`frameNumber()` 定义为"已成功读取的帧数"，第一帧就是 1，屏显 `Frame 1 / 750` 无需换算。

### 5. 如何保证主程序不直接操作 `VideoCapture`

不靠自觉，靠编译器：`video_reader.h` 里 `cv::VideoCapture` 只有前置声明（`namespace cv { class VideoCapture; }`），真正的 `#include <opencv2/videoio.hpp>` 只出现在 `video_reader.cpp` 内部，私有成员用 `std::unique_ptr<cv::VideoCapture>` 持有、析构函数在 `.cpp` 里定义。

于是 `video_mode.cpp` 根本拿不到 `cv::VideoCapture` 的完整类型，谁写一句 `cv::VideoCapture cap;` 编译直接失败（incomplete type）。

静态验证（预期无输出）：

```bash
grep -n "VideoCapture" src/video_mode.cpp | grep -v "//"
```

### 6. 两个 FPS 的区别

| 名称 | 含义 | 用途 |
|---|---|---|
| 屏显 FPS | `FpsMeter` 实测的处理吞吐 | 画在画面上，反映真实处理速度，不美化 |
| 容器帧率 | 输入视频的 `CAP_PROP_FPS` | 传给 `VideoWriter`，决定输出视频时长 |

题面"输出视频中显示的 FPS 与处理画面一致"指叠加内容一致，不是"容器帧率等于实测吞吐"。若把实测吞吐当容器帧率传进去，输出视频会快进或慢放。

算法：最近 30 帧滑动窗口求平均 + 指数平滑（新值权重 0.2）；样本不足 2 个时返回 0，屏显渲染成 `--`，避免首帧闪一个假的 `0.0`。

### 7. 编码器降级链

```text
avc1 / .mp4  →  mp4v / .mp4  →  XVID / .avi  →  MJPG / .avi  →  FFV1 / .avi
```

逐个尝试，全部失败才报错并列出试过的组合。奇数宽高（如 641×481）会让 `yuv420p` 编码失败，此时自动落到支持任意尺寸的 FFV1。

`VideoSink` 析构函数强制 `release()`：mp4 的 moov 索引在 release 时写入，不释放就得到 0 字节文件。用 RAII 保证，不依赖调用方记得。

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

拆分原则：主流程函数只负责编排与错误分支；凡是可能单独失败、或被多处共用的逻辑，都下沉为独立函数。不是为了凑数把 5 行代码包一层。

## 调试时遇到的问题与解决方法

### 1. 程序跑起来没有窗口，也不报任何错

原因：没装 Qt6 运行库。MSYS2 的 OpenCV 编译时启用了 `-DWITH_QT=ON`，但 `qt6-5compat` 在 OpenCV 包里只是 optdepends。

解决：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat
```

验证：`pacman -Q | grep qt6` 应能看到 `qt6-base` 与 `qt6-5compat`。这是本工程最花时间的坑——现象完全不报错，很容易误判成 `imshow` 写错了。

### 2. 报 `Could not find or load the Qt platform plugin "windows"`

原因：Qt 的窗口后端是运行时按插件加载的，靠 `platforms/qwindows.dll`。从 `ucrt64/bin` 之外运行、或单独把 exe 拷走就找不到。

解决：在 UCRT64 终端里运行；或 `export QT_QPA_PLATFORM_PLUGIN_PATH=/ucrt64/share/qt6/plugins/platforms`。

### 3. `find_package(OpenCV 4 ...)` 被拒绝

```text
Could not find a configuration file for package "OpenCV" that is compatible
with requested version "4".
  D:/msys64/ucrt64/lib/cmake/opencv5/OpenCVConfig.cmake, version: 5.0.0
    The version found is not compatible with the version requested.
```

原因：直觉以为 `find_package(OpenCV 4 ...)` 表示"版本 ≥ 4"，其实 CMake 的版本参数是"请求 4.x 的兼容版本"，而 OpenCV 的 config 用 SameMajorVersion 策略——主版本号必须相同，所以直接拒绝 5.0.0。

解决：不写版本号，改为 `find_package(OpenCV REQUIRED COMPONENTS ...)`。这样 4.x 与 5.x 都能匹配，换到装了 4.x 的教学机上也能构建。

### 4. 报找不到 `libopencv_core-500.dll`

原因：`PATH` 里没有 `D:/msys64/ucrt64/bin`。解决：在 UCRT64 终端里运行。

### 5. 按 Esc 没反应

原因：Qt 后端的窗口必须先获得焦点才能收到键盘事件。解决：先用鼠标点一下画面，再按 Esc。

### 6. 处理 FPS 偏低（约 10–20）

现象：`motion.mp4`（1280×720@30、180 帧）跑完约 15 秒，屏显 FPS 约 11.8。

原因：这是真实测量值，主要开销在 H.264 软件编码（1280×720 时约 12 fps），不是 bug。屏显 FPS 按设计就是实际处理吞吐，不美化；容器帧率仍取源帧率 30，所以输出视频的时长与播放速度完全正确（实测 `duration=6.000000`）。

若想让演示更流畅：把 `video_writer.cpp` 候选列表里的 `mp4v` 提到 `avc1` 之前，编码快数倍，代价是文件更大。

### 7. 输出 mp4 是 0 字节 / 播放器打不开

原因：`VideoWriter::release()` 没被执行，mp4 的 moov 索引在 release 时写入。解决：`VideoSink` 析构函数强制 `release()`，RAII 保证。

### 8. 文件名里有中文，图片读不到

原因：OpenCV 内部用窄字符 `fopen`，在 Windows 上对非 ANSI 路径不可靠。解决：`loadImageStrict()` 内置 `ifstream + imdecode` 兜底；但仍推荐把素材放在纯英文路径下。

### 9. 控制台中文乱码

`main()` 第一行调用 `SetConsoleOutputCP(CP_UTF8)`，正常不会乱码；老终端若仍有问题可临时 `chcp 65001`。

### 10. 超大图片的窗口超出屏幕

原因：图片模式用 `WINDOW_AUTOSIZE`（1:1 显示，不缩放），保证坐标换算绝对准确、"点哪读哪"可肉眼验证；代价是 4K 图（3840×2400）窗口会超出屏幕。点击可见区域仍能正确显示坐标与 BGR，属预期行为。

### 11. 在 Windows 上 `pacman-key --init` 卡住（环境搭建阶段）

原因：某些 Windows 环境里这一步会卡在 GPG 生成密钥，CPU 长时间空转。解决：把 `/etc/pacman.conf` 的 `SigLevel` 设为 `Never` 后直接同步数据库（只影响签名校验，包体仍来自官方镜像）。

### 12. 退出码约定（便于定位错误来源）

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

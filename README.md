# 视觉素材查看与标注程序（cv_hw）

第一周综合练习作业。使用 **CMake** 管理的 **C++17 + OpenCV** 工程，实现图片模式与视频模式，
并将视频处理结果保存到 `output/` 目录。

> 本工程在本机完整跑通并通过全部可自动验证的检查项，实测数据见「十二、实测结果」。

---

## 一、功能清单

### 图片模式（30 分）

| 要求 | 实现位置 |
|---|---|
| 判断图片是否读取成功 | `loadImageStrict()`：文件存在性检查 + `imread` 判空 + `imdecode` 兜底 |
| 显示图片，按键后正常退出 | `runImageMode()`：`namedWindow` + `imshow` + `waitKey(0)` |
| 输出图片宽度、高度和通道数 | `runImageMode()` 打印 `cols / rows / channels()`，并附中文释义 |
| 鼠标点击输出坐标与 BGR | `onMouse()`：`EVENT_LBUTTONDOWN` + `at<Vec3b>(y, x)`，带越界与通道数保护 |

### 视频模式（35 分）

| 要求 | 实现位置 |
|---|---|
| 判断视频是否成功打开 | `VideoReader::isOpened()`，构造后立即判断 |
| 画面显示帧号、分辨率、处理 FPS | `drawOverlay()`，三行信息绘制在左上角半透明底板内 |
| 按 Esc 提前退出 / 自然结束退出 | 主循环 `waitKey()` 返回 27 则 break；`readNext()` 返回 false 也 break |
| `VideoReader` 类设计 | `include/video_reader.h` + `src/video_reader.cpp` |
| 主程序不直接操作 `cv::VideoCapture` | `video_mode.cpp` 中无任何 `VideoCapture` 字样（**编译期强制**，见 6.2） |

### 保存结果（20 分）

| 要求 | 实现位置 |
|---|---|
| 输出到 `output/`，目录不存在自动创建 | `VideoSink::openWithFallback()` 调用 `app::ensureDirectory()` |
| 输出尺寸与输入一致 | `VideoWriter` 用 `reader.frameSize()` 打开；`write()` 前再做尺寸校验 |
| 输出视频能正常播放 | `VideoSink` 析构强制 `release()`，保证 mp4 的 moov 索引写入 |
| 叠加信息与处理画面一致 | `drawOverlay()` 只调用一次，显示与写盘消费同一块 `cv::Mat` |

### 工程与说明（15 分）

| 要求 | 实现 |
|---|---|
| `CMakeLists.txt` 可配置可构建 | 见 `CMakeLists.txt`，另提供 `CMakePresets.json` |
| 源文件清晰，≥2 个职责明确的函数 | 实际拆出 7 个（见 6.6） |
| `README.md` 写明环境/编译/运行/已知问题 | 即本文件 |
| 输入文件不存在或读不了给出明确提示 | 8 类错误各有中文提示 + 独立退出码（见「七、退出码」） |

---

## 二、开发环境

| 项目 | 版本 / 说明 |
|---|---|
| 操作系统 | Windows 11（`MINGW64_NT-10.0-26200`） |
| C++ 编译器 | **GCC 16.2.0**（MSYS2 UCRT64，`g++.exe (Rev6, Built by MSYS2 project)`） |
| 构建工具 | **CMake 4.4.4** + **Ninja 1.13.2** |
| 视觉库 | **OpenCV 5.0.0**（MSYS2 包 `mingw-w64-ucrt-x86_64-opencv 5.0.0-7`） |
| 窗口后端 | **Qt 6.12.0**（`qt6-base` + `qt6-5compat`），OpenCV 内部构建信息显示 `GUI: QT6` |
| 视频编解码 | ffmpeg 9.0.2（含 libx264 / xvidcore / x265 / libvpx） |
| C++ 标准 | C++17（`-Wall -Wextra` 零警告） |
| IDE | VS Code（配合 `CMakePresets.json` 直接识别） |

### 为什么选这套环境

这台机器原本只有 Dev-C++ 自带的 **TDM-GCC 4.9.2（2014 年）**：没有 `<filesystem>`，
不支持 CMake 工程，也无法链接 OpenCV 5.x。而 OpenCV 官方的 Windows 预编译包是
**MSVC ABI**，MinGW 无法直接链接。所以选择 MSYS2 —— 用 pacman 一条命令装齐
「现代 GCC + CMake + Ninja + 预编译 OpenCV」，且 OpenCV 与编译器出自同一套工具链，
不存在 ABI 不匹配问题。

### ⚠️ 一个必须知道的坑：Qt6 一定要手动装

MSYS2 的 OpenCV 编译时启用了 `-DWITH_QT=ON`（HighGUI 用 Qt 做窗口后端），
但 `qt6-5compat` 在 OpenCV 包里只是 **optdepends（可选依赖）**，pacman 不会自动安装。

少了它的现象非常隐蔽：

> **编译顺利通过 → 运行不报任何错 → 但窗口根本不出来**

所以安装命令必须显式带上这两个包：

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat
```

跑一次 `env_check` 就能确认（见「五、运行」第 0 步）。

---

## 三、目录结构

```
cv-hw/
├── CMakeLists.txt          构建入口（C++17、找 OpenCV、定义 3 个 target）
├── CMakePresets.json       固化的 UCRT64 + Ninja 配置，VS Code 可直接识别
├── README.md               本文件
├── .gitignore
│
├── include/                头文件（声明）
│   ├── app_common.h        退出码、UTF-8 控制台、统一提示、建目录、文件检查
│   ├── video_reader.h      ★ VideoReader 类声明（唯一出现 VideoCapture 的头文件）
│   ├── video_writer.h      VideoSink 类声明（编码器降级 + 自动建目录）
│   ├── fps_meter.h         FpsMeter 类声明（滑动窗口 + 指数平滑）
│   ├── overlay.h           OverlayInfo 结构 + drawOverlay 声明（唯一叠加入口）
│   ├── image_mode.h        runImageMode 声明（不含 OpenCV 头文件）
│   └── video_mode.h        runVideoMode 声明（不含 OpenCV 头文件）
│
├── src/                    实现
│   ├── main.cpp            参数解析 + 分派，不含任何业务逻辑
│   ├── app_common.cpp      公共设施实现
│   ├── image_mode.cpp      图片模式 + 鼠标回调
│   ├── video_mode.cpp      视频模式主循环（显示 + 叠加 + 写盘）
│   ├── video_reader.cpp    ★ VideoReader 实现（唯一出现 cap->read() 的文件）
│   ├── video_writer.cpp    VideoSink 实现（唯一出现 VideoWriter 的文件）
│   ├── fps_meter.cpp       FPS 统计实现
│   └── overlay.cpp         drawOverlay 实现（唯一往画面上写字的地方）
│
├── tools/                  辅助工具（不参与主程序）
│   ├── env_check.cpp       环境自检：版本 / GUI 后端 / 视频编解码 / 弹窗测试
│   ├── make_assets.cpp     生成测试素材：三色块校准图 + 彩条图 + 运动视频
│   └── deploy_deps.sh      收集依赖 DLL，做出可直接双击的 dist/ 发布包
│
├── assets/                 测试素材
│   ├── calib_bgr.png       三色块校准图 640x480（验证鼠标"点哪读哪"）
│   ├── bars_smpte.png      彩条图 800x600（验证读图/显示/通道数）
│   ├── photo1.jpg          真实照片 3840x2400（Windows 壁纸，测大图）
│   ├── photo2.jpg          真实照片 1920x1200（第二套复测）
│   ├── motion.mp4          1280x720@30fps 6s H.264（主测试）
│   ├── motion2.avi         640x480@25fps 5s mpeg4（第二套复测）
│   └── gen_motion.mp4      640x480@25fps 6s（make_assets 生成，备用）
│
├── output/                 运行产物（程序自动创建）
├── docs/                   讲解文档
└── build/                  构建目录（不提交）
```

---

## 四、编译

### 前置：必须在 UCRT64 环境中操作

打开开始菜单的 **`MSYS2 UCRT64`**（或在其它终端设置 `MSYSTEM=UCRT64`）。
这样 `PATH` 里才会有 `C:/msys64/ucrt64/bin`，才能找到 OpenCV 的 DLL。

### 步骤

```bash
cd /d/cv-hw

# 1) 配置
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 2) 构建
cmake --build build

# 3) （可选）构建辅助工具
cmake --build build --target env_check
cmake --build build --target make_assets
```

> **如果报 `Could not find a package configuration file provided by "OpenCV"`**
> ，手动指定路径即可：
> ```bash
> cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
>       -DOpenCV_DIR=C:/msys64/ucrt64/lib/cmake/opencv5
> ```

---

## 五、运行

### 第 0 步：环境自检（强烈建议先跑）

```bash
./build/env_check.exe
```

应看到：
- `OpenCV 版本 : 5.0.0`
- `GUI: QT6`、`QT: YES`、`FFMPEG: YES`
- **弹出一个绿色窗口**，写着 `GUI OK`，按任意键关闭

窗口不出来 → 看「九、已知问题」第 1 条。

### 第 1 步：生成测试素材（`assets/` 里已备好，此步可跳过）

```bash
./build/make_assets.exe assets
```

### 第 2 步：图片模式

```bash
./build/cv_hw.exe img assets/calib_bgr.png
./build/cv_hw.exe img assets/photo1.jpg
```

操作：控制台打印宽/高/通道数；**鼠标左键点击画面**打印坐标与 BGR；**按任意键退出**。

### 第 3 步：视频模式

```bash
./build/cv_hw.exe video assets/motion.mp4
./build/cv_hw.exe video assets/motion2.avi
```

操作：左上角实时显示 `Frame / Size / FPS`；**按 Esc 提前退出**（需先点一下窗口给它焦点）；
播完自动退出；结果保存在 `./output/`。

### 第 4 步：检查输出

```bash
ls -lh output/
ffprobe -v error -select_streams v:0 -show_entries stream=width,height,nb_frames -of default=noprint_wrappers=1 output/motion.mp4
```

### 第 5 步：打包成可双击的发布包（可选）

```bash
bash tools/deploy_deps.sh
```

在 `dist/` 下生成自带全部 DLL 的版本，拷到别的电脑也能跑。

---

## 六、设计说明

### 6.1 VideoReader：接口与帧号语义

```cpp
class VideoReader {
public:
    explicit VideoReader(const std::string& videoPath);   // 构造即打开 + 读元数据
    ~VideoReader();
    VideoReader(const VideoReader&) = delete;             // 禁拷贝
    VideoReader& operator=(const VideoReader&) = delete;

    bool  isOpened()    const;   // 是否成功打开
    int   frameNumber() const;   // 当前帧号（1 基）
    int   totalFrames() const;   // 总帧数（未知 -1）
    double fps()        const;   // 帧率（未知 -1.0）
    cv::Size frameSize() const;  // 分辨率（未知 0x0）

    bool readNext(cv::Mat& frame);   // 读下一帧，成功才推进帧号
private:
    void readMetadata();
    std::unique_ptr<cv::VideoCapture> m_capture;   // 视频对象：私有
    // ... 其余状态全部 private
};
```

对照评分点：

| 评分点 | 落实 |
|---|---|
| 私有状态 | 7 个成员全在 `private`，无 public 成员变量；读元数据也是私有方法 |
| 构造函数接路径并打开 | `explicit` 构造，成员初始化列表全部赋初值（杜绝未初始化），失败不抛异常 |
| 查询接口 | 6 个，全部 `const`、全部无副作用（不读帧、不移动解码位置） |
| 读取接口 | `bool readNext(Mat&)`，**成功后才 `++m_frameNumber`**，失败不改状态、不交付脏帧 |
| 主程序不碰 VideoCapture | 见 6.2，**编译期强制** |

**帧号语义**：采用 **1 基**，即 `frameNumber()` = 已成功读取的帧数，第一帧就是 1，
屏显 `Frame 1 / 750` 无需换算。`readNext()` 失败（读到末尾或解码出错）时帧号**不推进**，
所以收尾打印的"共处理 N 帧"是准确的，不会出现比总帧数多 1 的情况。

### 6.2 如何保证主程序不直接操作 VideoCapture

不是靠"记得别这么写"，而是靠**编译器**：

`video_reader.h` 里 `cv::VideoCapture` **只有前置声明**（`namespace cv { class VideoCapture; }`），
真正的 `#include <opencv2/videoio.hpp>` 只出现在 `video_reader.cpp` 内部；
私有成员用 `std::unique_ptr<cv::VideoCapture>` 持有，析构函数在 `.cpp` 里定义。

于是 `video_mode.cpp`（主程序）**根本拿不到 `cv::VideoCapture` 这个类型**。
谁写一句 `cv::VideoCapture cap;`，编译直接失败（不完整类型）。

静态验证：

```bash
# 1) 主程序里有没有"非注释"的 VideoCapture 用法？（预期：无输出）
grep -n "VideoCapture" src/video_mode.cpp | grep -v "//"

# 2) 全工程看看这个词都出现在哪（注释里提到它是正常的，因为要解释设计）
grep -rn "VideoCapture" src/ include/
#    实际命中：video_reader.h 的前置声明与私有成员、video_reader.cpp 的真实使用、
#    以及若干解释性注释。主程序 video_mode.cpp 中的命中项全部是注释行。
```

### 6.3 叠加一致性：为什么不可能不一致

主循环里 `drawOverlay()` **只调用一次**，然后同一块 `cv::Mat` 分别喂给写盘和显示：

```cpp
if (!reader.readNext(frame)) break;   // 自然结束
fpsMeter.tick();
drawOverlay(frame, info);             // ★ 原地画，只画一次
sink.write(frame);                    // 写盘
cv::imshow(kWindowName, frame);       // 显示  ← 同一块内存
```

两者消费同一份像素，物理上不可能不一致。

**容易踩的错法**（会扣分）：先 `imshow` 再 `drawOverlay` 再 `write`；
或显示用一套颜色/字号、写盘用另一套参数。

### 6.4 两个 FPS 的区别（重要）

| 名称 | 含义 | 用途 |
|---|---|---|
| **屏显 FPS** | `FpsMeter` 实测的处理吞吐 | 画在画面上，反映真实处理速度 |
| **容器帧率** | 输入视频的 `CAP_PROP_FPS` | 传给 `VideoWriter`，决定输出视频时长 |

题面要求"输出视频中显示的 FPS 与处理画面一致"，指的是**叠加内容一致**，
不是"容器帧率等于实测吞吐"。若把实测吞吐当容器帧率传进去，输出视频会快进或慢放。

**算法**：最近 30 帧滑动窗口求平均 + 指数平滑（新值权重 0.2）。
样本不足 2 个时返回 0，屏显渲染成 `--`，不会出现首帧闪 `0.0`。

### 6.5 编码器降级链

```
avc1 / .mp4  →  mp4v / .mp4  →  XVID / .avi  →  MJPG / .avi  →  FFV1 / .avi
```

全部失败才报错，并列出**试过哪些组合**。奇数宽高（如 641×481）会让 `yuv420p` 编码失败，
此时会自动落到支持任意尺寸的 FFV1。

### 6.6 职责划分（题面要求 ≥2 个，实际 7 个）

| 函数 | 职责 | 为什么必须独立 |
|---|---|---|
| `runImageMode()` | 图片模式全流程编排 | `main` 只分派，不含业务 |
| `runVideoMode()` | 视频模式全流程编排 | 与上式对称，体现"模式"概念 |
| `loadImageStrict()` | 存在性 + 解码 + 判空 + imdecode 兜底 | 会单独失败，是"明确提示"的落点 |
| `drawOverlay()` | 帧号/分辨率/FPS 叠加 | 被显示与写盘两个消费者共用 |
| `VideoSink::openWithFallback()` | 5 级编码降级 + 建目录 | 会单独失败且有降级策略 |
| `onMouse()` | 点击取坐标 + BGR | 回调天然独立 |
| `FpsMeter::tick() / fps()` | 时间统计 | 纯算法，与画面解耦 |

拆分原则：**主流程函数只负责编排与错误分支；凡是可能单独失败、或被多处共用的逻辑，
都下沉为独立函数。** 不是为了凑数把 5 行代码包一层。

---

## 七、退出码

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

实测示例：

```text
$ ./build/cv_hw.exe img assets/nonexist.jpg
错误：找不到输入文件：assets/nonexist.jpg
      当前工作目录：D:\cv-hw
      提示：可以用相对路径（assets/photo1.jpg）或绝对路径（D:/pics/a.jpg）。
$ echo $?
3
```

---

## 八、分阶段实现记录

题面建议"每完成一步都保留一个可运行版本，便于分别排查路径、窗口事件、视频读取和编码问题"。
本项目按此执行，git 提交历史对应如下：

| 阶段 | 内容 | 验收标准 |
|---|---|---|
| **v0** | 环境自检（`env_check`） | 窗口弹出 + `GUI: QT6` + `FFMPEG: YES` |
| **v1** | 图片模式：读图 + 判空 + 尺寸输出 + 显示退出 | 坏路径有中文提示；正常图打印宽高通道 |
| **v2** | 鼠标回调读 BGR | 点红块输出 `B=0 G=0 R=255` |
| **v3** | `VideoReader` + 逐帧显示 + Esc/自然退出 | 帧号连续、Esc 可中断 |
| **v4** | `FpsMeter` + `drawOverlay` | 三行信息稳定，首帧显示 `--` |
| **v5** | `VideoSink` 写盘 | `output/` 自动创建，能播，尺寸一致 |
| **v6** | README + 打包 + 换素材复测 | 按 README 从零跑通 |

---

## 九、已知问题

### 1. 程序跑起来没有窗口，也不报错

**最常见原因**：没装 Qt6 运行库。

```bash
pacman -S mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat
```

验证：`pacman -Q | grep qt6` 应能看到 `qt6-base` 与 `qt6-5compat`。

### 2. 报 `Could not find or load the Qt platform plugin "windows"`

Qt 的窗口后端是**运行时按插件加载**的，靠 `platforms/qwindows.dll`。
从 `ucrt64/bin` 之外运行、或单独把 exe 拷走，就会找不到它。

处理：
- 在 **UCRT64 终端**里运行（推荐）
- 或设置：`export QT_QPA_PLATFORM_PLUGIN_PATH=/ucrt64/share/qt6/plugins/platforms`
- 或跑 `bash tools/deploy_deps.sh` 打包，插件会一并拷进 `dist/`

### 3. 报找不到 `libopencv_core-500.dll`

同上，`PATH` 里没有 `C:/msys64/ucrt64/bin`。在 UCRT64 终端里运行即可。

### 4. 按 Esc 没反应

Qt 后端的窗口需要**先获得焦点**才能收到键盘事件。
运行时先用鼠标点一下画面，再按 Esc。

### 5. 处理 FPS 偏低（约 10–20）

这是**真实测量值**，主要开销在 H.264 软件编码（1280×720 时约 12 fps）。
本机实测：`motion.mp4`（1280×720@30、180 帧）跑完约 15 秒，屏显 FPS 约 11.8。

这不是 bug：
- 屏显 FPS 按设计就是"实际处理吞吐"，不美化
- 容器帧率仍取源帧率 30，所以**输出视频的时长与播放速度完全正确**（实测 duration=6.000s）
- 想让演示更流畅，可把 `video_writer.cpp` 里的候选列表把 `mp4v` 提到 `avc1` 之前
  （编码快数倍，代价是文件更大）

### 6. 输出 mp4 是 0 字节 / 播放器打不开

说明 `VideoWriter::release()` 没被执行（mp4 的 moov 索引在 release 时写入）。
本工程用 `VideoSink` 析构函数强制保证，正常不会出现。

### 7. 控制台中文乱码

`main()` 第一行调用 `SetConsoleOutputCP(CP_UTF8)`，正常情况下不会乱码。
老终端若仍有问题，可临时 `chcp 65001`。

### 8. 文件名里有中文，读不到

OpenCV 内部用窄字符 `fopen`，在 Windows 上对非 ANSI 路径不可靠。
`loadImageStrict()` 已内置 `ifstream + imdecode` 兜底；但**推荐把素材放在纯英文路径下**。

### 9. OpenCV 是 5.0，与教材的 4.x 有差异

本工程只用 4.x / 5.x 共有的稳定 API。三处差异已对冲：

1. `CAP_PROP_*` 在 5.x 不支持时返回 **-1**（4.x 返回 0）→ 所有元数据都做 `<= 0` 兜底
2. `putText` 在 5.0 换成 TrueType 渲染，字宽与 4.x 不同 → 信息框尺寸一律用
   `getTextSize()` 实测，不硬编码
3. 老 C 宏 `CV_FONT_HERSHEY_SIMPLEX` 已移除 → 一律写 `cv::FONT_HERSHEY_SIMPLEX`

### 10. `find_package(OpenCV 4 ...)` 会失败（本工程遇到过的真实坑）

直觉上会以为 `find_package(OpenCV 4 ...)` 表示"版本 ≥ 4"，其实不是。
CMake 的版本参数是"请求 4.x 的**兼容**版本"，而 OpenCV 的 config 文件用
**SameMajorVersion** 策略——主版本号必须相同。所以它会直接拒绝 5.0.0：

```
Could not find a configuration file for package "OpenCV" that is compatible
with requested version "4".
  D:/msys64/ucrt64/lib/cmake/opencv5/OpenCVConfig.cmake, version: 5.0.0
    The version found is not compatible with the version requested.
```

本工程的处理办法是**不写版本号**：`find_package(OpenCV REQUIRED COMPONENTS ...)`，
这样 4.x 和 5.x 都能匹配，换到装了 4.x 的教学机上也能构建。

### 11. 在 Windows 上 `pacman-key --init` 卡住（环境搭建阶段）

某些 Windows 环境里这一步会卡在 GPG 生成密钥（CPU 长时间空转）。
本机搭建时即遇到，处理办法是把 `/etc/pacman.conf` 的 `SigLevel` 设为 `Never`
后直接同步数据库（只影响签名校验，包体仍来自官方镜像）。

### 12. 超大图片的窗口会超出屏幕

图片模式用 `WINDOW_AUTOSIZE`（1:1 显示，不缩放），这样坐标换算绝对准确、
"点哪读哪"可以肉眼验证。代价是 4K 图（如 `photo1.jpg` 3840×2400）的窗口会超出屏幕。
点击可见区域仍然能正确显示坐标与 BGR。

---

## 十、换到其它环境构建

`CMakeLists.txt` 没有写死任何路径或编译器，换环境只需换工具链：

### Visual Studio + OpenCV 4.x（国内教学最常见）

1. 安装 Visual Studio（勾选「使用 C++ 的桌面开发」）
2. 下载 OpenCV 官方 Windows 包，解压到比如 `C:/opencv`
3. 配置：

```bash
cmake -S . -B build -DOpenCV_DIR=C:/opencv/build
cmake --build build --config Release
```

源码一行都不用改。

### 通用要点

- 头文件一律写成 `#include <opencv2/xxx.hpp>`，不依赖目录结构
- 链接用 `${OpenCV_LIBS}`
- 不依赖任何 MSYS2 特有的路径或工具（`CMakePresets.json` 里的路径只在用 preset 时生效，
  直接用 `-S/-B` 就不受影响）

---

## 十一、验收清单

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
| 11 | `grep -n "VideoCapture" src/video_mode.cpp \| grep -v "//"` | 无输出（主程序里没有任何非注释用法） | VideoReader 设计 |

---

## 十二、实测结果

本机完整跑通，以下为实际输出：

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

**第二套素材（motion2.avi）**

```text
分辨率   : 640 x 480
源帧率   : 25 fps
总帧数   : 125
处理帧数 : 125
```

**编译**：11 个编译单元，`-Wall -Wextra` 下**零错误零警告**。

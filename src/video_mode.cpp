// ============================================================================
//  video_mode.cpp —— 视频模式
//
//  对应题目「二、视频模式（35 分）」和「三、保存结果（20 分）」：
//    1. 判断视频是否成功打开
//    2. 显示视频画面，并在画面上显示帧号、分辨率和处理 FPS
//    3. 按 Esc 可以提前退出；视频自然结束时也能正常退出
//    4. 通过 VideoReader 的公开接口读取，不直接操作 cv::VideoCapture
//    5. 处理结果保存到 output/，目录不存在自动创建；尺寸与输入一致；能正常播放；
//       输出视频里的帧号/分辨率/FPS 与处理画面一致
//
//  注意本文件：
//    - 没有 #include <opencv2/videoio.hpp>
//    - 没有任何 cv::VideoCapture / cv::VideoWriter 字样
//  读视频走 VideoReader，写视频走 VideoSink，两个类各自封装了 videoio 的细节。
//  这样"主程序不直接操作 VideoCapture"就不是靠自觉，而是编译期强制：
//  谁手滑写一个 cv::VideoCapture，这里根本编不过（类型未声明）。
// ============================================================================
#include "video_mode.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

#include "app_common.h"
#include "fps_meter.h"
#include "overlay.h"
#include "video_reader.h"
#include "video_writer.h"

namespace {

const char* kWindowName = "Video Mode - press ESC to quit";
const char* kOutputDir  = "output";

// 由源帧率推算每帧的等待时间（毫秒）。
//
// 为什么要按源帧率来等？
//   不等待的话，程序会以 CPU 能达到的最快速度把视频"播"完——
//   一段 6 秒的视频可能 0.5 秒就放完了，完全看不出画面变化，
//   也没法正常按 Esc。
//
// 为什么要夹在 [1, 100] 之间？
//   下限 1：即使源帧率很高（120fps），也至少给窗口一次刷新机会，
//           同时避免 waitKey(0) 那种"无限等待"的语义混进来。
//   上限 100：如果元数据异常导致算出几千毫秒，程序会看起来像卡死。
const int kMinDelayMs = 1;
const int kMaxDelayMs = 100;

int computeDelayMs(double sourceFps)
{
    const double fps = (sourceFps > 0.0 && sourceFps < 1000.0) ? sourceFps : 30.0;
    int delay = static_cast<int>(1000.0 / fps + 0.5);
    return std::max(kMinDelayMs, std::min(delay, kMaxDelayMs));
}

}  // namespace

// ============================================================================
//  runVideoMode —— 视频模式全流程
// ============================================================================
int runVideoMode(const std::string& videoPath)
{
    // ---- 第 1 步：输入文件是否存在 ----
    if (!app::fileExists(videoPath)) {
        app::printError("错误：找不到输入文件：" + videoPath);
        app::printError("      当前工作目录：" + std::filesystem::current_path().string());
        app::printError("      提示：可以用相对路径（assets/motion.mp4）或绝对路径（D:/v/a.mp4）。");
        return app::kExitNoInput;
    }

    // ---- 第 2 步：打开视频（覆盖"判断视频是否成功打开"）----
    VideoReader reader(videoPath);
    if (!reader.isOpened()) {
        app::printError("错误：无法打开视频文件：" + videoPath);
        app::printError("      可能原因：文件已损坏、扩展名与实际格式不符，");
        app::printError("      或者该视频的编解码器不被当前 OpenCV 构建支持。");
        return app::kExitVideoOpen;
    }

    // ---- 打印视频基本信息 ----
    std::cout << "\n===== 视频模式 =====\n"
              << "视频路径 : " << reader.path() << "\n"
              << "分辨率   : " << reader.frameSize().width << " x " << reader.frameSize().height << "\n"
              << "源帧率   : "
              << ((reader.fps() > 0.0) ? std::to_string(static_cast<int>(reader.fps() + 0.5)) + " fps"
                                       : std::string("未知"))
              << "\n"
              << "总帧数   : "
              << ((reader.totalFrames() > 0) ? std::to_string(reader.totalFrames())
                                             : std::string("未知"))
              << "\n"
              << std::endl;

    // ---- 第 3 步：确定输出分辨率 ----
    //
    // 正常情况下构造函数读到的元数据就够用了。
    // 但极少数容器读不出分辨率，这时就先读一帧出来看看——
    // 这一帧不能丢，得先存进 pendingFrame，稍后照常处理。
    cv::Size outputSize = reader.frameSize();
    cv::Mat  pendingFrame;
    bool     hasPendingFrame = false;

    if (outputSize.width <= 0 || outputSize.height <= 0) {
        if (!reader.readNext(pendingFrame)) {
            app::printError("错误：视频已打开但无法读取第一帧，文件可能已损坏：" + videoPath);
            return app::kExitVideoRead;
        }
        hasPendingFrame = true;
        outputSize = pendingFrame.size();
    }

    // ---- 第 4 步：准备输出文件（覆盖"输出目录自动创建"）----
    //
    // 输出文件主名沿用输入文件名，这样 inputs 和 outputs 一眼能对上。
    // 用 std::filesystem::path::stem() 取"去掉扩展名和目录"的部分，
    // 比手动找最后一个 '.' 可靠（能正确处理 a.b.mp4 这种文件名）。
    std::string baseName = std::filesystem::path(videoPath).stem().string();
    if (baseName.empty()) {
        baseName = "result";
    }

    // 容器帧率用"源帧率"而不是实测处理吞吐。
    // 这两者含义不同：容器帧率决定播放时长，实测吞吐只是性能指标。
    // 用错了的话，输出视频要么快进要么慢放。
    const double containerFps = (reader.fps() > 0.0) ? reader.fps() : 30.0;

    VideoSink sink;
    std::string usedCodecDesc;
    std::string errorText;

    if (!sink.openWithFallback(kOutputDir, baseName, containerFps, outputSize,
                               usedCodecDesc, errorText)) {
        app::printError("错误：无法创建输出视频文件。");
        app::printError("      " + errorText);
        return app::kExitVideoWrite;
    }

    std::cout << "输出文件 : " << sink.outputPath() << "\n"
              << "编码格式 : " << usedCodecDesc << "\n"
              << "容器帧率 : " << static_cast<int>(containerFps + 0.5) << " fps\n"
              << std::endl;

    // ---- 第 5 步：逐帧处理主循环 ----
    const int delayMs = computeDelayMs(reader.fps());

    FpsMeter fpsMeter(30);
    bool userInterrupted = false;

    cv::namedWindow(kWindowName, cv::WINDOW_AUTOSIZE);

    while (true) {
        // --- 5.1 取一帧 ---
        cv::Mat frame;
        if (hasPendingFrame) {
            // 前面为了拿分辨率而预读的那一帧，先处理它
            frame = pendingFrame;
            hasPendingFrame = false;
        }
        else if (!reader.readNext(frame)) {
            // 读到结尾（或者这一帧解码失败）——自然结束
            break;
        }

        // --- 5.2 组装叠加信息 ---
        OverlayInfo info;
        info.frameNumber = reader.frameNumber();   // 1 基，与"已处理帧数"一致
        info.totalFrames = reader.totalFrames();
        info.frameSize   = frame.size();           // 用实际帧尺寸，保证显示的就是真实分辨率
        info.procFps     = fpsMeter.fps();

        // --- 5.3 叠加信息（★ 全流程只调用这一次）---
        //
        // 这一行是整个"保存结果"评分的关键：
        //   画完之后，下面 write() 写入磁盘的和 imshow() 显示到屏幕的，
        //   是同一块 cv::Mat 的同一份像素。
        //   所以"输出视频中显示的信息与处理画面一致"是结构上保证的，
        //   不需要靠两处代码写得一样来碰运气。
        drawOverlay(frame, info);

        // --- 5.4 写入结果视频 ---
        // 注意顺序：先画再写。如果反过来（先写后画），
        // 视频里就没有叠加信息了——这是很容易犯的错。
        sink.write(frame);

        // --- 5.5 显示 ---
        cv::imshow(kWindowName, frame);

        // --- 5.6 等一会儿，同时接收按键 ---
        //
        // waitKey 在这里身兼两职：
        //   1. 按源帧率节流，让播放速度和原视频一致
        //   2. 驱动窗口刷新与事件处理（不调用的话窗口不会重绘）
        // 返回 -1 表示这段时间内没有按键。
        const int key = cv::waitKey(delayMs);

        // 统计放在 waitKey 之后：这样测到的是"端到端吞吐"（含显示和等待），
        // 读数会稳定在源帧率附近，和肉眼看到的速度一致。
        fpsMeter.tick();

        // 27 是 Esc 键的 ASCII 码。
        if (key == 27) {
            userInterrupted = true;
            break;
        }
    }

    // ---- 第 6 步：收尾 ----
    //
    // 必须显式 release：mp4 的索引要在这一步才写进文件。
    // 虽然 VideoSink 的析构函数也会兜底，但这里显式调用，
    // 是为了让"文件此刻就已经写完"这件事在代码上一目了然。
    sink.release();

    cv::destroyAllWindows();

    // ---- 第 7 步：统计输出 ----
    std::cout << "\n===== 处理结束 =====\n"
              << "结束方式 : " << (userInterrupted ? "用户按 Esc 提前退出" : "视频自然播放结束") << "\n"
              << "处理帧数 : " << reader.frameNumber() << "\n"
              << "输出文件 : " << sink.outputPath() << "\n"
              << std::endl;

    return app::kExitOk;
}

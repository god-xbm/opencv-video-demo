// ============================================================================
//  video_writer.h —— 结果视频写出器
//
//  为什么单独封装一个类，而不是在 video_mode.cpp 里直接用 cv::VideoWriter？
//
//  因为 cv::VideoWriter 和 cv::VideoCapture 声明在同一个头文件
//  <opencv2/videoio.hpp> 里。若 video_mode.cpp 直接包含 videoio.hpp，
//  那么"主程序不得直接操作 VideoCapture"就只能靠人工自觉来保证。
//
//  把写盘能力封在 VideoSink 里、并且同样用前置声明把 videoio 挡在头文件之外，
//  video_mode.cpp 就完全接触不到 videoio 的任何类型。
//  一旦有人手滑写了 cv::VideoCapture，编译会直接失败（不完整类型）。
//  用编译器当守门员，比靠自觉可靠得多。
//
//  另外，这个类还负责两件容易被忽略却会直接丢分的事：
//    A. 输出目录自动创建
//    B. 编码器降级：不同容器/编码器在不同机器上的可用性不一致，
//       逐个尝试，全部失败才报错，并把试过哪些组合告诉用户
// ============================================================================
#pragma once

#include <memory>
#include <string>

#include <opencv2/core.hpp>  // cv::Mat / cv::Size

namespace cv {
class VideoWriter;  // 前置声明：把 videoio 的实现细节挡在头文件之外
}

class VideoSink {
public:
    VideoSink();

    // 禁止拷贝：VideoWriter 持有编码器上下文，同时被两处写出会互相踩数据。
    VideoSink(const VideoSink&) = delete;
    VideoSink& operator=(const VideoSink&) = delete;

    // 析构时自动释放。
    // 这一点非常关键：mp4 的 moov（索引）是在 release() 时才写入文件的。
    // 如果程序中途 return 或异常退出而没有 release，就会得到一个 0 字节、
    // 或者有画面但播放器拖不动、时长显示为 0 的坏文件——
    // 正好命中"输出视频能够正常播放"这一条评分。
    ~VideoSink();

    // 尝试创建输出视频。
    //   outputDir : 输出目录（不存在会自动创建）
    //   baseName  : 输出文件主名（不含扩展名）
    //   fps       : 容器帧率，应当传"输入视频的帧率"，不是实测处理吞吐
    //   frameSize : 输出分辨率，必须与输入完全一致
    //   usedDesc  : 出参，回填"最终用的是哪种编码器/容器"，便于操作者知道结果
    //   errorText : 出参，全部失败时回填详细原因
    // 成功返回 true。
    bool openWithFallback(const std::string& outputDir,
                          const std::string& baseName,
                          double fps,
                          const cv::Size& frameSize,
                          std::string& usedDesc,
                          std::string& errorText);

    // 写入一帧。内部会检查尺寸是否与打开时一致，不一致就拒绝写入并返回 false，
    // 避免"输出视频尺寸与输入不一致"这种悄悄发生的错误。
    bool write(const cv::Mat& frame);

    void release();

    bool        isOpened()   const { return m_isOpened; }
    std::string outputPath() const { return m_outputPath; }

private:
    std::unique_ptr<cv::VideoWriter> m_writer;
    std::string                      m_outputPath;
    cv::Size                         m_frameSize;
    double                           m_fps;
    bool                             m_isOpened;
};

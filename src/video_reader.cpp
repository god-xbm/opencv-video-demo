// ============================================================================
//  video_reader.cpp —— VideoReader 的实现
//
//  这里是全工程唯一真正使用 cv::VideoCapture 的地方（另外只在头文件里有一句
//  前置声明）。其它文件里出现的 VideoCapture 字样全部是注释。
// ============================================================================
#include "video_reader.h"

#include <opencv2/videoio.hpp>  // 完整类型只在这一层引入

VideoReader::VideoReader(const std::string& videoPath)
    : m_capture(std::make_unique<cv::VideoCapture>()),
      m_path(videoPath),
      m_frameNumber(0),
      m_totalFrames(-1),
      m_fps(-1.0),
      m_frameSize(0, 0),
      m_isOpened(false)
{
    // 成员初始化列表里把 7 个状态全部显式赋了初值。
    // 这一步看着啰嗦，但很关键：如果漏掉某个成员，它就是未定义值，
    // 后面 isOpened() 返回真、帧号却是垃圾数字，会非常难查。
    //
    // 打开视频。
    // 先用默认后端（CAP_ANY）让 OpenCV 自己挑；失败再显式点名 FFMPEG 试一次。
    // MSYS2 版 OpenCV 编译时关掉了 MSMF，实际能用的就是 FFMPEG，
    // 但"先 AUTO 后指定"的顺序能让它在别的构建上也能工作。
    m_isOpened = m_capture->open(videoPath);

    if (!m_isOpened) {
        m_isOpened = m_capture->open(videoPath, cv::CAP_FFMPEG);
    }

    if (!m_isOpened) {
        // 打开失败：直接把状态保持为"未打开"返回，不抛异常。
        // 让调用方先判断 isOpened() 再决定打印什么提示，
        // 比 try/catch 更直白，也更容易写出精确的报错文案。
        return;
    }

    readMetadata();
}

VideoReader::~VideoReader() = default;

void VideoReader::readMetadata()
{
    // 读帧率。
    //
    // 注意这个兜底判断：OpenCV 5 里，当某个属性不被支持时
    // CAP_PROP_FPS 会返回 -1（4.x 返回 0）。如果不加判断，
    // 后面拿 -1 去算 waitKey 的等待毫秒数，就会得到负数；
    // 传给 VideoWriter 更是直接打不开文件。
    // 所以这里统一按"取值不合理就视为未知"处理，未知用 -1.0 表示。
    const double fps = m_capture->get(cv::CAP_PROP_FPS);
    m_fps = (fps > 0.0 && fps < 1000.0) ? fps : -1.0;

    // 读总帧数。未知时留 -1，显示层会渲染成 "Frame N" 而不是 "Frame N / -1"。
    const double total = m_capture->get(cv::CAP_PROP_FRAME_COUNT);
    m_totalFrames = (total > 0.0) ? static_cast<int>(total + 0.5) : -1;

    // 读分辨率。
    const int width  = static_cast<int>(m_capture->get(cv::CAP_PROP_FRAME_WIDTH));
    const int height = static_cast<int>(m_capture->get(cv::CAP_PROP_FRAME_HEIGHT));
    if (width > 0 && height > 0) {
        m_frameSize = cv::Size(width, height);
    }
}

bool VideoReader::readNext(cv::Mat& frame)
{
    if (!m_isOpened) {
        return false;
    }

    // 先读到临时变量里。
    // 这样做的意义：如果这一帧解码失败，frame 保持原样，
    // 调用方不会拿到一张空图或半张图去显示/写盘。
    cv::Mat temp;
    if (!m_capture->read(temp) || temp.empty()) {
        // 读到末尾，或者这一帧解码失败。
        // 两种情况都算"没有下一帧"，帧号保持不变。
        return false;
    }

    frame = temp;      // 只在确定成功之后，才把帧交给调用方
    ++m_frameNumber;   // ★ 读取成功后更新帧号（题目明确要求的时序）

    // 极少数视频容器读不出分辨率，这里用第一帧补救一下，
    // 保证后续创建 VideoWriter 时尺寸一定有效。
    if (m_frameSize.width <= 0 || m_frameSize.height <= 0) {
        m_frameSize = temp.size();
    }

    return true;
}

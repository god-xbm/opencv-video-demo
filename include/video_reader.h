// ============================================================================
//  video_reader.h —— 视频读取状态管理类
//
//  这是本次作业明确要求的类（占 15 分），设计要点：
//    1. 私有状态：视频对象、当前帧号等全部 private，外部碰不到
//    2. 构造函数：接收路径并打开视频，失败不抛异常，用 isOpened() 查询
//    3. 查询接口：是否打开 / 当前帧号 / 总帧数 / 帧率 / 分辨率
//    4. 读取接口：bool readNext(Mat&)，读取成功后才更新帧号
//    5. 主程序只能通过公开接口读视频，不能直接操作 cv::VideoCapture
//
//  ── 关于第 5 点，这里有个刻意的设计 ──
//
//  cv::VideoCapture 在本头文件里只做了「前置声明」，没有 #include <opencv2/videoio.hpp>，
// 真正的完整类型只出现在 video_reader.cpp 内部。私有成员用 unique_ptr 持有它。
//
// 这样做带来一个硬保证：
//     video_mode.cpp（主程序）只包含本头文件，因此**根本拿不到 cv::VideoCapture 这个类型**。
//     如果有人在主程序里手滑写了 cv::VideoCapture cap;，编译会直接失败（不完整类型）。
//
//  也就是说，"主程序不直接操作 VideoCapture"这件事不是靠开发者自觉遵守，
//  而是被编译器强制执行的。
//
//  检查"主程序里有没有真正使用它"（预期无输出，因为只有注释会提到这个词）：
//      grep -n "VideoCapture" src/video_mode.cpp | grep -v "//"
// ============================================================================
#pragma once

#include <memory>
#include <string>

#include <opencv2/core.hpp>  // cv::Mat / cv::Size（core 模块，不含 videoio）

namespace cv {
class VideoCapture;  // 前置声明：把 videoio 的实现细节挡在头文件之外
}

class VideoReader {
public:
    // 构造函数：接收视频路径，立即尝试打开，并读取后续要用到的元数据。
    //
    // 失败时不抛异常，而是把 isOpened() 置为 false——这样调用方可以先判断、
    // 再决定打印什么提示，比 try/catch 更直白，也更容易写出精确的报错。
    explicit VideoReader(const std::string& videoPath);

    // 析构必须在 .cpp 里定义：unique_ptr 的删除器需要完整类型，
    // 而这里 cv::VideoCapture 只有一个前置声明。
    ~VideoReader();

    // 禁止拷贝：一个视频对象代表一路真实的解码状态，
    // 若被两个对象共享，拷贝出来的那个会跟着推进同一个解码位置，
    // 帧号立刻错乱。直接禁掉，从根上杜绝这类事故。
    VideoReader(const VideoReader&) = delete;
    VideoReader& operator=(const VideoReader&) = delete;

    // ---- 查询接口：全部 const，全部无副作用 ----
    // "无副作用"是很关键的约束：这些函数只是把成员读出来，
    // 绝不读帧、绝不移动解码位置。否则查一次帧号就跳一帧，没法用了。
    bool        isOpened()    const { return m_isOpened; }
    int         frameNumber() const { return m_frameNumber; }
    int         totalFrames() const { return m_totalFrames; }
    double      fps()         const { return m_fps; }
    cv::Size    frameSize()   const { return m_frameSize; }
    std::string path()        const { return m_path; }

    // ---- 读取接口 ----
    // 读取下一帧到 frame。
    //   成功：把帧交给 frame，帧号 +1，返回 true
    //   失败：返回 false，帧号保持不变，frame 内容不被改写
    //
    // 为什么失败时不推进帧号：若 EOF 也 +1，最终帧号会比总帧数多 1，
    // 打印"共处理 N 帧"就多算了。保持"帧号 = 已成功读取的帧数"，
    // 收尾统计才准确。
    bool readNext(cv::Mat& frame);

private:
    // 读取 fps / 总帧数 / 分辨率。构造时调用一次。
    // 做成私有方法，是因为它只服务于构造流程，外部没必要也不应该调用。
    void readMetadata();

    // ---- 私有状态 ----
    // 全部在 private，没有任何 public 成员变量。
    std::unique_ptr<cv::VideoCapture> m_capture;  // 视频对象：外部不可见、不可操作
    std::string                       m_path;     // 原始路径，仅用于错误提示
    int                               m_frameNumber;  // 已成功读取的帧数（因此首帧 = 1）
    int                               m_totalFrames;  // 总帧数，未知时为 -1
    double                            m_fps;          // 帧率，未知时为 -1.0
    cv::Size                          m_frameSize;    // 分辨率，未知时为 (0,0)
    bool                              m_isOpened;     // 是否成功打开
};

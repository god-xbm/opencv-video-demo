// ============================================================================
//  fps_meter.cpp —— 处理帧率统计的实现
// ============================================================================
#include "fps_meter.h"

#include <opencv2/core/utility.hpp>  // cv::getTickCount / cv::getTickFrequency

namespace {

// 取当前时刻（秒，浮点）。
//
// 为什么用 cv::getTickCount 而不是 std::chrono？
//   1. 它是 OpenCV 自带的、跨平台的高精度计时，精度通常优于毫秒
//   2. 工程里已经在用 OpenCV，少引一个头文件、少一种写法，风格更统一
//
// getTickCount() 返回的是"时钟滴答数"，必须除以 getTickFrequency()
// 才换算成秒。直接拿滴答数相减是没有意义的。
double nowSeconds()
{
    return static_cast<double>(cv::getTickCount()) / cv::getTickFrequency();
}

}  // namespace

FpsMeter::FpsMeter(std::size_t windowSize)
    : m_window(windowSize < 2 ? 2 : windowSize),  // 窗口至少 2，否则算不出时间差
      m_lastFps(0.0)
{
}

void FpsMeter::tick()
{
    m_stamps.push_back(nowSeconds());

    // 只保留最近 m_window 个时刻。
    // 用 deque 而不是 vector，是因为要在头部弹出，deque 的 pop_front 是 O(1)。
    while (m_stamps.size() > m_window) {
        m_stamps.pop_front();
    }

    // 只有 1 个时刻时还构不成"时间差"，直接返回。
    // 此时 m_lastFps 保持 0，显示层会渲染成 "--"，
    // 不会出现第一帧就闪一个 0.0 或者 inf 的难看画面。
    if (m_stamps.size() < 2) {
        return;
    }

    const double span = m_stamps.back() - m_stamps.front();
    if (span <= 1e-6) {
        return;   // 时间差小到可以忽略（例如两帧在同一时刻完成），跳过
    }

    // 窗口内的平均帧率：(N 个时刻之间共 N-1 个间隔) / 总时长
    const double instantFps = static_cast<double>(m_stamps.size() - 1) / span;

    // 指数平滑。
    // 新值权重 0.2，旧值 0.8：足够抑制单帧抖动，又不会像全量平均那样迟钝。
    // 第一次有有效值时直接采用，避免从 0 慢慢爬升（那样前几秒的读数都是错的）。
    if (m_lastFps <= 0.0) {
        m_lastFps = instantFps;
    }
    else {
        m_lastFps = 0.8 * m_lastFps + 0.2 * instantFps;
    }
}

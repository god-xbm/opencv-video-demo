// ============================================================================
//  overlay.cpp —— 信息叠加的实现
//
//  这个函数是整个工程里唯一往画面上写字的地方。
//  显示和写盘都调用它、都只调用一次，所以两边的内容必然一致。
// ============================================================================
#include "overlay.h"

#include <algorithm>
#include <cstdio>
#include <string>

#include <opencv2/imgproc.hpp>  // putText / getTextSize / rectangle / addWeighted

namespace {

// 把帧率格式化成 1 位小数；无有效样本时返回 "--"。
//
// 返回 "--" 而不是 "0.0" 是很重要的小细节：
// 首帧还没来得及统计，显示 0.0 会让人以为程序卡住了；
// 显示 "--" 一眼就知道"还没开始算"。
std::string formatFps(double value)
{
    if (value <= 0.0) {
        return "--";
    }
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.1f", value);
    return std::string(buffer);
}

}  // namespace

void drawOverlay(cv::Mat& canvas, const OverlayInfo& info)
{
    // 只处理 8 位 3 通道的彩色画面。
    // 灰色图或浮点图不在这里处理——本程序的处理对象都是 BGR 视频帧。
    if (canvas.empty() || canvas.type() != CV_8UC3) {
        return;
    }

    // ---- 组装三行文字 ----
    char line1[96];
    char line2[96];
    char line3[96];

    if (info.totalFrames > 0) {
        std::snprintf(line1, sizeof(line1), "Frame : %d / %d", info.frameNumber, info.totalFrames);
    }
    else {
        // 总帧数未知时，不显示 "/ -1" 这种没有意义的东西
        std::snprintf(line1, sizeof(line1), "Frame : %d", info.frameNumber);
    }

    std::snprintf(line2, sizeof(line2), "Size  : %d x %d",
                  info.frameSize.width, info.frameSize.height);

    std::snprintf(line3, sizeof(line3), "FPS   : %s", formatFps(info.procFps).c_str());

    const char* lines[3] = { line1, line2, line3 };

    // ---- 自适应字号 ----
    //
    // 按画面高度线性缩放：720p 大约 0.55，1080p 大约 0.83，上限 1.0、下限 0.40。
    // 这样在 640x480 的小视频上字不会糊成一块，在 4K 上也不会小得看不见。
    //
    // 为什么不用固定字号？因为 OpenCV 5 把 putText 的渲染引擎换成了
    // TrueType，同样的 fontScale 在不同版本下占的像素宽度并不相同。
    // 写死一个字号和框宽，换个版本或换种分辨率就会溢出或截断。
    // 所以下面所有尺寸都由 getTextSize() 实际测量后再算。
    const double scale = std::min(1.0, std::max(0.40, info.frameSize.height / 1300.0));
    const int thickness = 1;

    // ---- 实测文字尺寸 ----
    int maxTextWidth = 0;
    int lineHeight = 0;

    for (int i = 0; i < 3; ++i) {
        int baseline = 0;
        const cv::Size textSize =
            cv::getTextSize(lines[i], cv::FONT_HERSHEY_SIMPLEX, scale, thickness, &baseline);
        maxTextWidth = std::max(maxTextWidth, textSize.width);
        lineHeight = std::max(lineHeight, textSize.height);
    }

    if (lineHeight <= 0) {
        lineHeight = static_cast<int>(20 * scale);   // 理论上不会发生，留个保险
    }

    // 内边距与行距也随字号缩放
    const int padding = std::max(6, static_cast<int>(10 * scale));
    const int gap = std::max(4, static_cast<int>(6 * scale));

    // ---- 底板尺寸与位置 ----
    // 放在左上角：绝大多数视频的主体在中央、字幕在底部，
    // 左上角是最不挡事的位置。
    const int boxWidth = maxTextWidth + padding * 2;
    const int boxHeight = padding * 2 + lineHeight * 3 + gap * 2;

    const int originX = 10;
    const int originY = 10;
    const int finalWidth = std::min(boxWidth, canvas.cols - originX - 1);
    const int finalHeight = std::min(boxHeight, canvas.rows - originY - 1);

    // 画面小到连底板都放不下，就干脆不画了（总比画一半越界崩溃好）
    if (finalWidth <= 2 || finalHeight <= 2) {
        return;
    }

    const cv::Rect box(originX, originY, finalWidth, finalHeight);

    // ---- 半透明黑色底板 ----
    //
    // 为什么需要底板？如果画面本身很亮（雪景、白墙、天空），
    // 直接写黄字几乎看不清。加一层 45% 黑底之后，
    // 无论背景多亮，文字都能保持可读。
    //
    // 实现上不能用 rectangle(.., -1) 直接填黑，那样会把画面挖掉一块；
    // 用 addWeighted 把原图和黑色按比例混合，得到的才是"半透明"效果。
    cv::Mat roi = canvas(box);
    cv::Mat black(roi.size(), roi.type(), cv::Scalar(0, 0, 0));
    cv::addWeighted(black, 0.45, roi, 0.55, 0.0, roi);

    // 一圈细边框，让底板边界更清晰
    cv::rectangle(canvas, box, cv::Scalar(200, 200, 200), 1, cv::LINE_8);

    // ---- 写文字 ----
    // 颜色 Scalar(0, 255, 255) 是 BGR 顺序下的"黄"：
    //   B=0, G=255, R=255 → 红+绿 = 黄。
    // 顺便呼应一下本作业"输出 BGR 值"的知识点——OpenCV 里颜色参数永远是 BGR。
    int textY = originY + padding + lineHeight;
    for (int i = 0; i < 3; ++i) {
        cv::putText(canvas,
                    lines[i],
                    cv::Point(originX + padding, textY),
                    cv::FONT_HERSHEY_SIMPLEX,
                    scale,
                    cv::Scalar(0, 255, 255),
                    thickness,
                    cv::LINE_AA);
        textY += lineHeight + gap;
    }
}

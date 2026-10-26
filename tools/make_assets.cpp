// ============================================================================
//  tools/make_assets.cpp —— 测试素材生成器
//
//  为什么需要它？
//  作业要求"提交前使用另一张图片和另一段同类型视频进行测试"，
//  所以必须能稳定产出一批已知内容的素材。这个程序用 OpenCV 自己把图画出来：
//
//    1. calib_bgr.png —— 三色块校准图
//       左上纯红 (B=0,G=0,R=255)、右上纯绿、左下纯蓝。
//       鼠标点红块必须打印 B=0 G=0 R=255，点绿块必须 B=0 G=255 R=0。
//       这样一来，"点击取色"对不对就不是靠感觉，而是有确定答案可以核对。
//
//    2. bars_smpte.png —— 彩色条图
//       一张颜色丰富、贴近真实图片的素材，用来测正常的读图/显示流程。
//
//    3. gen_motion.mp4 —— 一段带明显运动的视频
//       方块从左向右移动，圆形上下摆动，帧号越大位置越靠右。
//       肉眼一眼就能看出"逐帧处理有没有跳帧、有没有卡住"。
//       如果这台机器上视频编码器不可用，这一步会失败，但只警告不退出，
//       因为本程序的主要产出是那两张图。
//
//  构建方式：
//      cmake --build build --target make_assets
//  运行（在工程根目录）：
//      ./build/make_assets.exe assets
// ============================================================================
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

namespace {

// 在指定位置画一行带底色的说明文字
void label(cv::Mat& canvas, const std::string& text, const cv::Point& org, const cv::Scalar& color)
{
    cv::putText(canvas, text, org, cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 3, cv::LINE_AA);
    cv::putText(canvas, text, org, cv::FONT_HERSHEY_SIMPLEX, 0.7, color, 1, cv::LINE_AA);
}

// ---------------------------------------------------------------------------
//  三色块校准图
// ---------------------------------------------------------------------------
bool makeCalibrationImage(const std::string& outputDir)
{
    const int width = 640;
    const int height = 480;

    cv::Mat canvas(height, width, CV_8UC3, cv::Scalar(40, 40, 40));

    // 三个纯色方块。注意 Scalar 的顺序是 BGR：
    //   Scalar(0, 0, 255)   → B=0   G=0   R=255 → 红
    //   Scalar(0, 255, 0)   → B=0   G=255 R=0   → 绿
    //   Scalar(255, 0, 0)   → B=255 G=0   R=0   → 蓝
    struct Block {
        cv::Rect    rect;
        cv::Scalar  color;
        std::string text;
    };

    const Block blocks[] = {
        { cv::Rect(30, 60, 170, 140), cv::Scalar(0, 0, 255), "RED  B=0 G=0 R=255" },
        { cv::Rect(235, 60, 170, 140), cv::Scalar(0, 255, 0), "GREEN B=0 G=255 R=0" },
        { cv::Rect(440, 60, 170, 140), cv::Scalar(255, 0, 0), "BLUE B=255 G=0 R=0" },
    };

    for (const Block& block : blocks) {
        cv::rectangle(canvas, block.rect, block.color, cv::FILLED);
        // 在方块中心写一句白字（带黑描边，任何底色上都看得清）
        int baseline = 0;
        const cv::Size textSize = cv::getTextSize("SAMPLE", cv::FONT_HERSHEY_SIMPLEX, 0.8, 2, &baseline);
        const cv::Point org(block.rect.x + (block.rect.width - textSize.width) / 2,
                            block.rect.y + block.rect.height / 2 + textSize.height / 2);
        cv::putText(canvas, "SAMPLE", org, cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 4, cv::LINE_AA);
        cv::putText(canvas, "SAMPLE", org, cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);
    }

    // 每个方块下面标出预期值，方便点完之后直接核对
    label(canvas, blocks[0].text, cv::Point(30, 235),  cv::Scalar(80, 80, 255));
    label(canvas, blocks[1].text, cv::Point(235, 235), cv::Scalar(80, 255, 80));
    label(canvas, blocks[2].text, cv::Point(440, 235), cv::Scalar(255, 80, 80));

    // 中间一个十字准星，用来验证"点哪读哪"的精度
    cv::line(canvas, cv::Point(width / 2, 280), cv::Point(width / 2, 460), cv::Scalar(255, 255, 255), 1, cv::LINE_AA);
    cv::line(canvas, cv::Point(60, 370),      cv::Point(width - 60, 370),   cv::Scalar(255, 255, 255), 1, cv::LINE_AA);

    label(canvas, "click a block, expect the BGR printed below it", cv::Point(60, 320), cv::Scalar(200, 200, 200));
    label(canvas, "crosshair center = (320, 370)", cv::Point(60, 430), cv::Scalar(200, 200, 200));

    const std::string path = outputDir + "/calib_bgr.png";
    if (!cv::imwrite(path, canvas)) {
        std::cerr << "[失败] 无法写出 " << path << std::endl;
        return false;
    }
    std::cout << "[成功] " << path << "  (" << width << " x " << height << ", 3 通道)" << std::endl;
    return true;
}

// ---------------------------------------------------------------------------
//  彩色条图（模拟真实彩色素材）
// ---------------------------------------------------------------------------
bool makeColorBarsImage(const std::string& outputDir)
{
    const int width = 800;
    const int height = 600;

    cv::Mat canvas(height, width, CV_8UC3, cv::Scalar(0, 0, 0));

    // 上方 2/3 画 7 条标准彩条
    const cv::Scalar bars[] = {
        cv::Scalar(255, 255, 255),  // 白
        cv::Scalar(255, 255, 0),    // 青
        cv::Scalar(255, 0, 255),    // 品红
        cv::Scalar(255, 0, 0),      // 蓝
        cv::Scalar(0, 255, 255),    // 黄
        cv::Scalar(0, 255, 0),      // 绿
        cv::Scalar(0, 0, 255),      // 红
    };

    const int barWidth = width / 7;
    const int barHeight = height * 2 / 3;

    for (int i = 0; i < 7; ++i) {
        const int x = i * barWidth;
        const int w = (i == 6) ? (width - x) : barWidth;
        cv::rectangle(canvas, cv::Rect(x, 0, w, barHeight), bars[i], cv::FILLED);
    }

    // 下方 1/3 画灰阶渐变，用来检验不同亮度下的文字可读性
    for (int x = 0; x < width; ++x) {
        const int gray = static_cast<int>(255.0 * x / (width - 1));
        cv::line(canvas,
                 cv::Point(x, barHeight),
                 cv::Point(x, height - 1),
                 cv::Scalar(gray, gray, gray));
    }

    label(canvas, "SMPTE COLOR BARS 800x600  channels=3", cv::Point(20, barHeight + 50),
          cv::Scalar(0, 0, 0));
    label(canvas, "use this to check imread / imshow / channel count", cv::Point(20, barHeight + 90),
          cv::Scalar(0, 0, 0));

    const std::string path = outputDir + "/bars_smpte.png";
    if (!cv::imwrite(path, canvas)) {
        std::cerr << "[失败] 无法写出 " << path << std::endl;
        return false;
    }
    std::cout << "[成功] " << path << "  (" << width << " x " << height << ", 3 通道)" << std::endl;
    return true;
}

// ---------------------------------------------------------------------------
//  运动视频（编码器不可用时只警告，不影响其它素材）
// ---------------------------------------------------------------------------
bool makeMotionVideo(const std::string& outputDir)
{
    const int    width  = 640;
    const int    height = 480;
    const double fps    = 25.0;
    const int    frames = 150;   // 25fps × 150 帧 = 6 秒

    const std::string path = outputDir + "/gen_motion.mp4";

    cv::VideoWriter writer;
    const int fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
    if (!writer.open(path, fourcc, fps, cv::Size(width, height), true) || !writer.isOpened()) {
        std::cerr << "[警告] 无法创建 " << path
                  << "（当前 OpenCV 缺少可用的视频编码器）。"
                  << "其它素材已生成，可继续做图片模式。" << std::endl;
        return false;
    }

    for (int i = 0; i < frames; ++i) {
        cv::Mat frame(height, width, CV_8UC3, cv::Scalar(30, 30, 35));

        // 网格背景，方便看清运动
        for (int x = 0; x < width; x += 40) {
            cv::line(frame, cv::Point(x, 0), cv::Point(x, height), cv::Scalar(55, 55, 60));
        }
        for (int y = 0; y < height; y += 40) {
            cv::line(frame, cv::Point(0, y), cv::Point(width, y), cv::Scalar(55, 55, 60));
        }

        // 横向移动的方块
        const int boxX = static_cast<int>((width - 60) * (static_cast<double>(i) / (frames - 1)));
        cv::rectangle(frame, cv::Rect(boxX, 60, 60, 60), cv::Scalar(0, 200, 255), cv::FILLED);

        // 上下摆动的圆
        const int circleY = 300 + static_cast<int>(80 * std::sin(i * 0.12));
        cv::circle(frame, cv::Point(width / 2, circleY), 40, cv::Scalar(255, 120, 60), cv::FILLED);

        // 帧号：一眼就能看出解码到第几帧、有没有重复帧
        char text[64];
        std::snprintf(text, sizeof(text), "gen_motion  frame %03d / %03d", i + 1, frames);
        cv::putText(frame, text, cv::Point(20, height - 30),
                    cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(240, 240, 240), 2, cv::LINE_AA);

        writer.write(frame);
    }

    writer.release();
    std::cout << "[成功] " << path << "  (" << width << " x " << height << ", " << fps
              << " fps, " << frames << " 帧)" << std::endl;
    return true;
}

}  // namespace

int main(int argc, char** argv)
{
    // 输出目录：默认 assets，也可以从命令行指定
    const std::string outputDir = (argc >= 2) ? std::string(argv[1]) : std::string("assets");

    std::cout << "生成测试素材到：" << outputDir << "\n" << std::endl;

    bool allOk = true;
    allOk = makeCalibrationImage(outputDir) && allOk;
    allOk = makeColorBarsImage(outputDir) && allOk;

    // 视频生成失败不算致命（可能只是编码器缺失），所以不并入 allOk
    makeMotionVideo(outputDir);

    std::cout << "\n完成。" << std::endl;
    return allOk ? 0 : 1;
}

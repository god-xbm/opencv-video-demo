// ============================================================================
//  image_mode.cpp —— 图片模式
//
//  对应题目「一、图片模式（30 分）」：
//    1. 判断图片是否读取成功
//    2. 显示图片，按键后正常退出
//    3. 输出图片宽度、高度和通道数
//    4. 鼠标点击图片时，输出点击位置的坐标和该像素的 BGR 值
// ============================================================================
#include "image_mode.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

#include "app_common.h"

namespace {

// 鼠标回调需要的上下文。
//
// 为什么用结构体 + userdata，而不是一个全局 cv::Mat？
//   - 全局变量的状态是隐式的，别人读代码时不知道谁改过它，也无法开两个窗口
//   - userdata 是 OpenCV 官方给回调传状态的渠道，作用域清晰
//   - 这里存的是 cv::Mat 的浅拷贝（引用计数），不会复制一整张图的像素
struct ImageViewContext {
    cv::Mat     image;       // 当前显示的图像
    std::string windowName;  // 窗口名（回调里如需重绘用得上）
};

// ---------------------------------------------------------------------------
//  loadImageStrict —— 严格读图
//
//  "严格"的意思是：读不到就一定要说清楚是哪一种读不到。
//  题目要求"输入文件读取失败时，应给出明确提示，而不是继续处理空图像"，
//  所以这个函数绝不返回一张空 Mat，失败必然返回非 0 的退出码。
//
//  返回 app::kExitOk 表示成功，其它值表示具体失败原因。
// ---------------------------------------------------------------------------
int loadImageStrict(const std::string& path, cv::Mat& out)
{
    // 第一道关：文件到底在不在？
    // 先查存在性再 imread，是为了把"路径写错了"和"文件坏了"区分开——
    // 这两种情况的提示完全不同，混在一起会让使用者一头雾水。
    if (!app::fileExists(path)) {
        app::printError("错误：找不到输入文件：" + path);
        app::printError("      当前工作目录：" + std::filesystem::current_path().string());
        app::printError("      提示：可以用相对路径（assets/photo1.jpg）或绝对路径（D:/pics/a.jpg）。");
        return app::kExitNoInput;
    }

    // 第二道关：正常读取
    cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
    if (!image.empty()) {
        out = image;
        return app::kExitOk;
    }

    // 第三道关（保底）：用 C++ 文件流把字节读进来，再交给 imdecode。
    //
    // 为什么需要这一层？OpenCV 内部打开文件用的是窄字符 fopen，
    // 在 Windows 上遇到含中文/日文等非 ANSI 字符的路径时可能打不开，
    // 此时 imread 只会返回空 Mat，并不会告诉你"其实是路径字符的问题"。
    // 改走"标准库读字节 + imdecode"，就绕开了这个限制。
    std::ifstream ifs(path, std::ios::binary);
    if (ifs) {
        std::vector<unsigned char> buffer((std::istreambuf_iterator<char>(ifs)),
                                          std::istreambuf_iterator<char>());
        if (!buffer.empty()) {
            image = cv::imdecode(buffer, cv::IMREAD_COLOR);
            if (!image.empty()) {
                out = image;
                return app::kExitOk;
            }
        }
    }

    app::printError("错误：文件存在但无法解码为图片：" + path);
    app::printError("      可能原因：文件已损坏、扩展名与实际格式不符，或该格式不被当前 OpenCV 支持。");
    return app::kExitImageDecode;
}

// ---------------------------------------------------------------------------
//  onMouse —— 鼠标回调
//
//  OpenCV 的鼠标回调必须是"普通函数指针"，不能是带捕获的 lambda，
//  所以要拿到图像数据，只能靠最后一个 userdata 参数。
// ---------------------------------------------------------------------------
void onMouse(int event, int x, int y, int /*flags*/, void* userdata)
{
    auto* ctx = static_cast<ImageViewContext*>(userdata);
    if (ctx == nullptr || ctx->image.empty()) {
        return;
    }

    // 只关心"左键按下"。鼠标移动也会触发回调，不筛掉的话会刷屏。
    if (event != cv::EVENT_LBUTTONDOWN) {
        return;
    }

    // 越界保护。
    // 窗口如果被拉伸或拖动过，坐标有可能落在图像范围之外，
    // 此时直接 at() 取像素就是未定义行为（可能读到垃圾值，也可能崩溃）。
    if (x < 0 || y < 0 || x >= ctx->image.cols || y >= ctx->image.rows) {
        std::cout << "[提示] 点击位置 (x=" << x << ", y=" << y
                  << ") 超出图像范围 [" << ctx->image.cols << " x " << ctx->image.rows
                  << "]，已忽略。" << std::endl;
        return;
    }

    // 取像素。
    //
    // 关键点 1：at<T> 的参数顺序是 (行, 列)，也就是 (y, x)，不是 (x, y)。
    //          写反了不会报错，但读出来的是另一个像素——这是最常见的坑。
    // 关键点 2：Vec3b 的内存顺序就是 B、G、R，和 OpenCV 的默认彩色顺序一致，
    //          所以 bgr[0]=蓝、bgr[1]=绿、bgr[2]=红。题目要的正是这个顺序。
    // 关键点 3：必须转成 int 再打印。uchar 直接输出会被当成字符，
    //          255 打印出来是一个奇怪的符号而不是数字。
    const int channels = ctx->image.channels();
    if (channels == 3) {
        const cv::Vec3b bgr = ctx->image.at<cv::Vec3b>(y, x);
        std::cout << "[点击] 坐标 (x=" << x << ", y=" << y << ")"
                  << "  B=" << static_cast<int>(bgr[0])
                  << "  G=" << static_cast<int>(bgr[1])
                  << "  R=" << static_cast<int>(bgr[2])
                  << std::endl;
    }
    else if (channels == 1) {
        const int gray = static_cast<int>(ctx->image.at<uchar>(y, x));
        std::cout << "[点击] 坐标 (x=" << x << ", y=" << y << ")"
                  << "  灰度=" << gray << "  (单通道图像，无 BGR)" << std::endl;
    }
    else {
        std::cout << "[点击] 坐标 (x=" << x << ", y=" << y << ")"
                  << "  该图像为 " << channels << " 通道，暂不支持取 BGR。" << std::endl;
    }
}

// 把通道数翻译成人话，方便确认 imread 到底按什么方式解码的
const char* describeChannels(int channels)
{
    switch (channels) {
        case 1: return "单通道（灰度图）";
        case 3: return "三通道（BGR 彩色图）";
        case 4: return "四通道（BGRA，含透明通道）";
        default: return "其它";
    }
}

}  // namespace

// ============================================================================
//  runImageMode —— 图片模式全流程
// ============================================================================
int runImageMode(const std::string& imagePath)
{
    // ---- 第 1 步：严格读图（覆盖"判断图片是否读取成功"）----
    cv::Mat image;
    const int loadResult = loadImageStrict(imagePath, image);
    if (loadResult != app::kExitOk) {
        return loadResult;   // 具体原因已经打印过了，直接把退出码往上抛
    }

    // ---- 第 2 步：输出宽度、高度、通道数 ----
    std::cout << "\n===== 图片模式 =====\n"
              << "图片路径 : " << imagePath << "\n"
              << "宽度     : " << image.cols << " 像素\n"
              << "高度     : " << image.rows << " 像素\n"
              << "通道数   : " << image.channels() << "  (" << describeChannels(image.channels()) << ")\n"
              << "像素总数 : " << static_cast<long long>(image.cols) * image.rows << "\n"
              << std::endl;

    // ---- 第 3 步：建窗口、挂鼠标回调、显示 ----
    //
    // 上下文放在栈上，生命周期覆盖整个 waitKey(0) 阻塞期间，
    // 所以回调里拿到指针是安全的。
    ImageViewContext ctx;
    ctx.image = image;
    ctx.windowName = "Image Mode - left click to inspect BGR, any key to quit";

    // WINDOW_AUTOSIZE：窗口尺寸严格等于图像尺寸，不缩放。
    //
    // 这一点很重要：如果允许窗口缩放，画面会被拉伸，那么鼠标点到的
    // 窗口坐标和真实像素坐标就不是一回事了。OpenCV 内部虽然会做换算，
    // 但 1:1 显示是最不容易出岔子的做法，也让"点哪读哪"可以肉眼验证。
    cv::namedWindow(ctx.windowName, cv::WINDOW_AUTOSIZE);
    cv::setMouseCallback(ctx.windowName, onMouse, &ctx);
    cv::imshow(ctx.windowName, image);

    std::cout << "窗口已打开。\n"
              << "  - 用鼠标左键点击画面任意位置，控制台会打印该点坐标与 BGR 值\n"
              << "  - 按任意键退出\n"
              << std::endl;

    // ---- 第 4 步：等待按键 ----
    // waitKey(0) 表示无限等待，直到有按键事件。
    // 它同时也是 OpenCV 处理窗口消息（重绘、鼠标事件）的时机，
    // 没有它的话，窗口会是一片空白、鼠标回调也不会被触发。
    cv::waitKey(0);

    // ---- 第 5 步：清理 ----
    // 销毁窗口并解绑回调，避免析构顺序问题（ctx 在栈上先于窗口销毁）。
    cv::destroyAllWindows();

    std::cout << "图片模式结束。" << std::endl;
    return app::kExitOk;
}

// ============================================================================
//  tools/env_check.cpp —— 环境自检工具
//
//  这个程序的作用：在写任何业务代码之前，先确认三件事是真的能用。
//
//    1. OpenCV 到底编进 exe 没有、是哪个版本
//    2. 图形界面后端（HighGUI）到底是什么 —— 这决定了 imshow 能不能弹窗
//    3. 视频编解码支持（FFMPEG）有没有 —— 这决定了能不能读写 mp4/avi
//
//  为什么第 2 点要单独查？
//  因为 MSYS2 版的 OpenCV 是用 Qt6 做窗口后端的，并且 qt6 只是"可选依赖"。
//  如果只装了 opencv 没装 qt6，出现的现象是：
//      编译通过  →  运行不报错  →  但窗口根本不出来
//  这是最难查的一类失败（因为没有任何错误信息）。所以必须先验证。
//
//  运行方式（在工程根目录）：
//      ./build/env_check.exe
//
//  预期结果：
//      - 打印出 OpenCV 版本
//      - GUI 一行显示 QT6（或其它非 NONE 的值）
//      - FFMPEG: YES
//      - 弹出一个绿色窗口，里面有文字，按任意键关闭
// ============================================================================
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/core/utility.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

namespace {

// 把多行文本按行拆开
std::vector<std::string> splitLines(const std::string& text)
{
    std::vector<std::string> lines;
    std::string current;
    for (char ch : text) {
        if (ch == '\n') {
            lines.push_back(current);
            current.clear();
        }
        else if (ch != '\r') {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        lines.push_back(current);
    }
    return lines;
}

// 只打印我们关心的那几行构建信息。
// 完整的 getBuildInformation() 有上百行，全打出来反而找不到关键信息。
void printKeyBuildInfo()
{
    const std::vector<std::string> keys = {
        "GUI:", "VTK:", "QT:", "Win32 UI:", "GTK",
        "FFMPEG:", "Video I/O:", "avcodec", "avformat",
        "Media I/O:", "Version control:"
    };

    const std::string info = cv::getBuildInformation();
    for (const std::string& line : splitLines(info)) {
        for (const std::string& key : keys) {
            if (line.find(key) != std::string::npos) {
                std::cout << line << std::endl;
                break;
            }
        }
    }
}

}  // namespace

int main()
{
    std::cout << "========================================\n"
              << "   OpenCV 环境自检\n"
              << "========================================\n"
              << "OpenCV 版本 : " << CV_VERSION << "\n"
              << "支持 C++ 标准: " << __cplusplus << "\n"
              << std::endl;

    std::cout << "--- 构建信息（关键行）---" << std::endl;
    printKeyBuildInfo();
    std::cout << std::endl;

    // ---- 检查视频编解码 ----
    std::cout << "--- 视频编解码支持 ---" << std::endl;
    {
        // 不需要真的打开文件，只要能构造出 VideoCapture 就说明 videoio 编进来了
        cv::VideoCapture probe;
        std::cout << "VideoCapture 可构造 : " << (probe.isOpened() ? "已打开(异常)" : "是") << std::endl;
        cv::VideoWriter probeWriter;
        std::cout << "VideoWriter  可构造 : " << (probeWriter.isOpened() ? "已打开(异常)" : "是") << std::endl;
    }
    std::cout << std::endl;

    // ---- 检查图形界面 ----
    std::cout << "--- 图形界面检查 ---" << std::endl;
    std::cout << "接下来会弹出一个绿色窗口，请确认它真的出现了。" << std::endl;
    std::cout << "如果没有窗口弹出、程序直接结束或报错，说明 HighGUI 后端不可用。" << std::endl;

    try {
        const std::string windowName = "env_check - 如果你看到这个窗口，GUI 就是好的";

        cv::Mat canvas(360, 640, CV_8UC3, cv::Scalar(40, 140, 40));  // BGR: 绿
        cv::putText(canvas, "GUI OK", cv::Point(40, 110),
                    cv::FONT_HERSHEY_SIMPLEX, 2.2, cv::Scalar(0, 0, 0), 6, cv::LINE_AA);
        cv::putText(canvas, "GUI OK", cv::Point(40, 110),
                    cv::FONT_HERSHEY_SIMPLEX, 2.2, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);

        const std::string versionText = std::string("OpenCV ") + CV_VERSION;
        cv::putText(canvas, versionText, cv::Point(40, 210),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 4, cv::LINE_AA);
        cv::putText(canvas, versionText, cv::Point(40, 210),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(255, 255, 255), 2, cv::LINE_AA);

        cv::putText(canvas, "press any key to quit", cv::Point(40, 300),
                    cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(220, 220, 220), 2, cv::LINE_AA);

        cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);
        cv::imshow(windowName, canvas);

        std::cout << "\n窗口已请求显示。请按键关闭窗口..." << std::endl;
        cv::waitKey(0);
        cv::destroyAllWindows();

        std::cout << "[通过] 图形界面后端工作正常。" << std::endl;
    }
    catch (const cv::Exception& e) {
        std::cerr << "\n[失败] 图形界面不可用！" << std::endl;
        std::cerr << "       OpenCV 异常信息：" << e.what() << std::endl;
        std::cerr << "\n       常见原因与处理办法：" << std::endl;
        std::cerr << "       1) 缺少 Qt6 运行库：在 MSYS2 UCRT64 终端执行" << std::endl;
        std::cerr << "          pacman -S mingw-w64-ucrt-x86_64-qt6-base mingw-w64-ucrt-x86_64-qt6-5compat" << std::endl;
        std::cerr << "       2) 在 MSYS2 UCRT64 终端里运行本程序（而不是在别处双击）" << std::endl;
        return 1;
    }

    std::cout << "\n========================================\n"
              << "   环境自检结束\n"
              << "========================================" << std::endl;

    return 0;
}

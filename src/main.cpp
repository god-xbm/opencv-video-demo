// ============================================================================
//  main.cpp —— 程序入口
//
//  本文件的职责只有一个：看命令行说了什么，然后把活儿交给对应的模式。
//  这里不读图、不读视频、不开窗口——所有实际工作都在 image_mode / video_mode 里。
//
//  这样做的好处：老师/同学想看"图片模式怎么实现"，直接打开 image_mode.cpp，
//  不会被一堆命令行解析代码打断。而且以后要加第三种模式（比如摄像头实时模式），
//  只在这里多加一个 if 分支即可，其它文件一个字都不用改。
// ============================================================================
#include <iostream>
#include <string>

#include <opencv2/core.hpp>  // 仅用于 catch (const cv::Exception&)，不调用任何功能

#include "app_common.h"
#include "image_mode.h"
#include "video_mode.h"

namespace {

void printUsage()
{
    std::cout
        << "\n===== 视觉素材查看与标注程序 (cv_hw) =====\n\n"
        << "用法:\n"
        << "  cv_hw img   <图片路径>   图片模式\n"
        << "  cv_hw video <视频路径>   视频模式\n"
        << "  cv_hw --help             显示本帮助\n\n"
        << "示例:\n"
        << "  cv_hw img   assets/calib_bgr.png\n"
        << "  cv_hw img   assets/photo1.jpg\n"
        << "  cv_hw video assets/motion.mp4\n"
        << "  cv_hw video assets/motion2.avi\n\n"
        << "操作说明:\n"
        << "  图片模式 —— 鼠标左键点击画面，打印该点坐标与像素 BGR 值；按任意键退出。\n"
        << "  视频模式 —— 画面左上角实时显示帧号/分辨率/处理 FPS；按 Esc 提前退出；\n"
        << "              视频自然播放结束时自动退出；处理结果同时保存到 output/ 目录。\n\n"
        << "退出码:\n"
        << "  0 成功            2 参数错误        3 输入文件不存在\n"
        << "  4 图片解码失败    5 视频打不开      6 首帧读取失败\n"
        << "  7 输出目录创建失败 8 输出视频创建失败 9 未捕获的 OpenCV 异常\n"
        << std::endl;
}

}  // namespace

int main(int argc, char** argv)
{
    // 第一件事就是把控制台代码页切成 UTF-8。
    // 必须抢在所有中文输出之前，否则 GBK 控制台会把下面的中文全打印成乱码。
    app::enableUtf8Console();

    // 只有 --help 一个参数：打印帮助后正常退出
    if (argc == 2) {
        const std::string first = argv[1];
        if (first == "--help" || first == "-h" || first == "/?") {
            printUsage();
            return app::kExitOk;
        }
    }

    // 严格校验参数个数：既不是 1 个也不是 3 个（程序名 + 模式 + 路径），一律报用法。
    // 严格校验而不是"忽略多余参数"，是为了避免"多打了个路径却静默只处理第一个"
    // 这类很难排查的困惑。
    if (argc != 3) {
        app::printError("错误：参数个数不正确。");
        printUsage();
        return app::kExitBadUsage;
    }

    const std::string mode = argv[1];
    const std::string path = argv[2];

    // 整个流程套一层 try/catch。
    //
    // 原因：OpenCV 在遇到"窗口系统不可用""编解码器缺失"这类问题时是抛异常的，
    // 不捕获的话程序会直接 abort，终端上只剩一句 terminate called after throwing
    // an instance of 'cv::Exception'，看不到任何中文提示——恰好违背了题目
    // "应给出明确提示"的要求。
    try {
        if (mode == "img") {
            return runImageMode(path);
        }
        if (mode == "video") {
            return runVideoMode(path);
        }

        app::printError("错误：未知模式 '" + mode + "'，只支持 img 或 video。");
        printUsage();
        return app::kExitBadUsage;
    }
    catch (const cv::Exception& e) {
        app::printError("错误：OpenCV 运行时异常。");
        app::printError(std::string("      ") + e.what());
        app::printError("      常见原因：当前 OpenCV 构建缺少图形界面后端（窗口弹不出来）、");
        app::printError("      或缺少对应的视频编解码器。");
        return app::kExitUnhandled;
    }
    catch (const std::exception& e) {
        app::printError(std::string("错误：发生标准库异常：") + e.what());
        return app::kExitUnhandled;
    }
}

// ============================================================================
//  app_common.h —— 全工程共用的基础设施
//
//  职责：退出码约定、控制台编码、统一的提示输出、目录与文件检查。
//  这些是与 OpenCV 无关的"杂事"，集中放一处，各模块不必重复实现。
// ============================================================================
#pragma once

#include <string>

namespace app {

// ---------------------------------------------------------------------------
// 退出码约定
//
// 为什么要有退出码：题目要求"输入文件读取失败时给出明确提示"。
// 光有文字提示还不够——用一个固定的数字把"哪一类失败"告诉调用方，
// 无论是人工排查还是写脚本做自动化测试，都能一眼区分。
// ---------------------------------------------------------------------------
constexpr int kExitOk          = 0;  // 正常结束
constexpr int kExitBadUsage    = 2;  // 命令行参数不对
constexpr int kExitNoInput     = 3;  // 输入文件不存在
constexpr int kExitImageDecode = 4;  // 文件在，但图片解码失败
constexpr int kExitVideoOpen   = 5;  // 视频打不开
constexpr int kExitVideoRead   = 6;  // 视频打开了但读不出第一帧
constexpr int kExitOutputDir   = 7;  // 输出目录创建失败
constexpr int kExitVideoWrite  = 8;  // 输出视频创建失败（编码器全不可用）
constexpr int kExitUnhandled   = 9;  // 未捕获的 OpenCV 异常

// 把控制台代码页切到 UTF-8。
// 必须在任何中文输出之前调用，否则 Windows 的 GBK 控制台会把 UTF-8 字节
// 当成乱码显示。在非 Windows 平台上是空操作。
void enableUtf8Console();

// 普通信息 / 错误信息。分开两个函数，是为了让错误统一走 stderr，
// 便于重定向（例如 ./cv_hw video x.mp4 2> err.log 只抓错误）。
void printInfo(const std::string& msg);
void printError(const std::string& msg);

// 确保目录存在（不存在就创建，含多级）。成功返回 true。
// 对应题目"输出目录不存在时自动创建"这一条。
bool ensureDirectory(const std::string& dir);

// 判断给定路径是否是一个存在的普通文件。
// 单独写成函数，是为了在"文件不存在"和"文件存在但内容坏了"之间
// 给出不同的话术——这两类失败的原因完全不同，提示也不该一样。
bool fileExists(const std::string& path);

}  // namespace app

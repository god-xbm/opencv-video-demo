// ============================================================================
//  app_common.cpp —— 基础设施实现
// ============================================================================
#include "app_common.h"

#include <filesystem>
#include <iostream>

#ifdef _WIN32
// NOMINMAX 必须在 windows.h 之前定义。
// 否则 windows.h 会定义 min/max 宏，把 <algorithm> 里的 std::min/std::max
// 一并替换掉，编译时报出一堆莫名奇妙的错误（这是 Windows 上的经典陷阱）。
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace app {

void enableUtf8Console()
{
#ifdef _WIN32
    // 把控制台输入/输出代码页都切成 UTF-8（65001）。
    //
    // 背景：源代码是 UTF-8 保存的，GCC 默认也按 UTF-8 生成执行字符集，
    // 所以中文字符串在 exe 里就是一段 UTF-8 字节。而 Windows 控制台默认
    // 是 GBK（936）代码页，直接打印就成乱码。切代码页是成本最低的解法，
    // 不需要把源码改成 GBK，也不需要 wcout + imbue 那一套。
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
}

void printInfo(const std::string& msg)
{
    std::cout << msg << std::endl;
}

void printError(const std::string& msg)
{
    std::cerr << msg << std::endl;
}

bool ensureDirectory(const std::string& dir)
{
    // 用 std::filesystem 而不是 _mkdir / system("mkdir")：
    //   - 支持多级目录一次创建（output/a/b 这种）
    //   - 不依赖平台命令，也不用担心路径里的空格
    //   - 用 error_code 重载，出错不抛异常，返回 false 即可，便于统一提示
    std::error_code ec;

    if (std::filesystem::exists(dir, ec)) {
        return std::filesystem::is_directory(dir, ec);
    }
    return std::filesystem::create_directories(dir, ec) && !ec;
}

bool fileExists(const std::string& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

}  // namespace app

# ============================================================================
#  CMakeLists.txt —— 唯一构建入口
#
#  用法（详见 README.md）：
#      cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#      cmake --build build
#
#  说明：本项目只使用 OpenCV 的核心模块，写法在 OpenCV 4.x 与 5.x 下通用。
# ============================================================================

# 最低版本 3.16；策略版本上限 4.0。
# 之所以写成 "3.16...4.0"：CMake 4.x 已经移除了 3.5 以下策略版本的兼容，
# 写死 3.10 之类会在新版 CMake 上直接报错。范围写法让新旧 CMake 都能接受。
cmake_minimum_required(VERSION 3.16...4.0)

project(cv_hw LANGUAGES CXX)

# ---- C++ 标准 ----------------------------------------------------------------
# 固定 C++17：std::filesystem、结构化绑定等特性都要用到；
# OpenCV 5 的 CMake 配置自身也要求 cxx_std_17。
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 单配置生成器（Ninja / MinGW Makefiles）默认给 Release。
# 不加这句的话 CMAKE_BUILD_TYPE 为空 → 不优化 → 视频处理帧率低得离谱，
# 会让人误以为代码写错了。
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---- 找 OpenCV ---------------------------------------------------------------
# 这里**故意不写版本号**。
#
# 为什么不能写 find_package(OpenCV 4 ...)？
#   很多人以为那表示"版本 >= 4"，其实不是。CMake 的版本参数意思是
#   "请求 4.x 的兼容版本"，而 OpenCV 的 config 文件用的是 SameMajorVersion
#   策略——要求主版本号完全相同。所以写 4 会直接拒绝 5.0.0，报出：
#       The version found is not compatible with the version requested
#   本项目实测就踩到了这个坑（本机是 OpenCV 5.0.0-7）。
#
#   不写版本号则任何版本都能匹配，OpenCV 4.x 和 5.x 都能用，
#   换到装了 4.x 的教学机上也一样能构建。
#
# 如果 CMake 找不到（多见于手工指定安装位置），可以在命令行补：
#     -DOpenCV_DIR=<OpenCVConfig.cmake 所在目录>
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui videoio)

message(STATUS "OpenCV 版本 : ${OpenCV_VERSION}")
message(STATUS "OpenCV 路径 : ${OpenCV_DIR}")

# ---- 主程序 ------------------------------------------------------------------
set(APP_SOURCES
    src/main.cpp
    src/app_common.cpp
    src/image_mode.cpp
    src/video_mode.cpp
    src/video_reader.cpp
    src/video_writer.cpp
    src/fps_meter.cpp
    src/overlay.cpp
)

add_executable(cv_hw ${APP_SOURCES})

target_include_directories(cv_hw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 用 OpenCV_LIBS 而不是 OpenCV_LIBRARIES：前者是官方文档化变量，
# 其元素是 imported target，能自动带上 include 目录与编译特性。
target_link_libraries(cv_hw PRIVATE ${OpenCV_LIBS})

if(MSVC)
    target_compile_options(cv_hw PRIVATE /W4)
else()
    target_compile_options(cv_hw PRIVATE -Wall -Wextra)
endif()

# 构建完顺手把 output/ 建好，省得第一次运行时才由程序创建（也让演示更顺）。
add_custom_command(TARGET cv_hw POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:cv_hw>/output"
    COMMENT "确保 output 目录存在"
)

# ---- 辅助工具：生成测试素材 --------------------------------------------------
# 不参与默认构建流程，但可以用 cmake --build build --target make_assets 单独构建。
# 作用：用 OpenCV 自己画出"三色块校准图"，鼠标取色是否点哪读哪，一测便知。
add_executable(make_assets tools/make_assets.cpp)
target_link_libraries(make_assets PRIVATE ${OpenCV_LIBS})

# ---- 辅助工具：环境自检 ------------------------------------------------------
# 先跑它再跑主程序：确认 OpenCV 版本、GUI 后端、视频编解码三件事都正常。
#   cmake --build build --target env_check
#   ./build/env_check.exe
add_executable(env_check tools/env_check.cpp)
target_link_libraries(env_check PRIVATE ${OpenCV_LIBS})

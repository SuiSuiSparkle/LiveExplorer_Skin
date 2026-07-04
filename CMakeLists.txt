cmake_minimum_required(VERSION 3.10)
project(LiveExplorer_Skin)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加可执行文件
# WIN32 参数告诉 CMake 这是一个 GUI 应用程序（不是控制台程序），对应 -mwindows
# 添加 .rc 资源文件以包含图标
add_executable(liveexplorer_skin WIN32 liveexplorer_skin.cpp liveexplorer_skin.rc)

# 链接必要的库
# user32: 窗口管理
# gdi32: 图形设备接口
# gdiplus: GDI+ 图形库
# winmm: 多媒体定时器 (timeBeginPeriod)
target_link_libraries(liveexplorer_skin 
    PRIVATE 
    gdiplus 
    gdi32 
    user32
    winmm
)

# 针对 MSVC 的特定设置（如果需要）
if(MSVC)
    # 确保字符集为 Unicode
    add_compile_definitions(UNICODE _UNICODE)
endif()

# 构建后自动复制图片文件夹到输出目录
# 同时也复制生成的 exe 到项目根目录，方便直接运行
add_custom_command(TARGET liveexplorer_skin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/image"
    "$<TARGET_FILE_DIR:liveexplorer_skin>/image"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:liveexplorer_skin>"
    "${CMAKE_SOURCE_DIR}/liveexplorer_skin.exe"
    COMMENT "Copying assets and executable..."
)
cmake_minimum_required(VERSION 3.16)
project(LiveExplorer_Skin VERSION 1.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Network QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 COMPONENTS Core Gui Widgets Network REQUIRED)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/Config.cpp
    src/NativeOverlay.cpp
    src/Controller.cpp
    src/SettingsDialog.cpp
)

set(HEADERS
    src/Config.h
    src/NativeOverlay.h
    src/Controller.h
    src/SettingsDialog.h
)

add_executable(LiveExplorer_Skin WIN32 ${SOURCES} ${HEADERS} ${CMAKE_BINARY_DIR}/filemngr_bgimage_res.o)
# add_executable(LiveExplorer_Skin WIN32 ${SOURCES} ${HEADERS} filemngr_bgimage.rc)

if(Qt6_FOUND)
    target_link_libraries(LiveExplorer_Skin PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Network)
else()
    target_link_libraries(LiveExplorer_Skin PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Network)
endif()

# Windows libraries
target_link_libraries(LiveExplorer_Skin PRIVATE dwmapi user32 gdi32 gdiplus winmm)

# Copy image folder (handled by build script now due to path issues with '&')
# add_custom_command(TARGET filemngr_bgimage POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     "${CMAKE_SOURCE_DIR}/image"
#     "$<TARGET_FILE_DIR:filemngr_bgimage>/image"
#     COMMENT "Copying images..."
# )
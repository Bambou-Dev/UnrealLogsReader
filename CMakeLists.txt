cmake_minimum_required(VERSION 3.16)
project(UnrealLogsReader)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Fetch deps ---
include(FetchContent)
FetchContent_Declare(
    glfw GIT_REPOSITORY https://github.com/glfw/glfw GIT_TAG 3.3.8
)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
    nfd GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git GIT_TAG v1.1.1
)
FetchContent_MakeAvailable(nfd)

# --- ImGui sources ---
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(UnrealLogsReader src/main.cpp ${IMGUI_SOURCES})
target_include_directories(UnrealLogsReader PRIVATE ${IMGUI_DIR} ${IMGUI_DIR}/backends)

# --- Cross-platform OpenGL link ---
if(WIN32)
    target_link_libraries(UnrealLogsReader PRIVATE glfw nfd opengl32)
elseif(UNIX AND NOT APPLE)
    find_package(OpenGL REQUIRED)
    target_link_libraries(UnrealLogsReader PRIVATE glfw nfd OpenGL::GL dl pthread)
elseif(APPLE)
    find_library(COCOA_LIBRARY Cocoa REQUIRED)
    find_library(IOKIT_LIBRARY IOKit REQUIRED)
    find_library(COREVIDEO_LIBRARY CoreVideo REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    target_link_libraries(UnrealLogsReader PRIVATE glfw nfd ${COCOA_LIBRARY} ${IOKIT_LIBRARY} ${COREVIDEO_LIBRARY} ${OPENGL_FRAMEWORK})
endif()

# --- Windows entry point fix ---
if(MSVC)
    set_target_properties(UnrealLogsReader PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

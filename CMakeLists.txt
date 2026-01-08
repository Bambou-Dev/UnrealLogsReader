cmake_minimum_required(VERSION 4.1)
project(UnrealLogsReader)

set(CMAKE_CXX_STANDARD 26)

# --- 1. Fetch GLFW automatically ---
include(FetchContent)
FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw
        GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# --- Fetch Native File Dialog (NFD) ---
FetchContent_Declare(
        nfd
        GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended.git
        GIT_TAG        v1.1.1
)
FetchContent_MakeAvailable(nfd)

include_directories(src external/imgui)

# --- 2. Setup ImGui Sources ---
# We need the core files AND the specific backends for GLFW and OpenGL3
set(IMGUI_DIR "${CMAKE_SOURCE_DIR}/external/imgui")

# DEBUG: Check if the path is correct
if(NOT EXISTS "${IMGUI_DIR}/imgui.cpp")
    message(FATAL_ERROR "ERROR: Could not find imgui.cpp! \nExpected it at: ${IMGUI_DIR}/imgui.cpp \nPlease check if you extracted ImGui into a subfolder inside vendor/imgui.")
endif()

set(IMGUI_SOURCES
        ${IMGUI_DIR}/imgui.cpp
        ${IMGUI_DIR}/imgui_demo.cpp
        ${IMGUI_DIR}/imgui_draw.cpp
        ${IMGUI_DIR}/imgui_tables.cpp
        ${IMGUI_DIR}/imgui_widgets.cpp
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

add_executable(UnrealLogsReader WIN32 src/main.cpp ${IMGUI_SOURCES})

if(MSVC)
    set_target_properties(UnrealLogsReader PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()

# --- 4. Include Directories ---
target_include_directories(UnrealLogsReader PRIVATE
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
)

# --- 5. Link Libraries ---
# Link against GLFW and OpenGL
target_link_libraries(UnrealLogsReader PRIVATE glfw nfd)

# Fix for Windows linking (if you are on Windows)
if(WIN32)
    target_link_libraries(UnrealLogsReader PRIVATE opengl32)
endif()
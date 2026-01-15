# Unreal Logs Reader

A simple, cross-platform desktop tool to explore and analyze Unreal Engine log files with an intuitive graphical interface.

## Features

- **Load and parse** Unreal Engine `.log` and `.txt` files
- **Filter logs** by level (Errors, Warnings, Display messages)
- **Filter by category** (LogCook, LogTemp, etc.)
- **Search** through logs with case-insensitive text search
- **Hide duplicates** to focus on unique log entries
- **Context inspector** shows surrounding log lines for better understanding
- **Multi-select** logs with Ctrl+Click and Shift+Click
- **Copy to clipboard** with Ctrl+C (formats with markdown code blocks)
- **Syntax highlighting** by log level (red for errors, yellow for warnings)
- **Modern dark theme** interface

## Windows Setup

### Step 1: Install Git

1. Download Git from https://git-scm.com/download/windows
2. Run the installer
3. Use default settings (just click "Next" through the installer)
4. Verify installation by opening **Command Prompt** and typing:
   ```cmd
   git --version
   ```
   You should see something like `git version 2.x.x`

### Step 2: Install CMake

CMake is a tool that prepares the code for building.

1. Download CMake from https://cmake.org/download/
2. Choose the **Windows x64 Installer** (`.msi` file)
3. Run the installer
4. **Important**: Select "Add CMake to the system PATH" during installation
5. Verify installation by opening a **new** Command Prompt and typing:
   ```cmd
   cmake --version
   ```
   You should see `cmake version 3.x.x` or higher

### Step 3: Install Visual Studio Build Tools

This provides the C++ compiler needed to build the application.

**Option A: Full Visual Studio (Larger download, includes IDE)**
1. Download Visual Studio from https://visualstudio.microsoft.com/downloads/
2. Choose "Community" edition (free)
3. In the installer, select **"Desktop development with C++"**
4. Click Install

**Option B: Build Tools Only (Smaller download, command-line only)**
1. Download "Build Tools for Visual Studio" from the same page
2. In the installer, select **"Desktop development with C++"**
3. Click Install

### Step 4: Download the Project

1. Open Command Prompt
2. Navigate to where you want the project:
   ```cmd
   cd C:\Users\YourName\Projects
   ```
3. Download the project:
   ```cmd
   git clone https://github.com/YOUR_USERNAME/UnrealLogsReader.git
   cd UnrealLogsReader
   ```

### Step 5: Download and Setup ImGui

Follow the [Download ImGui](#download-imgui-required-for-all-platforms) instructions above.

### Step 6: Install make

Open powershell as an administrator and type the following command:

```cmd
winget install ezwinports.make
```

### Step 7: Build and Run

Use the Makefile to build and run the application:

```cmd
make run
```

The executable is located at `.\build\Release\UnrealLogsReader.exe`

## Linux Setup

These instructions cover Arch Linux, Debian/Ubuntu, and Fedora. Other distributions may require similar packages.

### Step 1: Install Build Tools and CMake

**Arch Linux:**
```
sudo pacman -S --needed base-devel cmake
```
**Debian**
```
sudo apt update
sudo apt install build-essential cmake
```
### Step 2: Install OpenGL and GLFW Development Libraries

**Arch Linux**
```
sudo pacman -S mesa glfw-x11
```
**Debian**
```
sudo apt install libgl1-mesa-dev libglfw3-dev
```

### Step 3: clone the repo
```
git clone https://github.com/Bambou-Dev/UnrealLogsReader.git
cd UnrealLogsReader
```
### Step 4: Build the project

```
mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j$(nproc)
```

you'll find the executable in **UnrealLogsReader/build/UnrealLogsReader**

## Makefile Commands

| Command | Description |
|---------|-------------|
| `make` | Configure and build the project (default target) |
| `make build` | Same as `make` - configure and build |
| `make configure` | Create build directory and run CMake configuration |
| `make run` | Build the project and run the executable |
| `make clean` | Remove the build and dist directory completely |

## Usage Guide

1. **Launch** the application by running the executable
2. **Click "Load Log File"** to open a file browser
3. **Select** an Unreal Engine `.log` or `.txt` file
4. **Use filters** at the top to narrow down log entries:
   - Check/uncheck Errors, Warnings, Display
   - Select a specific Category from the dropdown
   - Type in the Search box
   - Toggle "Show Duplicates" to hide repeated entries
5. **Click on a log line** to see surrounding context in the Inspector panel
6. **Multi-select** logs using Ctrl+Click (toggle) or Shift+Click (range)
7. **Copy** selected logs with Ctrl+C. This also strips the datetime at the beginning of the line and add ``` between the lines. (Used to send it on discord with good formatting)
8. **Right-click** for context menu options

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl + C | Copy selected log entries |
| Ctrl + Click | Toggle selection on a log entry |
| Shift + Click | Select range of log entries |


## Technologies Used

- **C++23** - Modern C++ programming language
- **CMake** - Cross-platform build system
- **GLFW** - Window creation and input handling (auto-downloaded)
- **OpenGL** - Graphics rendering
- **Dear ImGui** - Immediate mode GUI library
- **Native File Dialog** - Cross-platform file browser (auto-downloaded)

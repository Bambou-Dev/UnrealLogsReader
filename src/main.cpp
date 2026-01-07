#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <vector>
#include <string>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <map>
#include <set>
#include <nfd.h>

// =========================================================
// --- 1. DATA STRUCTURES ---
enum class LogLevel { Display, Warning, Error };

struct LogEntry {
    std::string FullText;
    std::string Category;
    LogLevel Level = LogLevel::Error;
    size_t ContentHash = 0;
    bool IsHeader = false;
    int LogIndex = 0;
};

// UE Logs usually look like:
// [2024.01.01-14.22.33:123] LogCook: Error: Missing Texture...
// We want to extract "LogCook" (Category) and "Error" (Level)
void ParseLogLine(const std::string& line, LogEntry& entry) {
    entry.FullText = line;
    entry.Level = LogLevel::Display;
    entry.Category = "General";

    // 1. Detect Level (Simple string check is fastest)
    if (line.find("Error:") != std::string::npos || line.find("Critical:") != std::string::npos) {
        entry.Level = LogLevel::Error;
    } else if (line.find("Warning:") != std::string::npos) {
        entry.Level = LogLevel::Warning;
    }

    // 2. Detect Category (Text before the first colon, or specifically LogX)
    // Adjust this logic based on your specific log format needs
    size_t catStart = line.find("]Log");
    if (catStart != std::string::npos) {
        // Found standard UE category format like [123]LogTemp:
        catStart++; // Skip ']'
        const size_t catEnd = line.find(':', catStart);
        if (catEnd != std::string::npos) {
            entry.Category = line.substr(catStart, catEnd - catStart);
        }
    }
}

struct LogViewerState {
    std::vector<LogEntry> AllLogs;
    std::vector<int> FilteredIndices; // Indices of logs that match current filters

    std::map<LogLevel, int> LevelsCount; // Number of logs of each LogLevel

    std::set<int> SelectedIndices; // Stores indices of the *filtered* list
    int LastClickedIndex = -1;     // Used for Shift+Click ranges

    // Filters
    bool ShowErrors = true;
    bool ShowWarnings = true;
    bool ShowDisplay = true;
    char SearchBuffer[128] = "";
    std::string SelectedCategory = "All";
    std::set<std::string> UniqueCategories; // To populate the dropdown

    bool ShowDuplicates = true;

    static void ParseProperties(LogEntry& entry) {
        // 1. Default values
        entry.Level = LogLevel::Display;
        entry.Category = "General";

        // 2. Detect Level
        // We look for "Error:" or "Critical:" anywhere in the text
        if (entry.FullText.find("Error:") != std::string::npos ||
            entry.FullText.find("Critical:") != std::string::npos ||
            entry.FullText.find("Fatal:") != std::string::npos) {
            entry.Level = LogLevel::Error;
            }
        else if (entry.FullText.find("Warning:") != std::string::npos) {
            entry.Level = LogLevel::Warning;
        }

        // 3. Detect Category
        // Tries to find the pattern "]LogX:" or "> LogX:"
        size_t catStart = entry.FullText.find("Log");
        if (catStart != std::string::npos) {
            // Check if it is preceded by ']' or '> ' or ' '
            if (catStart > 0 && (entry.FullText[catStart-1] == ']' || entry.FullText[catStart-1] == ' ' || entry.FullText[catStart-1] == ':')) {
                size_t catEnd = entry.FullText.find(':', catStart);
                if (catEnd != std::string::npos) {
                    entry.Category = entry.FullText.substr(catStart, catEnd - catStart);
                }
            }
        }
    }

   void LoadFile(const std::string& path) {
        AllLogs.clear();
        UniqueCategories.clear();
        UniqueCategories.insert("All");

        std::ifstream file(path);
        if (!file.is_open()) return;

        std::string line;

        // Track state for continuation lines
        LogLevel currentLevel = LogLevel::Display;
        std::string currentCategory = "General";

        int CurrentIndex = -1;
        while (std::getline(file, line)) {
            // Stop at summary
            if (line.find("Warning/Error Summary") != std::string::npos) break;
            if (line.empty()) continue;

            CurrentIndex++;

            LogEntry entry;
            entry.FullText = line;
            entry.LogIndex = CurrentIndex;

            // --- 1. IDENTIFY IF HEADER OR CONTINUATION ---
            if (!line.empty() && line[0] == '[') {
                entry.IsHeader = true;

                // --- 2. PARSE PROPERTIES ---
                entry.Level = LogLevel::Display;
                entry.Category = "General";

                if (line.find("Error:") != std::string::npos ||
                    line.find("Critical:") != std::string::npos ||
                    line.find("Fatal:") != std::string::npos) {
                    entry.Level = LogLevel::Error;
                }
                else if (line.find("Warning:") != std::string::npos) {
                    entry.Level = LogLevel::Warning;
                }

                // Extract Category
                size_t catStart = line.find("Log");
                if (catStart != std::string::npos) {
                     // Safety check to ensure it's the category tag
                    if (catStart > 0 && (line[catStart-1] == ']' || line[catStart-1] == ' ' || line[catStart-1] == ':')) {
                        size_t catEnd = line.find(':', catStart);
                        if (catEnd != std::string::npos) {
                            entry.Category = line.substr(catStart, catEnd - catStart);
                        }
                    }
                }

                // --- 3. COMPUTE HASH (Unique ID) ---
                // We want to hash ONLY the message, skipping the timestamp "[2024...][123]"
                // If we find "Log", start hashing from there. Otherwise hash the whole line.
                std::string textToHash = (catStart != std::string::npos) ? line.substr(catStart) : line;
                entry.ContentHash = std::hash<std::string>{}(textToHash);

                // Update "Current" state
                currentLevel = entry.Level;
                currentCategory = entry.Category;
            }
            else {
                // Continuation line
                entry.IsHeader = false;
                entry.Level = currentLevel;
                entry.Category = currentCategory;
                entry.FullText = "      " + line; // Visual indent
                entry.ContentHash = 0; // Hash irrelevant for children, they follow parent
            }

            AllLogs.push_back(entry);
            LevelsCount[entry.Level]++;
            UniqueCategories.insert(entry.Category);
        }
        ApplyFilters();
    }


    void ApplyFilters() {
        FilteredIndices.clear();
        SelectedIndices.clear();
        LastClickedIndex = -1;
        std::string search(SearchBuffer);
        std::ranges::transform(search, search.begin(), ::tolower);

        std::set<size_t> seenHashes;
        bool isSkippingDuplicates = false;



        for (int i = 0; i < AllLogs.size(); ++i) {
            const auto& log = AllLogs[i];

            // --- DUPLICATE HANDLING ---
            if (log.IsHeader) {
                // If this is a header, check if we've seen it before
                if (!ShowDuplicates && seenHashes.contains(log.ContentHash)) {
                    isSkippingDuplicates = true; // Start skipping this entire block
                } else {
                    isSkippingDuplicates = false; // Valid unique entry, stop skipping
                    seenHashes.insert(log.ContentHash);
                }
            }

            // If we are currently inside a duplicate block (Header + its children), skip
            if (isSkippingDuplicates) continue;


            // --- STANDARD FILTERS ---
            if (log.Level == LogLevel::Error && !ShowErrors) continue;
            if (log.Level == LogLevel::Warning && !ShowWarnings) continue;
            if (log.Level == LogLevel::Display && !ShowDisplay) continue;
            if (SelectedCategory != "All" && log.Category != SelectedCategory) continue;

            if (!search.empty()) {
                std::string logLower = log.FullText;
                std::ranges::transform(logLower, logLower.begin(), ::tolower);
                if (logLower.find(search) == std::string::npos) continue;
            }

            FilteredIndices.push_back(i);
        }
    }
};

// Global state instance
LogViewerState g_LogState;
int g_LastClickedIndex = -1;

std::string CleanLogLine(const std::string& line) {
    // Find the end of the timestamp (first closing bracket)
    const size_t endBracket = line.find(']');
    std::string text = line;

    // If found and looks like a timestamp (at start of line), strip it
    if (endBracket != std::string::npos && endBracket < 40) {
        text = line.substr(endBracket + 1);

         // Remove leading " > " or spaces that might remain
        const size_t firstChar = text.find_first_not_of(" >");
        if (firstChar != std::string::npos) {
            text = text.substr(firstChar);
        }
    }
    return text;
}

void RenderLogViewer() {
    ImGui::Begin("Unreal Log Reader");

    // -- Top Bar: Load & Filters --
    if (ImGui::Button("Load Log File")) {
        // --- 2. Open File Explorer ---
        NFD_Init(); // Initialize NFD
        nfdchar_t *outPath;
        nfdfilteritem_t filterItem[1] = { { "Unreal Logs", "log,txt" } };

        // Open the dialog
        nfdresult_t result = NFD_OpenDialog(&outPath, filterItem, 1, nullptr);

        if (result == NFD_OKAY) {
            g_LogState.LoadFile(outPath); // Load the selected file
            NFD_FreePath(outPath);
        } else if (result == NFD_CANCEL) {
            // User pressed cancel
        } else {
            printf("Error: %s\n", NFD_GetError());
        }
        NFD_Quit();
    }

    ImGui::Separator();

    // Checkboxes
    bool filterChanged = false;
    filterChanged |= ImGui::Checkbox("Errors", &g_LogState.ShowErrors); ImGui::SameLine();
    filterChanged |= ImGui::Checkbox("Warnings", &g_LogState.ShowWarnings); ImGui::SameLine();
    filterChanged |= ImGui::Checkbox("Display", &g_LogState.ShowDisplay); ImGui::SameLine();
    filterChanged |= ImGui::Checkbox("Show Duplicates", &g_LogState.ShowDuplicates);

    ImGui::Text("Warnings: %d", g_LogState.LevelsCount[LogLevel::Warning]); ImGui::SameLine();
    ImGui::Text("Errors: %d", g_LogState.LevelsCount[LogLevel::Error]);

    ImGui::SetNextItemWidth(150);
    if (ImGui::BeginCombo("Category", g_LogState.SelectedCategory.c_str())) {
        for (const auto& cat : g_LogState.UniqueCategories) {
            bool isSelected = (g_LogState.SelectedCategory == cat);
            if (ImGui::Selectable(cat.c_str(), isSelected)) {
                g_LogState.SelectedCategory = cat;
                filterChanged = true;
            }
            if (isSelected) ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SameLine();
    ImGui::Text("Search:"); ImGui::SameLine();
    if (ImGui::InputText("##Search", g_LogState.SearchBuffer, sizeof(g_LogState.SearchBuffer))) {
        filterChanged = true;
    }

    if (filterChanged) {
        g_LogState.ApplyFilters();
    }

    ImGui::Separator();

    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C)) {
        if (!g_LogState.SelectedIndices.empty()) {
            std::string clipboardText = "```\n";
            for (int idx : g_LogState.SelectedIndices) {
                // Safety check
                if (idx >= 0 && idx < g_LogState.FilteredIndices.size()) {
                    int originalIndex = g_LogState.FilteredIndices[idx];
                    clipboardText += CleanLogLine(g_LogState.AllLogs[originalIndex].FullText + "\n");
                }
            }
            clipboardText += "```"; // End with backticks
            ImGui::SetClipboardText(clipboardText.c_str());
        }
    }

    std::string newCategoryFilter;

    ImGui::BeginChild("LogScroll", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    ImGuiListClipper clipper;
    clipper.Begin(g_LogState.FilteredIndices.size());

    while (clipper.Step()) {
        for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            int originalIndex = g_LogState.FilteredIndices[i];
            const LogEntry& log = g_LogState.AllLogs[originalIndex];

            // --- COLOR LOGIC ---
            ImVec4 color = ImVec4(0.9f, 0.9f, 0.9f, 1.0f); // Default Light Grey
            if (log.Level == LogLevel::Error) color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); // Red
            else if (log.Level == LogLevel::Warning) color = ImVec4(1.0f, 0.9f, 0.4f, 1.0f); // Yellow
            else if (log.Category == "LogCook") color = ImVec4(0.6f, 0.8f, 1.0f, 1.0f); // Light Blue

            // --- SELECTION LOGIC ---
            bool isSelected = g_LogState.SelectedIndices.contains(i);

            ImGui::PushStyleColor(ImGuiCol_Text, color);

            // Generate a unique ID for Selectable using "##" + index
            std::string label = "##Line" + std::to_string(i);

            // Draw the selectable line (spans full width)
            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns)) {
                // 1. Handle CTRL+Click (Toggle)
                if (ImGui::GetIO().KeyCtrl) {
                    if (isSelected) g_LogState.SelectedIndices.erase(i);
                    else g_LogState.SelectedIndices.insert(i);
                    g_LogState.LastClickedIndex = i;
                }
                // 2. Handle SHIFT+Click (Range)
                else if (ImGui::GetIO().KeyShift && g_LogState.LastClickedIndex != -1) {
                    int start = std::min(g_LogState.LastClickedIndex, i);
                    int end = std::max(g_LogState.LastClickedIndex, i);

                    // Clear previous selection if you want standard OS behavior,
                    // or keep it if you want additive. Standard is usually to clear:
                    g_LogState.SelectedIndices.clear();

                    for (int n = start; n <= end; n++) {
                        g_LogState.SelectedIndices.insert(n);
                    }
                }
                // 3. Handle Normal Click (Single select)
                else {
                    g_LogState.SelectedIndices.clear();
                    g_LogState.SelectedIndices.insert(i);
                    g_LogState.LastClickedIndex = i;
                    g_LastClickedIndex = log.LogIndex;
                }
            }

            // Draw the actual text on top of the Selectable
            ImGui::SameLine();
            ImGui::TextUnformatted(log.FullText.c_str());

            ImGui::PopStyleColor();

            // Right-Click Context Menu
            std::string contextMenuId = "##ctx" + std::to_string(i);
            if (ImGui::BeginPopupContextItem(contextMenuId.c_str())) {
                if (ImGui::Selectable("Copy")) {
                    const std::string text = "```\n" + CleanLogLine(log.FullText) + "\n```";
                    ImGui::SetClipboardText(text.c_str());
                }
                if (ImGui::Selectable("Filter to this Category")) {
                    g_LogState.SelectedCategory = log.Category;
                    newCategoryFilter = log.Category;
                }
                ImGui::EndPopup();
            }
        }
    }
    ImGui::EndChild();

    if (!newCategoryFilter.empty()) {
        g_LogState.SelectedCategory = newCategoryFilter;
        g_LogState.ApplyFilters();
    }

    ImGui::End();

    // 2. The Context Window
    ImGui::Begin("Log Context (Inspector)");
    ImGui::BeginChild("LogContext", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
    if (g_LastClickedIndex != -1 && g_LastClickedIndex < g_LogState.AllLogs.size()) {

        // Calculate bounds (5 before, 5 after)
        int startIdx = std::max(0, g_LastClickedIndex - 5);
        int endIdx = std::min(static_cast<int>(g_LogState.AllLogs.size()), g_LastClickedIndex + 6); // +1 because loop is < endIdx

        ImGui::Text("Context around log #%d:", g_LastClickedIndex);
        ImGui::Separator();

        for (int i = startIdx; i < endIdx; i++) {
            const auto& log = g_LogState.AllLogs[i];

            // Highlight the specific line we clicked on

            if (i == g_LastClickedIndex) {
                // Make the selected line stand out with a background color
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 1, 0, 1)); // Green text
            } else {
                // Dim the surrounding context slightly
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1));
            }

            ImGui::Text("[%d] %s", i, log.FullText.c_str());

            ImGui::PopStyleColor();
        }
    } else {
        ImGui::TextDisabled("Select a log line to view context.");
    }
    ImGui::EndChild();
    ImGui::End();
}

// =========================================================

void SetupModernStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    // 1. Geometry - Make it softer
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 6);

    // 2. Colors - "Deep Slate" Theme
    ImVec4* colors = style.Colors;
    colors[ImGuiCol_Text]                   = ImVec4(0.95f, 0.96f, 0.98f, 1.00f);
    colors[ImGuiCol_WindowBg]               = ImVec4(0.12f, 0.12f, 0.13f, 1.00f); // Dark background
    colors[ImGuiCol_ChildBg]                = ImVec4(0.10f, 0.10f, 0.10f, 1.00f); // Darker scroll area
    colors[ImGuiCol_Border]                 = ImVec4(0.25f, 0.25f, 0.27f, 0.50f);

    // Headers (List items)
    colors[ImGuiCol_Header]                 = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.10f);
    colors[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 0.30f);

    // Buttons
    colors[ImGuiCol_Button]                 = ImVec4(0.20f, 0.25f, 0.30f, 1.00f);
    colors[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f); // Blue highlight
    colors[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);

    // Frame BG (Checkboxes, inputs)
    colors[ImGuiCol_FrameBg]                = ImVec4(0.20f, 0.20f, 0.22f, 1.00f);
    colors[ImGuiCol_FrameBgHovered]         = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
    colors[ImGuiCol_FrameBgActive]          = ImVec4(0.30f, 0.30f, 0.33f, 1.00f);
}

// Main Boilerplate
int main(int, char**)
{
    // 1. Setup Window
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Unreal Log Reader", nullptr, nullptr);
    if (window == nullptr)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // 2. Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking


    // 3. Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    SetupModernStyle();

    // --- 2. LOAD FONT (Crucial for modern look) ---
    // Windows usually has Segoe UI. We load it at 18px size.
    // If this file doesn't exist, ImGui will use the default font.
    if (std::filesystem::exists("C:\\Windows\\Fonts\\segoeui.ttf")) {
        io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    } else {
        // Fallback: Default font scaled up
        io.Fonts->AddFontDefault();
    }

    // 4. Main Loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        RenderLogViewer();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f); // Background color
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
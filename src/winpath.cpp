#include <windows.h>
#include <filesystem>
#include "winpath.h"

std::string getExecutableDirectory() {
    char pathBuffer[MAX_PATH];
    GetModuleFileNameA(NULL, pathBuffer, MAX_PATH);
    std::filesystem::path exePath(pathBuffer);
    std::filesystem::path parentDir = exePath.parent_path();
    return parentDir.string();
}
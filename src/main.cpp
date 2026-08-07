#include "pch.h"
#include "updater.h"
#include "logger.h"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR cmdLine, int) {
    wchar_t appData[MAX_PATH] = {};
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData);
    updater::LogInit(std::wstring(appData) + L"\\yyzTools\\logs");

    std::wstring appDir = updater::GetAppDir();
    std::wstring args = cmdLine ? cmdLine : L"";

    int rc;
    if (args.find(L"--check-and-apply") != std::wstring::npos) {
        rc = updater::RunCheckAndApply(appDir);
    } else if (args.find(L"--apply") != std::wstring::npos) {
        rc = updater::RunApply(appDir, false);
    } else {
        rc = updater::RunCheck(appDir);  // 默认 --check（静默）
    }
    return rc;
}

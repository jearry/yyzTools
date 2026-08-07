#include "pch.h"
#include "updater.h"
#include "http_client.h"
#include "hasher.h"
#include "zip_extract.h"
#include "applying.h"
#include "logger.h"
#include "json.h"
#include "version.h"

#pragma comment(lib, "version.lib")

namespace updater {

std::wstring GetAppDir() {
    wchar_t buf[MAX_PATH];
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring p = buf;
    return p.substr(0, p.find_last_of(L'\\'));
}

std::wstring GetUpdateDir() {
    wchar_t buf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    std::wstring p = std::wstring(buf) + L"\\yyzTools\\update";
    CreateDirectoryW(p.c_str(), nullptr);
    return p;
}

std::wstring GetTempDir() {
    wchar_t buf[MAX_PATH];
    GetTempPathW(MAX_PATH, buf);
    std::wstring p = std::wstring(buf) + L"yyzUpdater";
    CreateDirectoryW(p.c_str(), nullptr);
    return p;
}

// update.json 托管源：jsDelivr CDN 主源 + GitHub raw 备用。TODO: 发版前确认 owner/repo
static const wchar_t* UPDATE_JSON_URLS[] = {
    L"https://cdn.jsdelivr.net/gh/yyztools/yyztools@main/releases/update.json",
    L"https://raw.githubusercontent.com/yyztools/yyztools/main/releases/update.json"
};

struct UpdateReady {
    std::string version;
    std::wstring zipPath;
    std::string sha256;
};

static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int n = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), s.data(), n, nullptr, nullptr);
    return s;
}

static std::wstring Utf8ToWide(const std::string& s) {
    if (s.empty()) return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    std::wstring w(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), w.data(), n);
    return w;
}

static bool WriteUpdateReady(const UpdateReady& r) {
    std::wstring path = GetUpdateDir() + L"\\update.ready";
    std::ofstream f(path);
    if (!f) return false;
    f << "version=" << r.version << "\n";
    f << "zipPath=" << WideToUtf8(r.zipPath) << "\n";
    f << "sha256=" << r.sha256 << "\n";
    return true;
}

static bool ReadUpdateReady(UpdateReady& r) {
    std::wstring path = GetUpdateDir() + L"\\update.ready";
    std::ifstream f(path);
    if (!f) return false;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (k == "version") r.version = v;
        else if (k == "sha256") r.sha256 = v;
        else if (k == "zipPath") r.zipPath = Utf8ToWide(v);
    }
    return !r.version.empty() && !r.zipPath.empty();
}

static std::string GetMainExeVersion(const std::wstring& appDir) {
    std::wstring exe = appDir + L"\\yyzTools.exe";
    DWORD dummy = 0;
    DWORD sz = GetFileVersionInfoSizeW(exe.c_str(), &dummy);
    if (sz == 0) return {};
    std::vector<BYTE> data(sz);
    if (!GetFileVersionInfoW(exe.c_str(), 0, sz, data.data())) return {};
    VS_FIXEDFILEINFO* ffi = nullptr;
    UINT len = 0;
    if (!VerQueryValueW(data.data(), L"\\", (LPVOID*)&ffi, &len) || !ffi) return {};
    char buf[64];
    snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
             HIWORD(ffi->dwFileVersionMS), LOWORD(ffi->dwFileVersionMS),
             HIWORD(ffi->dwFileVersionLS), LOWORD(ffi->dwFileVersionLS));
    return buf;
}

enum CheckResult { kUpToDate, kReady, kError };

static CheckResult DoCheck(const std::wstring& appDir, bool silent) {
    std::string current = GetMainExeVersion(appDir);
    if (current.empty()) { Log("get current version failed"); return kError; }
    LogFmt("current version=%s", current.c_str());

    std::string jsonText;
    for (auto url : UPDATE_JSON_URLS) {
        LogFmt("try %ls", url);
        jsonText = DownloadText(url);
        if (!jsonText.empty()) break;
    }
    if (jsonText.empty()) {
        Log("fetch update.json failed");
        if (!silent) MessageBoxW(nullptr, L"无法连接更新服务器，请稍后重试。", L"检查更新", MB_OK | MB_ICONERROR);
        return kError;
    }

    JsonParser parser(jsonText);
    auto root = parser.Parse();
    if (!root || root->type != JsonValue::Obj) {
        Log("parse update.json failed");
        if (!silent) MessageBoxW(nullptr, L"更新信息解析失败。", L"检查更新", MB_OK | MB_ICONERROR);
        return kError;
    }

    auto* ver = root->Find("latestVersion");
    if (!ver || ver->type != JsonValue::Str) { Log("no latestVersion"); return kError; }
    const std::string& latest = ver->str;

    if (CompareVersion(latest, current) <= 0) {
        Log("already up-to-date");
        if (!silent) MessageBoxW(nullptr, L"当前已是最新版本。", L"检查更新", MB_OK | MB_ICONINFORMATION);
        return kUpToDate;
    }

    auto* pkgs = root->Find("packages");
    if (!pkgs || pkgs->type != JsonValue::Obj) { Log("no packages"); return kError; }
    auto* main = pkgs->Find("main");
    if (!main || main->type != JsonValue::Obj) { Log("no main package"); return kError; }
    auto* urlV = main->Find("url");
    auto* shaV = main->Find("sha256");
    if (!urlV || urlV->type != JsonValue::Str || !shaV || shaV->type != JsonValue::Str) {
        Log("main package missing url/sha256"); return kError;
    }

    std::wstring zipUrl = Utf8ToWide(urlV->str);
    std::wstring zipPath = GetTempDir() + L"\\main-" + Utf8ToWide(latest) + L".7z";
    LogFmt("downloading main package: %ls", zipUrl.c_str());
    if (!DownloadFile(zipUrl, zipPath, nullptr)) {
        Log("download main failed");
        if (!silent) MessageBoxW(nullptr, L"下载更新包失败，请稍后重试。", L"检查更新", MB_OK | MB_ICONERROR);
        return kError;
    }

    std::string actual = Sha256File(zipPath);
    if (_stricmp(actual.c_str(), shaV->str.c_str()) != 0) {
        LogFmt("sha256 mismatch: expected=%s actual=%s", shaV->str.c_str(), actual.c_str());
        DeleteFileW(zipPath.c_str());
        if (!silent) MessageBoxW(nullptr, L"更新包校验失败，已丢弃。", L"检查更新", MB_OK | MB_ICONERROR);
        return kError;
    }
    Log("sha256 ok");

    UpdateReady r;
    r.version = latest;
    r.zipPath = zipPath;
    r.sha256 = shaV->str;
    WriteUpdateReady(r);
    LogFmt("update.ready written, version=%s", latest.c_str());
    return kReady;
}

int RunCheck(const std::wstring& appDir) {
    Log("--check start");
    return DoCheck(appDir, true) == kError ? 1 : 0;
}

int RunApply(const std::wstring& appDir, bool forceKillMain) {
    Log("--apply start");
    UpdateReady r;
    if (!ReadUpdateReady(r)) { Log("no update.ready"); return 1; }
    LogFmt("apply version=%s", r.version.c_str());

    if (!WaitMainExit(appDir, forceKillMain ? 3000 : 30000, forceKillMain)) {
        Log("wait main exit failed");
        return 1;
    }
    KillChildProcesses();

    std::wstring staging = GetTempDir() + L"\\staging";
    RemoveDirRecursive(staging);
    if (!ExtractArchive(r.zipPath, staging)) { Log("extract failed"); return 1; }

    if (!ApplyStaging(appDir, staging)) { Log("apply staging had failures (partial)"); }

    LaunchMain(appDir);

    DeleteFileW(r.zipPath.c_str());
    DeleteFileW((GetUpdateDir() + L"\\update.ready").c_str());
    RemoveDirRecursive(staging);
    Log("--apply done");
    return 0;
}

int RunCheckAndApply(const std::wstring& appDir) {
    Log("--check-and-apply start");

    UpdateReady r;
    if (ReadUpdateReady(r)) {
        Log("found existing update.ready, apply directly");
        std::wstring msg = L"发现已下载的更新 " + Utf8ToWide(r.version) + L"，是否立即重启安装？";
        if (MessageBoxW(nullptr, msg.c_str(), L"检查更新", MB_YESNO | MB_ICONQUESTION) == IDNO)
            return 0;
        return RunApply(appDir, true);
    }

    CheckResult cr = DoCheck(appDir, false);
    if (cr == kReady) {
        UpdateReady r2;
        ReadUpdateReady(r2);
        std::wstring msg = L"发现新版本 " + Utf8ToWide(r2.version) + L"，是否立即重启安装？\n\n选择“否”将在下次启动时自动安装。";
        if (MessageBoxW(nullptr, msg.c_str(), L"检查更新", MB_YESNO | MB_ICONQUESTION) == IDYES) {
            return RunApply(appDir, true);
        }
        return 0;
    }
    return cr == kError ? 1 : 0;
}

} // namespace updater

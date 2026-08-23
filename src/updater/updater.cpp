// MIT License
//
// Copyright (c) 2026 yyzTools
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "pch.h"
#include "updater.h"
#include "http_client.h"
#include "hasher.h"
#include "zip_extract.h"
#include "applying.h"
#include "logger.h"
#include "json.h"
#include "version.h"

#include <algorithm>
#include <map>

namespace updater {

// %APPDATA%\yyzTools 根：安装目标、Config、Update 等所有持久数据均在此之下
static std::wstring GetAppDataRoot() {
    wchar_t buf[MAX_PATH];
    SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, buf);
    return std::wstring(buf) + L"\\yyzTools";
}

// 确保目录存在后返回（CreateDirectoryW 不级联创建，要求父目录已存在）
static std::wstring EnsureDir(const std::wstring& dir) {
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

// 更新目标 / 主程序启动目录：%APPDATA%\yyzTools\。同时是 Platform\7z、aria2c 等工具的定位基准
std::wstring GetInstallDir() {
    return EnsureDir(GetAppDataRoot());
}

// 更新工作区根：update.ready、update.json 都落在此
std::wstring GetUpdateDir() {
    return EnsureDir(GetAppDataRoot() + L"\\Update");
}

// 下载包持久缓存：跨进程保留，配合 sha256 复用避免重复下载
std::wstring GetDownloadDir() {
    return EnsureDir(GetUpdateDir() + L"\\downloads");
}

// 解压暂存根：每个包解压到 staging\<name>，应用完即删
std::wstring GetStagingDir() {
    return EnsureDir(GetUpdateDir() + L"\\staging");
}

// 解压工具副本：7z.exe/7z.dll 从 appDir 复制到此运行，避免自更新时源文件被覆盖锁住
std::wstring GetToolsDir() {
    return EnsureDir(GetUpdateDir() + L"\\tools");
}

// update.json 源的兜底 base（Update.json 丢失时用，GitHub 官方源）。镜像源由 update_config.json 配置。
static const wchar_t* FALLBACK_UPDATE_JSON_BASE = L"https://raw.githubusercontent.com/jearry/yyzTools/main";
// release 资产的 GitHub base（update.json 的 url 只放后缀 v<ver>/<file>，C++ 拼 base + 后缀 + 镜像）
static const wchar_t* RELEASE_BASE = L"https://github.com/jearry/yyzTools/releases/download";

// 包应用顺序：资源类先，platform 子包居中，main（含 yyzTools.exe / yyzUpdater.exe 自更新）最后
static const char* APPLY_ORDER[] = { "web", "modules", "wallpaper", "yyztools", "tools", "everything", "rapidocr", "ffmpeg", "main" };

struct PendingPackage {
    std::string name;
    std::string version;
    std::wstring zipPath;
    std::string sha256;
    std::vector<std::wstring> killProcesses;       // 该包应用前要普通杀的进程
    std::vector<std::wstring> killProcessesAdmin;  // 该包应用前要管理员杀的进程（如 Everything）
};

struct UpdatePlan {
    std::vector<PendingPackage> packages;
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

// 转义 JSON 字符串值：路径含反斜杠必须转义，否则 json.h 解析会丢字符
static std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (char c : s) {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else out += c;
    }
    return out;
}

static std::string ReadTextFile(const std::wstring& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return {};
    f.seekg(0, std::ios::end);
    std::streamoff sz = f.tellg();
    if (sz < 0) return {};
    f.seekg(0, std::ios::beg);
    std::string text((size_t)sz, '\0');
    f.read(&text[0], (std::streamsize)sz);
    return text;
}

// 取文件大小（字节）；不存在或不可访问返回 0。
static unsigned long long FileSizeBytes(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA ad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &ad)) return 0;
    return ((unsigned long long)ad.nFileSizeHigh << 32) | (unsigned long long)ad.nFileSizeLow;
}

// ---- 客户端镜像配置（%APPDATA%\yyzTools\Config\Update.json）----

// client_update.json 镜像配置路径（与 client_versions.json 同放 Update 目录）
static std::wstring GetConfigPath() {
    return GetUpdateDir() + L"\\client_update.json";
}

struct MirrorConfig {
    std::vector<std::wstring> updateJsonBases;   // update.json 源 base（main 前），拼 /releases/update.json
    std::vector<std::wstring> releaseMirrors;    // release 资产镜像前缀，拼 <前缀>/真实URL
};

// 读镜像配置。配置文件不存在则用默认并写入文件（让用户可编辑）。
// updateJsonBases: update.json 下载源（拼 /releases/update.json）
// releaseMirrors: release 资产镜像前缀（拼 <前缀>/真实URL）
static MirrorConfig ReadMirrorConfig() {
    MirrorConfig cfg;
    std::wstring path = GetConfigPath();
    std::string text = ReadTextFile(path);
    if (!text.empty()) {
        JsonParser parser(text);
        auto root = parser.Parse();
        if (root && root->type == JsonValue::Obj) {
            auto* b = root->Find("updateJsonBases");
            if (b && b->type == JsonValue::Arr) for (auto& v : b->arr) if (v && v->type == JsonValue::Str) cfg.updateJsonBases.push_back(Utf8ToWide(v->str));
            auto* m = root->Find("releaseMirrors");
            if (m && m->type == JsonValue::Arr) for (auto& v : m->arr) if (v && v->type == JsonValue::Str) cfg.releaseMirrors.push_back(Utf8ToWide(v->str));
        }
    }
    if (cfg.updateJsonBases.empty() && cfg.releaseMirrors.empty()) {
        // Update.json 丢失：用 GitHub 官方源兜底，写入文件让用户可编辑
        cfg.updateJsonBases.push_back(FALLBACK_UPDATE_JSON_BASE);
        std::ofstream f(path);
        if (f) {
            f << "{\n  \"updateJsonBases\": [\n    \"" << WideToUtf8(cfg.updateJsonBases[0]) << "\"\n  ],\n  \"releaseMirrors\": []\n}\n";
        }
    }
    return cfg;
}

// 真实 URL + 镜像前缀拼多源：[realUrl, mirror1/realUrl, mirror2/realUrl, ...]
static std::vector<std::wstring> BuildMultiSourceUrls(const std::wstring& realUrl, const std::vector<std::wstring>& mirrors) {
    std::vector<std::wstring> urls;
    urls.reserve(mirrors.size() + 1);
    urls.push_back(realUrl);
    for (auto& m : mirrors) urls.push_back(m + L"/" + realUrl);
    return urls;
}

// ---- 本地版本文件 versions.json（{app}\update\versions.json，不再读 exe）----

static std::map<std::string, std::string> ReadLocalVersions() {
    std::map<std::string, std::string> versions;
    std::string text = ReadTextFile(GetUpdateDir() + L"\\client_versions.json");
    if (text.empty()) return versions;
    JsonParser parser(text);
    auto root = parser.Parse();
    if (!root || root->type != JsonValue::Obj) return versions;
    for (auto& kv : root->obj) {
        if (kv.second && kv.second->type == JsonValue::Str)
            versions[kv.first] = kv.second->str;
    }
    return versions;
}

static bool SaveVersions(const std::map<std::string, std::string>& versions) {
    std::ofstream f(GetUpdateDir() + L"\\client_versions.json");
    if (!f) return false;
    f << "{\n";
    bool first = true;
    for (auto& kv : versions) {
        if (!first) f << ",\n";
        f << "  \"" << kv.first << "\": \"" << kv.second << "\"";
        first = false;
    }
    f << "\n}\n";
    return true;
}

static bool UpdateLocalVersion(const std::string& pkgName, const std::string& version) {
    auto versions = ReadLocalVersions();
    versions[pkgName] = version;
    return SaveVersions(versions);
}

// ---- update.ready（多包 JSON）----

// 写一个 JSON 字符串数组字段："field": ["a", "b"]
static void WriteStringArray(std::ofstream& f, const char* field, const std::vector<std::wstring>& arr) {
    f << "\"" << field << "\": [";
    for (size_t i = 0; i < arr.size(); i++) {
        if (i) f << ", ";
        f << "\"" << JsonEscape(WideToUtf8(arr[i])) << "\"";
    }
    f << "]";
}

static bool WriteUpdateReady(const UpdatePlan& plan) {
    std::ofstream f(GetUpdateDir() + L"\\update.ready");
    if (!f) return false;
    f << "{\n";
    f << "  \"packages\": [\n";
    for (size_t i = 0; i < plan.packages.size(); i++) {
        const auto& p = plan.packages[i];
        f << "    {\"name\": \"" << p.name
          << "\", \"version\": \"" << p.version
          << "\", \"zipPath\": \"" << JsonEscape(WideToUtf8(p.zipPath))
          << "\", \"sha256\": \"" << p.sha256 << "\", ";
        WriteStringArray(f, "killProcesses", p.killProcesses);
        f << ", ";
        WriteStringArray(f, "killProcessesAdmin", p.killProcessesAdmin);
        f << "}";
        if (i + 1 < plan.packages.size()) f << ",";
        f << "\n";
    }
    f << "  ]\n}\n";
    return true;
}

// ---- update.result（手动检查的一次性结果标记）----
// 只有手动触发（--check-and-apply）才写，供托盘读一次即删（NativeApi::GetUpdateState）。
// 静默 --check 不写，否则用户下次打开托盘会莫名弹出「已是最新版本」。
static void WriteCheckResultMark(const char* result) {
    std::ofstream f(GetUpdateDir() + L"\\update.result", std::ios::binary | std::ios::trunc);
    if (f) f << result;
}

static void ClearCheckResultMark() {
    DeleteFileW((GetUpdateDir() + L"\\update.result").c_str());
}

static bool ReadUpdateReady(UpdatePlan& plan) {
    std::string text = ReadTextFile(GetUpdateDir() + L"\\update.ready");
    if (text.empty()) return false;
    JsonParser parser(text);
    auto root = parser.Parse();
    if (!root || root->type != JsonValue::Obj) return false;

    auto* arr = root->Find("packages");
    if (!arr || arr->type != JsonValue::Arr) return false;
    for (auto& item : arr->arr) {
        if (!item || item->type != JsonValue::Obj) continue;
        auto* n = item->Find("name");
        auto* v = item->Find("version");
        auto* z = item->Find("zipPath");
        if (!n || n->type != JsonValue::Str) continue;
        if (!v || v->type != JsonValue::Str) continue;
        if (!z || z->type != JsonValue::Str) continue;
        PendingPackage p;
        p.name = n->str;
        p.version = v->str;
        p.zipPath = Utf8ToWide(z->str);
        auto* s = item->Find("sha256");
        if (s && s->type == JsonValue::Str) p.sha256 = s->str;
        auto* kp = item->Find("killProcesses");
        if (kp && kp->type == JsonValue::Arr) {
            for (auto& ke : kp->arr) if (ke && ke->type == JsonValue::Str) p.killProcesses.push_back(Utf8ToWide(ke->str));
        }
        auto* kpa = item->Find("killProcessesAdmin");
        if (kpa && kpa->type == JsonValue::Arr) {
            for (auto& ke : kpa->arr) if (ke && ke->type == JsonValue::Str) p.killProcessesAdmin.push_back(Utf8ToWide(ke->str));
        }
        plan.packages.push_back(p);
    }
    return !plan.packages.empty();
}

enum CheckResult { kUpToDate, kReady, kError };

static int ApplyOrderIndex(const std::string& name) {
    for (int i = 0; i < 9; i++)
        if (APPLY_ORDER[i] == name) return i;
    return 99;
}

static CheckResult DoCheck(const std::wstring& appDir) {
    (void)appDir;
    auto cfg = ReadMirrorConfig();
    LogFmt("mirror config: %zu updateJsonBases, %zu releaseMirrors",
           cfg.updateJsonBases.size(), cfg.releaseMirrors.size());
    for (size_t i = 0; i < cfg.updateJsonBases.size(); i++)
        LogFmt("  updateJsonBase[%zu] = %ls", i, cfg.updateJsonBases[i].c_str());
    for (size_t i = 0; i < cfg.releaseMirrors.size(); i++)
        LogFmt("  releaseMirror[%zu] = %ls", i, cfg.releaseMirrors[i].c_str());

    auto localVersions = ReadLocalVersions();
    LogFmt("local versions: %zu packages", localVersions.size());

    std::vector<std::wstring> jsonUrls;
    for (auto& base : cfg.updateJsonBases) jsonUrls.push_back(base + L"/releases/update.json");
    std::string jsonText = DownloadText(jsonUrls);
    if (jsonText.empty()) {
        Log("fetch update.json failed");
        return kError;
    }
    LogFmt("update.json fetched: %zu bytes", jsonText.size());

    // 缓存 update.json 原文到 Update 目录，供主进程读取非更新字段（如 home 广告位配置）。
    // 仅作缓存，写盘失败不影响后续更新流程。
    {
        std::ofstream f(GetUpdateDir() + L"\\update.json", std::ios::binary | std::ios::trunc);
        if (f) f << jsonText;
    }

    JsonParser parser(jsonText);
    auto root = parser.Parse();
    if (!root || root->type != JsonValue::Obj) {
        Log("parse update.json failed");
        return kError;
    }

    auto* pkgs = root->Find("packages");
    if (!pkgs || pkgs->type != JsonValue::Obj) { Log("no packages"); return kError; }
    LogFmt("update.json has %zu packages", pkgs->obj.size());

    UpdatePlan plan;

    std::vector<PendingPackage> pending;
    bool failed = false;
    const size_t pkgTotal = pkgs->obj.size();
    size_t pkgIdx = 0;
    for (auto& kv : pkgs->obj) {
        const std::string& name = kv.first;
        LogFmt("check pkg %zu/%zu: %s", ++pkgIdx, pkgTotal, name.c_str());
        JsonValue* pkgVal = kv.second.get();
        if (!pkgVal || pkgVal->type != JsonValue::Obj) continue;
        auto* verV = pkgVal->Find("version");
        auto* urlV = pkgVal->Find("url");
        auto* shaV = pkgVal->Find("sha256");
        if (!verV || verV->type != JsonValue::Str) { LogFmt("pkg %s missing version", name.c_str()); continue; }
        if (!urlV || !shaV || shaV->type != JsonValue::Str) {
            LogFmt("pkg %s missing url/sha256", name.c_str()); continue;
        }
        // url 只放真实 URL（字符串）；镜像由客户端配置拼。兼容数组格式（直接用）。
        std::vector<std::wstring> urls;
        if (urlV->type == JsonValue::Str) {
            // url 只放后缀（v<ver>/<file>），拼 RELEASE_BASE；兼容完整 URL（https:// 开头）
            std::wstring suffix = Utf8ToWide(urlV->str);
            std::wstring realUrl = (suffix.find(L"https://") == 0 || suffix.find(L"http://") == 0)
                ? suffix : (std::wstring(RELEASE_BASE) + L"/" + suffix);
            urls = BuildMultiSourceUrls(realUrl, cfg.releaseMirrors);
        } else if (urlV->type == JsonValue::Arr) {
            for (auto& u : urlV->arr) if (u && u->type == JsonValue::Str) urls.push_back(Utf8ToWide(u->str));
        }
        if (urls.empty()) { LogFmt("pkg %s empty url", name.c_str()); continue; }

        const std::string& remoteVer = verV->str;
        auto it = localVersions.find(name);
        const std::string localVer = (it != localVersions.end()) ? it->second : std::string();

        if (CompareVersion(remoteVer, localVer) <= 0) {
            LogFmt("pkg %s up-to-date: local=%s remote=%s", name.c_str(), localVer.c_str(), remoteVer.c_str());
            continue;
        }
        LogFmt("pkg %s has update: local=%s remote=%s", name.c_str(), localVer.c_str(), remoteVer.c_str());

        std::wstring zipPath = GetDownloadDir() + L"\\" + Utf8ToWide(name) + L"-" + Utf8ToWide(remoteVer) + L".7z";

        // 优先复用已缓存且校验通过的包，避免重复下载（上次 extract/apply 失败会保留包在此）
        bool reused = false;
        if (PathFileExistsW(zipPath.c_str())) {
            std::string cached = Sha256File(zipPath);
            if (!cached.empty() && _stricmp(cached.c_str(), shaV->str.c_str()) == 0) {
                reused = true;
                LogFmt("pkg %s reuse cached package (%llu bytes), skip download", name.c_str(), FileSizeBytes(zipPath));
            } else {
                LogFmt("pkg %s cached package invalid, redownload", name.c_str());
            }
        }
        if (!reused) {
            LogFmt("downloading %s package: %zu urls", name.c_str(), urls.size());
            if (!DownloadFile(urls, zipPath, nullptr)) {
                LogFmt("download %s failed, package kept for resume", name.c_str());
                failed = true;
                break;
            }
            LogFmt("downloaded %s: %llu bytes", name.c_str(), FileSizeBytes(zipPath));

            std::string actual = Sha256File(zipPath);
            if (_stricmp(actual.c_str(), shaV->str.c_str()) != 0) {
                LogFmt("sha256 mismatch for %s: expected=%s actual=%s", name.c_str(), shaV->str.c_str(), actual.c_str());
                failed = true;  // 保留文件，下次 aria2 覆盖重下（断点续传机制自动处理）
                break;
            }
            LogFmt("sha256 ok for %s", name.c_str());
        }

        PendingPackage p;
        p.name = name;
        p.version = remoteVer;
        p.zipPath = zipPath;
        p.sha256 = shaV->str;
        auto* kp = pkgVal->Find("killProcesses");
        if (kp && kp->type == JsonValue::Arr) {
            for (auto& ke : kp->arr) if (ke && ke->type == JsonValue::Str) p.killProcesses.push_back(Utf8ToWide(ke->str));
        }
        auto* kpa = pkgVal->Find("killProcessesAdmin");
        if (kpa && kpa->type == JsonValue::Arr) {
            for (auto& ke : kpa->arr) if (ke && ke->type == JsonValue::Str) p.killProcessesAdmin.push_back(Utf8ToWide(ke->str));
        }
        pending.push_back(p);
    }

    if (failed) {
        // 保留已下载的包（位于 Update 目录），下次复用，避免重复下载
        LogFmt("check aborted, %zu packages kept for retry", pending.size());
        return kError;
    }

    if (pending.empty()) {
        Log("already up-to-date");
        return kUpToDate;
    }

    // 按应用顺序排序（web -> modules -> wallpaper -> platform -> main）
    std::sort(pending.begin(), pending.end(), [](const PendingPackage& a, const PendingPackage& b) {
        return ApplyOrderIndex(a.name) < ApplyOrderIndex(b.name);
    });

    plan.packages = std::move(pending);
    WriteUpdateReady(plan);
    LogFmt("update.ready written, %zu packages", plan.packages.size());
    return kReady;
}

int RunCheck(const std::wstring& appDir) {
    Log("--check start");
    return DoCheck(appDir) == kError ? 1 : 0;
}

int RunApply(const std::wstring& appDir, bool forceKillMain) {
    Log("--apply start");
    UpdatePlan plan;
    if (!ReadUpdateReady(plan)) { Log("no update.ready"); return 1; }
    LogFmt("apply %zu packages", plan.packages.size());

    KillChildProcesses();

    const size_t pkgTotal = plan.packages.size();
    size_t pkgIdx = 0;
    for (auto& p : plan.packages) {
        LogFmt("apply pkg %zu/%zu: %s version=%s", ++pkgIdx, pkgTotal, p.name.c_str(), p.version.c_str());
        KillPackageProcesses(p.killProcesses, p.killProcessesAdmin);
        if (!PathFileExistsW(p.zipPath.c_str()))
            LogFmt("warning: zip missing before extract: %ls", p.zipPath.c_str());
        else
            LogFmt("zip ready: %ls (%llu bytes)", p.zipPath.c_str(), FileSizeBytes(p.zipPath));
        std::wstring staging = GetStagingDir() + L"\\" + Utf8ToWide(p.name);
        RemoveDirRecursive(staging);
        if (!ExtractArchive(p.zipPath, staging)) {
            LogFmt("extract %s failed, package kept for retry", p.name.c_str());
            RemoveDirRecursive(staging);
            continue;
        }
        bool ok = ApplyStaging(appDir, staging);
        RemoveDirRecursive(staging);
        if (ok) {
            UpdateLocalVersion(p.name, p.version);
            DeleteFileW(p.zipPath.c_str());  // 仅成功才清除下载包
            LogFmt("pkg %s applied, version updated, package cleaned", p.name.c_str());
        } else {
            LogFmt("pkg %s apply had failures (partial), version not updated, package kept for retry", p.name.c_str());
        }
    }

    // 先删 update.ready 再 LaunchMain：避免新主进程 PreRun 检测到 ready 再次拉起 --apply
    DeleteFileW((GetUpdateDir() + L"\\update.ready").c_str());

    LaunchMain(appDir);

    Log("--apply done");
    return 0;
}

int RunCheckAndApply(const std::wstring& appDir) {
    Log("--check-and-apply start");

    // 本轮检查开始，先清掉上一轮的结果标记，避免托盘读到过期结果
    ClearCheckResultMark();

    UpdatePlan plan;
    if (ReadUpdateReady(plan)) {
        Log("found existing update.ready, apply directly");
        return RunApply(appDir, true);
    }

    CheckResult cr = DoCheck(appDir);
    if (cr == kReady) {
        return RunApply(appDir, true);
    }
    // 无更新 / 检查失败都要留痕：托盘轮询回到 idle 时才能给出对应提示
    WriteCheckResultMark(cr == kUpToDate ? "uptodate" : "error");
    return cr == kError ? 1 : 0;
}

} // namespace updater

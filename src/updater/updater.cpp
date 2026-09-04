/*****************************************************************************
*  Update flow: version check, download, verification, staging
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "updater.h"
#include "http_client.h"
#include "hasher.h"
#include "zip_extract.h"
#include "applying.h"

#include "version.h"



namespace updater {

// Data root / install target = the directory that holds the updater exe itself (the data follows the exe: whatever it was installed or extracted into is what gets updated,
// so it does not depend on an externally supplied appDir - that argument does not exist yet during the --check stage)
static std::wstring GetAppDataRoot() {
    wchar_t self[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    std::wstring path(self);
    size_t pos = path.find_last_of(L"\\/");
    if (pos == std::wstring::npos) return std::wstring();
    return path.substr(0, pos);
}

// Ensures the directory exists and returns it (created level by level, so the Update subtree is built even when it is missing under the exe directory)
static std::wstring EnsureDir(const std::wstring& dir) {
    EnsureDirTree(dir);
    return dir;
}

// Update target / launch directory of the main program: the directory that holds the updater exe. Also the base used to locate tools such as Platform\7z and aria2c
std::wstring GetInstallDir() {
    return EnsureDir(GetAppDataRoot());
}

// Root of the update workspace: both update.ready and update.json land here
std::wstring GetUpdateDir() {
    return EnsureDir(GetAppDataRoot() + L"\\Update");
}

// Persistent cache of downloaded packages: kept across processes and reused through sha256 so nothing is downloaded twice
std::wstring GetDownloadDir() {
    return EnsureDir(GetUpdateDir() + L"\\downloads");
}

// Staging root for extraction: every package is extracted to staging\<name> and deleted once it has been applied
std::wstring GetStagingDir() {
    return EnsureDir(GetUpdateDir() + L"\\staging");
}

// Working copy of the extraction tools: 7z.exe/7z.dll are copied here from appDir and run from here, so the sources are not locked while being overwritten by a self-update
std::wstring GetToolsDir() {
    return EnsureDir(GetUpdateDir() + L"\\tools");
}

// Fallback base for the update.json source (used when Update.json is missing; the official GitHub source). Mirrors are configured through update_config.json.
static const wchar_t* FALLBACK_UPDATE_JSON_BASE = L"https://raw.githubusercontent.com/jearry/yyzTools/main";
// GitHub base of the release assets (update.json only stores the suffix v<ver>/<file>; C++ composes base + suffix + mirror)
static const wchar_t* RELEASE_BASE = L"https://github.com/jearry/yyzTools/releases/download";

// Package apply order: resources first, the platform sub-packages in the middle, and main (which contains the yyzTools.exe / yyzUpdater.exe self-update) last
static const char* APPLY_ORDER[] = { "web", "modules", "wallpaper", "yyztools", "tools", "yyzfilesearch", "rapidocr", "ffmpeg", "main" };

struct PendingPackage {
    std::string name;
    std::string version;
    std::wstring zipPath;
    std::string sha256;
    std::vector<std::wstring> killProcesses;       // processes to kill with normal rights before this package is applied
    std::vector<std::wstring> killProcessesAdmin;  // processes to kill with administrator rights before this package is applied
    std::vector<std::wstring> stopServices;        // services to stop before this package is applied (all restarted once every package is applied, e.g. yyzFileSearchSvc)
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

// ---- JSON read/write helpers (nlohmann; serialization escapes automatically, so backslashes in paths need no manual handling) ----

// Parses JSON text; returns a default-constructed value on failure or when the root is not an object, check with is_object()
static nlohmann::ordered_json ParseJsonObject(const std::string& text) {
    auto root = nlohmann::ordered_json::parse(text, nullptr, false);
    return (root.is_discarded() || !root.is_object()) ? nlohmann::ordered_json() : root;
}

// Reads a string field of an object; returns nullopt when it is missing or of the wrong type
static std::optional<std::string> JsonStr(const nlohmann::ordered_json& obj, const char* key) {
    auto it = obj.find(key);
    if (it == obj.end() || !it->is_string()) return std::nullopt;
    return it->get<std::string>();
}

// Reads a string array field of an object; returns an empty array when it is missing or of the wrong type
static std::vector<std::wstring> JsonStrArray(const nlohmann::ordered_json& obj, const char* key) {
    std::vector<std::wstring> out;
    auto it = obj.find(key);
    if (it != obj.end() && it->is_array())
        for (auto& e : *it)
            if (e.is_string()) out.push_back(Utf8ToWide(e.get<std::string>()));
    return out;
}

// Converts a string array to a JSON array
static nlohmann::ordered_json ToJsonArray(const std::vector<std::wstring>& arr) {
    auto out = nlohmann::ordered_json::array();
    for (auto& s : arr) out.push_back(WideToUtf8(s));
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

// Returns the file size in bytes; 0 when it does not exist or cannot be accessed.
static unsigned long long FileSizeBytes(const std::wstring& path) {
    WIN32_FILE_ATTRIBUTE_DATA ad;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &ad)) return 0;
    return ((unsigned long long)ad.nFileSizeHigh << 32) | (unsigned long long)ad.nFileSizeLow;
}

// ---- Client mirror configuration (<exe directory>\Config\Update.json) ----

// Path of the client_update.json mirror configuration (stored in the Update directory alongside client_versions.json)
static std::wstring GetConfigPath() {
    return GetUpdateDir() + L"\\client_update.json";
}

struct MirrorConfig {
    std::vector<std::wstring> updateJsonBases;   // update.json source bases (before main), suffixed with /releases/update.json
    std::vector<std::wstring> releaseMirrors;    // mirror prefixes of the release assets, composed as <prefix>/realURL
};

// Reads the mirror configuration. When the configuration file does not exist the defaults are used and written out (so the user can edit it).
// updateJsonBases: download sources of update.json (suffixed with /releases/update.json)
// releaseMirrors: mirror prefixes of the release assets (composed as <prefix>/realURL)
static MirrorConfig ReadMirrorConfig() {
    MirrorConfig cfg;
    std::wstring path = GetConfigPath();
    std::string text = ReadTextFile(path);
    if (!text.empty()) {
        auto root = ParseJsonObject(text);
        if (root.is_object()) {
            cfg.updateJsonBases = JsonStrArray(root, "updateJsonBases");
            cfg.releaseMirrors = JsonStrArray(root, "releaseMirrors");
        }
    }
    if (cfg.updateJsonBases.empty() && cfg.releaseMirrors.empty()) {
        // Update.json missing: fall back to the official GitHub source and write it out so the user can edit it
        cfg.updateJsonBases.push_back(FALLBACK_UPDATE_JSON_BASE);
        std::ofstream f(path);
        if (f) {
            f << "{\n  \"updateJsonBases\": [\n    \"" << WideToUtf8(cfg.updateJsonBases[0]) << "\"\n  ],\n  \"releaseMirrors\": []\n}\n";
        }
    }
    return cfg;
}

// Composes the multi-source list from the real URL plus the mirror prefixes: [realUrl, mirror1/realUrl, mirror2/realUrl, ...]
static std::vector<std::wstring> BuildMultiSourceUrls(const std::wstring& realUrl, const std::vector<std::wstring>& mirrors) {
    std::vector<std::wstring> urls;
    urls.reserve(mirrors.size() + 1);
    urls.push_back(realUrl);
    for (auto& m : mirrors) urls.push_back(m + L"/" + realUrl);
    return urls;
}

// ---- Local version file versions.json ({app}\update\versions.json; the exe is no longer read) ----

static std::map<std::string, std::string> ReadLocalVersions() {
    std::map<std::string, std::string> versions;
    std::string text = ReadTextFile(GetUpdateDir() + L"\\client_versions.json");
    if (text.empty()) return versions;
    auto root = ParseJsonObject(text);
    for (auto it = root.begin(); it != root.end(); ++it)
        if (it.value().is_string()) versions[it.key()] = it.value().get<std::string>();
    return versions;
}

static bool SaveVersions(const std::map<std::string, std::string>& versions) {
    std::ofstream f(GetUpdateDir() + L"\\client_versions.json");
    if (!f) return false;
    nlohmann::ordered_json root = nlohmann::ordered_json::object();
    for (auto& kv : versions) root[kv.first] = kv.second;
    f << root.dump(2) << "\n";
    return true;
}

static bool UpdateLocalVersion(const std::string& pkgName, const std::string& version) {
    auto versions = ReadLocalVersions();
    versions[pkgName] = version;
    return SaveVersions(versions);
}

// ---- update.ready (multi-package JSON) ----

static bool WriteUpdateReady(const UpdatePlan& plan) {
    auto packages = nlohmann::ordered_json::array();
    for (auto& p : plan.packages) {
        packages.push_back({
            {"name", p.name},
            {"version", p.version},
            {"zipPath", WideToUtf8(p.zipPath)},
            {"sha256", p.sha256},
            {"killProcesses", ToJsonArray(p.killProcesses)},
            {"killProcessesAdmin", ToJsonArray(p.killProcessesAdmin)},
            {"stopService", ToJsonArray(p.stopServices)},
        });
    }
    nlohmann::ordered_json root;
    root["packages"] = std::move(packages);
    std::ofstream f(GetUpdateDir() + L"\\update.ready", std::ios::binary | std::ios::trunc);
    if (!f) return false;
    f << root.dump(2) << "\n";
    return true;
}

// ---- update.result (one-shot result marker of a manual check) ----
// Only a manual run (--check-and-apply) writes it; the tray reads it once and deletes it (NativeApi::GetUpdateState).
// A silent --check does not write it, otherwise the tray would unexpectedly pop up "already up to date" the next time the user opens it.
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
    auto root = ParseJsonObject(text);

    auto arr = root.find("packages");
    if (arr == root.end() || !arr->is_array()) return false;
    for (auto& item : *arr) {
        if (!item.is_object()) continue;
        auto n = JsonStr(item, "name");
        auto v = JsonStr(item, "version");
        auto z = JsonStr(item, "zipPath");
        if (!n || !v || !z) continue;
        PendingPackage p;
        p.name = *n;
        p.version = *v;
        p.zipPath = Utf8ToWide(*z);
        if (auto s = JsonStr(item, "sha256")) p.sha256 = *s;
        p.killProcesses = JsonStrArray(item, "killProcesses");
        p.killProcessesAdmin = JsonStrArray(item, "killProcessesAdmin");
        p.stopServices = JsonStrArray(item, "stopService");
        plan.packages.push_back(std::move(p));
    }
    return !plan.packages.empty();
}

enum CheckResult { kUpToDate, kReady, kError };

// Cleans up orphaned downloads in the downloads directory: keeps the files of the current plan (including the aria2 resume control files) and deletes everything else.
// Package file names carry the version (<package>-<version>.7z), so once the remote version jumps (e.g. 1.0.5 is published right after 1.0.4 failed),
// the older file is never referenced by any plan again - the DeleteFileW of a successful apply only knows its own exact path and cannot remove them.
// Therefore each check does one whitelist sweep once the plan is settled; with no updates the whitelist is empty, which clears the whole directory.
//
// Resuming is unaffected: a download failure takes the failed branch of DoCheck, which returns directly without calling this function,
// leaving the partial .7z and its .aria2 for aria2 to resume on the next round.
static void CleanStaleDownloads(const std::vector<PendingPackage>& keep) {
    std::wstring dir = GetDownloadDir();
    std::set<std::wstring> keepNames;
    for (auto& p : keep) {
        size_t pos = p.zipPath.find_last_of(L"\\/");
        std::wstring file = (pos == std::wstring::npos) ? p.zipPath : p.zipPath.substr(pos + 1);
        keepNames.insert(file);
        keepNames.insert(file + L".aria2");  // the resume control file is kept along with the package itself
    }

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    size_t removed = 0;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (keepNames.count(fd.cFileName)) continue;
        std::wstring full = dir + L"\\" + fd.cFileName;
        if (DeleteFileW(full.c_str())) {
            removed++;
            InfoMsg("removed stale download: %ls", fd.cFileName);
        } else {
            InfoMsg("remove stale download failed (err=%lu): %ls", GetLastError(), fd.cFileName);
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
    if (removed) InfoMsg("cleaned %zu stale download(s)", removed);
}

static int ApplyOrderIndex(const std::string& name) {
    for (size_t i = 0; i < std::size(APPLY_ORDER); i++)
        if (APPLY_ORDER[i] == name) return (int)i;
    return 99;
}

static CheckResult DoCheck() {
    auto cfg = ReadMirrorConfig();
    InfoMsg("mirror config: %zu updateJsonBases, %zu releaseMirrors",
           cfg.updateJsonBases.size(), cfg.releaseMirrors.size());
    for (size_t i = 0; i < cfg.updateJsonBases.size(); i++)
        InfoMsg("  updateJsonBase[%zu] = %ls", i, cfg.updateJsonBases[i].c_str());
    for (size_t i = 0; i < cfg.releaseMirrors.size(); i++)
        InfoMsg("  releaseMirror[%zu] = %ls", i, cfg.releaseMirrors[i].c_str());

    auto localVersions = ReadLocalVersions();
    InfoMsg("local versions: %zu packages", localVersions.size());

    std::vector<std::wstring> jsonUrls;
    for (auto& base : cfg.updateJsonBases) jsonUrls.push_back(base + L"/releases/update.json");
    std::string jsonText = DownloadText(jsonUrls);
    if (jsonText.empty()) {
        InfoMsg("%s", "fetch update.json failed");
        return kError;
    }
    InfoMsg("update.json fetched: %zu bytes", jsonText.size());

    // Cache the raw update.json in the Update directory so the main process can read the non-update fields from it (such as the home ad slot configuration).
    // Cache only: failing to write it does not affect the rest of the update flow.
    {
        std::ofstream f(GetUpdateDir() + L"\\update.json", std::ios::binary | std::ios::trunc);
        if (f) f << jsonText;
    }

    auto root = ParseJsonObject(jsonText);
    if (!root.is_object()) {
        InfoMsg("%s", "parse update.json failed");
        return kError;
    }

    auto pkgs = root.find("packages");
    if (pkgs == root.end() || !pkgs->is_object()) { InfoMsg("%s", "no packages"); return kError; }
    InfoMsg("update.json has %zu packages", pkgs->size());

    UpdatePlan plan;

    std::vector<PendingPackage> pending;
    bool failed = false;
    const size_t pkgTotal = pkgs->size();
    size_t pkgIdx = 0;
    for (auto pkgIt = pkgs->begin(); pkgIt != pkgs->end(); ++pkgIt) {
        const std::string& name = pkgIt.key();
        InfoMsg("check pkg %zu/%zu: %s", ++pkgIdx, pkgTotal, name.c_str());
        const auto& pkgVal = pkgIt.value();
        if (!pkgVal.is_object()) continue;
        auto verV = JsonStr(pkgVal, "version");
        if (!verV) { InfoMsg("pkg %s missing version", name.c_str()); continue; }
        auto shaV = JsonStr(pkgVal, "sha256");
        if (!shaV) {
            InfoMsg("pkg %s missing url/sha256", name.c_str()); continue;
        }
        // url holds only the real URL (a string); the mirrors are composed from the client configuration. The array form is also accepted (used as-is).
        std::vector<std::wstring> urls;
        auto urlIt = pkgVal.find("url");
        if (urlIt != pkgVal.end() && urlIt->is_string()) {
            // url holds only the suffix (v<ver>/<file>), which is appended to RELEASE_BASE; a full URL (starting with https://) is also accepted
            std::wstring suffix = Utf8ToWide(urlIt->get<std::string>());
            std::wstring realUrl = (suffix.find(L"https://") == 0 || suffix.find(L"http://") == 0)
                ? suffix : (std::wstring(RELEASE_BASE) + L"/" + suffix);
            urls = BuildMultiSourceUrls(realUrl, cfg.releaseMirrors);
        } else if (urlIt != pkgVal.end() && urlIt->is_array()) {
            for (auto& u : *urlIt) if (u.is_string()) urls.push_back(Utf8ToWide(u.get<std::string>()));
        }
        if (urls.empty()) { InfoMsg("pkg %s empty url", name.c_str()); continue; }

        const std::string& remoteVer = *verV;
        auto lit = localVersions.find(name);
        const std::string localVer = (lit != localVersions.end()) ? lit->second : std::string();

        if (CompareVersion(remoteVer, localVer) <= 0) {
            InfoMsg("pkg %s up-to-date: local=%s remote=%s", name.c_str(), localVer.c_str(), remoteVer.c_str());
            continue;
        }
        InfoMsg("pkg %s has update: local=%s remote=%s", name.c_str(), localVer.c_str(), remoteVer.c_str());

        std::wstring zipPath = GetDownloadDir() + L"\\" + Utf8ToWide(name) + L"-" + Utf8ToWide(remoteVer) + L".7z";

        // Prefer reusing a cached package that passes verification, to avoid downloading it twice (a failed extract/apply leaves the package here)
        bool reused = false;
        if (PathFileExistsW(zipPath.c_str())) {
            std::string cached = Sha256File(zipPath);
            if (!cached.empty() && _stricmp(cached.c_str(), shaV->c_str()) == 0) {
                reused = true;
                InfoMsg("pkg %s reuse cached package (%llu bytes), skip download", name.c_str(), FileSizeBytes(zipPath));
            } else {
                InfoMsg("pkg %s cached package invalid, redownload", name.c_str());
            }
        }
        if (!reused) {
            InfoMsg("downloading %s package: %zu urls", name.c_str(), urls.size());
            if (!DownloadFile(urls, zipPath, nullptr)) {
                InfoMsg("download %s failed, package kept for resume", name.c_str());
                failed = true;
                break;
            }
            InfoMsg("downloaded %s: %llu bytes", name.c_str(), FileSizeBytes(zipPath));

            std::string actual = Sha256File(zipPath);
            if (_stricmp(actual.c_str(), shaV->c_str()) != 0) {
                InfoMsg("sha256 mismatch for %s: expected=%s actual=%s", name.c_str(), shaV->c_str(), actual.c_str());
                failed = true;  // the file is kept, aria2 overwrites and re-downloads it next time (handled automatically by the resume mechanism)
                break;
            }
            InfoMsg("sha256 ok for %s", name.c_str());
        }

        PendingPackage p;
        p.name = name;
        p.version = remoteVer;
        p.zipPath = zipPath;
        p.sha256 = *shaV;
        p.killProcesses = JsonStrArray(pkgVal, "killProcesses");
        p.killProcessesAdmin = JsonStrArray(pkgVal, "killProcessesAdmin");
        p.stopServices = JsonStrArray(pkgVal, "stopService");
        pending.push_back(p);
    }

    if (failed) {
        // Keep the packages that were already downloaded (they live in the Update directory) so they can be reused next time instead of being downloaded again.
        // Orphans are deliberately not cleaned here: the pending list we broke out of only covers up to the failing package, the ones after it were never checked,
        // and using it as a whitelist would delete caches that should have been reused. Defer the cleanup to the next successful check.
        InfoMsg("check aborted, %zu packages kept for retry", pending.size());
        return kError;
    }

    if (pending.empty()) {
        CleanStaleDownloads(pending);  // no updates -> the whitelist is empty -> the directory is supposed to be empty anyway, clear it all
        InfoMsg("%s", "already up-to-date");
        return kUpToDate;
    }

    // Sort into apply order (web -> modules -> wallpaper -> platform -> main)
    std::sort(pending.begin(), pending.end(), [](const PendingPackage& a, const PendingPackage& b) {
        return ApplyOrderIndex(a.name) < ApplyOrderIndex(b.name);
    });

    CleanStaleDownloads(pending);  // the plan is settled, remove the leftovers that do not belong to this round

    plan.packages = std::move(pending);
    if (!WriteUpdateReady(plan)) {
        InfoMsg("write update.ready failed, %zu packages kept for retry", plan.packages.size());
        return kError;
    }
    InfoMsg("update.ready written, %zu packages", plan.packages.size());
    return kReady;
}

int RunCheck() {
    InfoMsg("%s", "--check start");
    return DoCheck() == kError ? 1 : 0;
}

int RunApply(const std::wstring& appDir) {
    InfoMsg("%s", "--apply start");
    UpdatePlan plan;
    if (!ReadUpdateReady(plan)) { InfoMsg("%s", "no update.ready"); return 1; }
    InfoMsg("apply %zu packages", plan.packages.size());

    KillChildProcesses();

    // Record the services actually stopped in this round and restart them all once every package has been applied
    std::set<std::wstring> stoppedServices;

    const size_t pkgTotal = plan.packages.size();
    size_t pkgIdx = 0;
    for (auto& p : plan.packages) {
        InfoMsg("apply pkg %zu/%zu: %s version=%s", ++pkgIdx, pkgTotal, p.name.c_str(), p.version.c_str());
        KillPackageProcesses(p.killProcesses, p.killProcessesAdmin);
        for (auto& s : p.stopServices) {
            if (StopServiceByName(s)) stoppedServices.insert(s);
        }
        if (!PathFileExistsW(p.zipPath.c_str()))
            InfoMsg("warning: zip missing before extract: %ls", p.zipPath.c_str());
        else
            InfoMsg("zip ready: %ls (%llu bytes)", p.zipPath.c_str(), FileSizeBytes(p.zipPath));
        std::wstring staging = GetStagingDir() + L"\\" + Utf8ToWide(p.name);
        RemoveDirRecursive(staging);
        if (!ExtractArchive(p.zipPath, staging)) {
            InfoMsg("extract %s failed, package kept for retry", p.name.c_str());
            RemoveDirRecursive(staging);
            continue;
        }
        bool ok = ApplyStaging(appDir, staging);
        RemoveDirRecursive(staging);
        if (ok) {
            UpdateLocalVersion(p.name, p.version);
            DeleteFileW(p.zipPath.c_str());  // the downloaded package is removed only on success
            InfoMsg("pkg %s applied, version updated, package cleaned", p.name.c_str());
        } else {
            InfoMsg("pkg %s apply had failures (partial), version not updated, package kept for retry", p.name.c_str());
        }
    }

    // Restart the services that were stopped, now that everything has been applied
    for (auto& s : stoppedServices) {
        StartServiceByName(s);
    }

    // Delete update.ready before LaunchMain: this stops the new main process from detecting ready in PreRun and launching --apply a second time
    DeleteFileW((GetUpdateDir() + L"\\update.ready").c_str());

    LaunchMain(appDir);

    InfoMsg("%s", "--apply done");
    return 0;
}

int RunCheckAndApply(const std::wstring& appDir) {
    InfoMsg("%s", "--check-and-apply start");

    // A round starts by clearing the result marker of the previous round, so the tray cannot read a stale result
    ClearCheckResultMark();

    UpdatePlan plan;
    if (ReadUpdateReady(plan)) {
        InfoMsg("%s", "found existing update.ready, apply directly");
        return RunApply(appDir);
    }

    CheckResult cr = DoCheck();
    if (cr == kReady) {
        return RunApply(appDir);
    }
    // Both "no updates" and "check failed" have to leave a trace: the tray poll only returns to idle then, which is when the matching message can be shown
    WriteCheckResultMark(cr == kUpToDate ? "uptodate" : "error");
    return cr == kError ? 1 : 0;
}

} // namespace updater

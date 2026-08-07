#pragma once
#include <string>
#include <functional>

namespace updater {

// Via Platform/Aria2 multi-connection GET. Returns content; empty on failure.
std::string DownloadText(const std::wstring& url);

// Via Platform/Aria2. Downloads url to savePath. progress currently unused.
bool DownloadFile(const std::wstring& url, const std::wstring& savePath,
                  std::function<void(int)> progress = nullptr);

} // namespace updater

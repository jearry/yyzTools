#pragma once
#include <string>
#include <vector>
#include <functional>

namespace updater {

// Via Platform/Aria2 multi-source GET (urls 指向同一文件). Returns content; empty on failure.
std::string DownloadText(const std::vector<std::wstring>& urls);

// Via Platform/Aria2. Downloads urls (multi-source, 并行从多源拉取合并) to savePath.
// progress currently unused.
bool DownloadFile(const std::vector<std::wstring>& urls, const std::wstring& savePath,
                  std::function<void(int)> progress = nullptr);

} // namespace updater

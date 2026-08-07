#pragma once
#include <string>

namespace updater {

// Extract archive via Platform/7z into destDir. The 7z tool is copied to a temp
// copy first so it is never overwritten mid-extraction. Returns true on success.
bool ExtractArchive(const std::wstring& archivePath, const std::wstring& destDir);

} // namespace updater

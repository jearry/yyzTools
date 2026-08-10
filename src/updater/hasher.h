#pragma once
#include <string>
#include <cstdint>

namespace updater {

std::string Sha256File(const std::wstring& path);      // 返回小写 hex，失败空串
std::string Sha256Data(const uint8_t* data, size_t len);

} // namespace updater

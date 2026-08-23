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
#include "hasher.h"
#include "logger.h"

#pragma comment(lib, "bcrypt.lib")

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif

namespace updater {

static std::string ToHex(const uint8_t* data, size_t len) {
    static const char* hex = "0123456789abcdef";
    std::string s(len * 2, '0');
    for (size_t i = 0; i < len; i++) {
        s[i * 2]     = hex[data[i] >> 4];
        s[i * 2 + 1] = hex[data[i] & 0xF];
    }
    return s;
}

std::string Sha256Data(const uint8_t* data, size_t len) {
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) != STATUS_SUCCESS) return {};

    BCRYPT_HASH_HANDLE hHash = nullptr;
    std::string result;
    if (BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0) == STATUS_SUCCESS) {
        if (BCryptHashData(hHash, (PUCHAR)data, (ULONG)len, 0) == STATUS_SUCCESS) {
            uint8_t hash[32];
            if (BCryptFinishHash(hHash, hash, 32, 0) == STATUS_SUCCESS)
                result = ToHex(hash, 32);
        }
        BCryptDestroyHash(hHash);
    }
    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

std::string Sha256File(const std::wstring& path) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        LogFmt("Sha256File open failed: %lu", GetLastError());
        return {};
    }

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    std::string result;
    bool algOk = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, nullptr, 0) == STATUS_SUCCESS;
    bool hashOk = algOk && BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0) == STATUS_SUCCESS;

    if (hashOk) {
        uint8_t buf[65536];
        DWORD rd = 0;
        bool ok = true;
        while (ReadFile(hFile, buf, sizeof(buf), &rd, nullptr) && rd > 0) {
            if (BCryptHashData(hHash, buf, rd, 0) != STATUS_SUCCESS) { ok = false; break; }
        }
        if (ok) {
            uint8_t hash[32];
            if (BCryptFinishHash(hHash, hash, 32, 0) == STATUS_SUCCESS)
                result = ToHex(hash, 32);
        }
        BCryptDestroyHash(hHash);
    }
    if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
    CloseHandle(hFile);
    return result;
}

} // namespace updater

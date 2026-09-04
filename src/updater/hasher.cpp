/*****************************************************************************
*  File SHA256 verification
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "hasher.h"

using namespace yyzlib;

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

std::string Sha256File(const std::wstring& path) {
    HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        InfoMsg("Sha256File open failed: %lu", GetLastError());
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

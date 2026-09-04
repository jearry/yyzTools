/*****************************************************************************
*  Checksums (CRC32 / CRC16 / Checksum)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "HashUtil.h"

#include <bcrypt.h>


namespace yyzlib
{
	namespace
	{
		// CRC32 (IEEE 802.3, reflected polynomial 0xEDB88320, init 0xFFFFFFFF, final XOR 0xFFFFFFFF)
		uint32_t crc32_table[256];
		bool crc32_table_inited = false;

		void InitCrc32Table()
		{
			for (uint32_t i = 0; i < 256; ++i) {
				uint32_t crc = i;
				for (int j = 0; j < 8; ++j) {
					crc = (crc & 1) ? (0xEDB88320u ^ (crc >> 1)) : (crc >> 1);
				}
				crc32_table[i] = crc;
			}
			crc32_table_inited = true;
		}

		inline uint32_t Crc32Update(uint32_t crc, const uint8_t* data, size_t size)
		{
			for (size_t i = 0; i < size; ++i) {
				crc = crc32_table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
			}
			return crc;
		}

		// CRC16 (same algorithm as the original src/public/crc16.cpp: 4-bit table, no reflection)
		const uint16_t crc16_table[16] = {
			0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
			0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
		};

		inline uint16_t Crc16Byte(uint16_t crc, uint8_t byte)
		{
			uint16_t temp;

			temp = crc16_table[crc & 0xF];
			crc = (crc >> 4u) & 0x0FFFu;
			crc = crc ^ temp ^ crc16_table[byte & 0xF];

			temp = crc16_table[crc & 0xF];
			crc = (crc >> 4u) & 0x0FFFu;
			crc = crc ^ temp ^ crc16_table[(byte >> 4u) & 0xF];

			return crc;
		}

		inline uint16_t Crc16Update(uint16_t crc, const uint8_t* data, size_t size)
		{
			for (size_t i = 0; i < size; ++i) {
				crc = Crc16Byte(crc, data[i]);
			}
			return crc;
		}

		inline uint32_t Sum32Update(uint32_t sum, const uint8_t* data, size_t size)
		{
			for (size_t i = 0; i < size; ++i) {
				sum += data[i];
			}
			return sum;
		}

		// Read the file in chunks, feeding each chunk to the update function
		template<typename T, typename UpdateFn>
		T CalcFileChecksum(const tstring& filepath, T init, UpdateFn update)
		{
			HANDLE hFile = CreateFileW(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ,
				nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (hFile == INVALID_HANDLE_VALUE) {
				return init;
			}

			constexpr DWORD kChunkSize = 64 * 1024;
			std::vector<uint8_t> buf(kChunkSize);
			T value = init;

			for (;;) {
				DWORD readed = 0;
				if (!ReadFile(hFile, buf.data(), kChunkSize, &readed, nullptr) || readed == 0) {
					break;
				}
				value = update(value, buf.data(), static_cast<size_t>(readed));
			}

			CloseHandle(hFile);
			return value;
		}
	}

	uint32_t CalcCrc32(const void* data, size_t size)
	{
		if (!crc32_table_inited) {
			InitCrc32Table();
		}
		uint32_t crc = 0xFFFFFFFFu;
		crc = Crc32Update(crc, static_cast<const uint8_t*>(data), size);
		return crc ^ 0xFFFFFFFFu;
	}

	uint16_t CalcCrc16(const void* data, size_t size)
	{
		uint16_t crc = 0;
		return Crc16Update(crc, static_cast<const uint8_t*>(data), size);
	}

	uint32_t CalcSum32(const void* data, size_t size)
	{
		uint32_t sum = 0;
		return Sum32Update(sum, static_cast<const uint8_t*>(data), size);
	}

	uint32_t CalcFileCrc32(const tstring& filepath)
	{
		if (!crc32_table_inited) {
			InitCrc32Table();
		}
		uint32_t crc = 0xFFFFFFFFu;
		crc = CalcFileChecksum<uint32_t>(filepath, crc, [](uint32_t v, const uint8_t* d, size_t s) {
			return Crc32Update(v, d, s);
		});
		return crc ^ 0xFFFFFFFFu;
	}

	uint16_t CalcFileCrc16(const tstring& filepath)
	{
		uint16_t crc = 0;
		return CalcFileChecksum<uint16_t>(filepath, crc, [](uint16_t v, const uint8_t* d, size_t s) {
			return Crc16Update(v, d, s);
		});
	}

	uint32_t CalcFileSum32(const tstring& filepath)
	{
		uint32_t sum = 0;
		return CalcFileChecksum<uint32_t>(filepath, sum, [](uint32_t v, const uint8_t* d, size_t s) {
			return Sum32Update(v, d, s);
		});
	}

	bool HmacSha256(const void* key, size_t keylen, const void* data, size_t len, unsigned char out[32])
	{
		BCRYPT_ALG_HANDLE alg = nullptr;
		if (!BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_SHA256_ALGORITHM, nullptr, BCRYPT_ALG_HANDLE_HMAC_FLAG)))
			return false;

		bool ok = false;
		BCRYPT_HASH_HANDLE hash = nullptr;
		if (BCRYPT_SUCCESS(BCryptCreateHash(alg, &hash, nullptr, 0,
			(PUCHAR)key, (ULONG)keylen, 0))) {
			if (BCRYPT_SUCCESS(BCryptHashData(hash, (PUCHAR)data, (ULONG)len, 0)) &&
				BCRYPT_SUCCESS(BCryptFinishHash(hash, out, 32, 0))) {
				ok = true;
			}
			BCryptDestroyHash(hash);
		}
		BCryptCloseAlgorithmProvider(alg, 0);
		return ok;
	}
}

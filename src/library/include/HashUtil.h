/*****************************************************************************
*  Checksums (CRC32 / CRC16 / Checksum)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __YYZ_HASH_UTIL_H__
#define __YYZ_HASH_UTIL_H__

#include <cstdint>
#include <cstddef>

#include "TypeDefs.h"

namespace yyzlib
{
	// Buffer-based versions
	uint32_t CalcCrc32(const void* data, size_t size);
	uint16_t CalcCrc16(const void* data, size_t size);
	uint32_t CalcSum32(const void* data, size_t size);

	// File-based versions (chunked streaming, avoids loading whole large files into memory)
	uint32_t CalcFileCrc32(const tstring& filepath);
	uint16_t CalcFileCrc16(const tstring& filepath);
	uint32_t CalcFileSum32(const tstring& filepath);

	// HMAC-SHA256 (BCrypt implementation); out receives 32 bytes
	bool HmacSha256(const void* key, size_t keylen, const void* data, size_t len, unsigned char out[32]);
}

#endif

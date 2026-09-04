/*****************************************************************************
*  Common utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "TypeDefs.h"
#include "CommonUtil.h"
#include "WinUtil.h"
#include "FileUtil.h"

namespace yyzlib
{
	tstring GetAppBaseDir()
	{
		// Data root = the directory containing yyzTools.exe (single unified layout: data lives
		// wherever the app is installed/unpacked, staying compatible with existing install
		// directories). When the exe sits in the Platform\yyzTools subdirectory, normalize
		// two levels up; otherwise (Debug layout / host process / updater), use the exe
		// directory itself.
		tstring dir(GetAppDir());

		const TCHAR* suffix = _T("\\Platform\\yyzTools");
		size_t slen = _tcslen(suffix);
		if (dir.size() > slen && _tcsicmp(dir.c_str() + dir.size() - slen, suffix) == 0) {
			dir = dir.substr(0, dir.size() - slen);
		}

		DirCreate(dir);

		return dir;
	}

	tstring GetConfigFilePath(const tstring& app_name)
	{
		tstring path = GetAppBaseDir() + _T("\\Config\\");

		DirCreate(path);

		return path + app_name + _T(".json");
	}

	tstring GetLogFilePath(const tstring& mode_name)
	{
		tstring path = GetAppBaseDir() + _T("\\Logs\\");

		DirCreate(path);

		return path + mode_name + _T(".log");
	}

	tstring GetWebCachePath(const tstring& mode_name)
	{
		tstring path = GetAppBaseDir() + _T("\\WebCache\\");

		DirCreate(path);

		return path + mode_name;
	}

	std::unordered_map<std::string, int> g_auto_id;
	std::mutex g_auto_id_mutex;
	int GetAutoIncrementId(const std::string &key)
	{
		int ret = 0;
		std::lock_guard lock(g_auto_id_mutex);

		auto iter = g_auto_id.find(key);
		if (iter != g_auto_id.end()) {
			iter->second = iter->second + 1;

			ret = iter->second;
		}else {
			g_auto_id[key] = 0;
		}
		return ret;
	}
}

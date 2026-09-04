/*****************************************************************************
*  Type definitions
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_TYPEDEFS_H__
#define __XD_TYPEDEFS_H__

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>

namespace yyzlib
{

	typedef std::vector<std::string> StringList;
	
	typedef std::pair<std::string, std::string> StringPair;

	typedef std::vector<StringPair> StringPairList;
	
	typedef std::unordered_map<std::string, std::string> StringMap;
	
	typedef std::vector<StringMap> StringMapList;
	
	typedef std::unordered_set<std::string> StringSet;

	//
	
	typedef std::vector<std::wstring> WStringList;
	
	typedef std::pair<std::wstring, std::wstring> WStringPair;

	typedef std::vector<WStringPair> WStringPairList;
	
	typedef std::unordered_map<std::wstring, std::wstring> WStringMap;

	typedef std::vector<WStringMap> WStringMapList;

	typedef std::unordered_set<std::wstring> WStringSet;
	//
	
	typedef std::vector<uint8_t> ByteVector;

#ifdef UNICODE

	typedef std::wstring tstring;
	typedef WStringList TStringList;

	typedef WStringPair TStringPair;
	typedef WStringPairList TStringPairList;

	typedef WStringMap TStringMap;
	typedef WStringMapList TStringMapList;
	typedef WStringSet TStringSet;

#else
	typedef std::string tstring;
	typedef StringList TStringList;

	typedef StringPair TStringPair;
	typedef StringPairList TStringPairList;

	typedef StringMap TStringMap;
	typedef StringMapList TStringMapList;
	typedef StringSet TStringSet;

#endif

}

#endif /* __XD_TYPEDEFS_H__ */



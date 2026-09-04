/*****************************************************************************
*  Property tree utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_PTREE_UTIL_H__
#define __XD_PTREE_UTIL_H__

#include <string>
#include "RunLog.h"

#include "TypeDefs.h"
#include "Ptree.h"

namespace yyzlib
{
	//JSON string encoded in UTF-8
	std::string SaveToString(yyzlib::ptree &pt, bool pretty = true);

	//Parses a JSON string encoded in UTF-8
	bool LoadFromString(yyzlib::ptree &pt, const std::string &s);

	bool HasValue(yyzlib::ptree &pt, const std::string &key);

	bool SetValue(yyzlib::ptree &pt, const std::string &key, bool value);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, bool &value, bool default_value);

	bool SetValue(yyzlib::ptree &pt, const std::string &key, int value);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, int &value, int default_value);

	bool SetValue(yyzlib::ptree &pt, const std::string &key, uint32_t value);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, uint32_t &value, uint32_t default_value);

	bool SetValue(yyzlib::ptree& pt, const std::string& key, uint64_t value);

	bool GetValue(const yyzlib::ptree& pt, const std::string& key, uint64_t& value, uint64_t default_value);

	bool SetValue(yyzlib::ptree &pt, const std::string &key, float value);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, float &value, float default_value);

	bool SetValue(yyzlib::ptree &pt, const std::string &key, const char * value);
	bool SetValue(yyzlib::ptree &pt, const std::string &key, const std::string &value);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, std::string &value, const std::string &default_value);

	//String list
	bool SetValue(yyzlib::ptree &pt, const std::string &key, const StringList& strs);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, StringList &strs, const StringList& default_strs);

	bool SetValue(yyzlib::ptree &pt, const std::string &key, const StringMap& stru);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, StringMap &stru, const StringMap& default_stru);

	//List of custom-typed values
	bool SetValue(yyzlib::ptree &pt, const std::string &key, const StringMapList& strus);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, StringMapList &strus, const StringMapList& default_strus);

	//
	bool SetValue(yyzlib::ptree &pt, const std::string &key, const yyzlib::ptree &value);

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, yyzlib::ptree &value, const yyzlib::ptree& default_value);

	void Merge(yyzlib::ptree &object, const yyzlib::ptree &input);
	
}

#endif



/*****************************************************************************
*  Configuration definitions
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_CONFIG_DEF_H__
#define __XD_CONFIG_DEF_H__

#include <vector>
#include <map>
#include <unordered_map>
#include <variant>
#include "TypeDefs.h"
#include "Ptree.h"

namespace yyzlib
{
	/**
	* Config value type definition; supported types: bool, int, float, std::string, std::vector<std::string>
	*/
	typedef std::variant<bool, int, float, std::string, StringList, StringMap, StringMapList, yyzlib::ptree > ConfigValue;

	/**
	* Callback invoked to notify external code when a config entry is about to change
	*/
	typedef std::function<bool(const std::string& key, const ConfigValue& old_value, const ConfigValue& new_value)> ConfigTriggerPreFunc;


	/**
	* Callback invoked to notify external code after a config entry has changed
	*/
	typedef std::function<void()> ConfigTriggerPostFunc;

	/**
	* Config entry definition
	*/
	struct ConfigItem
	{
		std::string Key;                ///< Key of the config entry; wildcards are supported, mainly for mapping many config keys to one trigger
		ConfigValue DefaultValue;       ///< Default value of the config entry; string values must use std::string, not "ABC"
		//Both callbacks run outside the lock; their bodies may safely call lock-taking
		//ConfigMgr interfaces such as Get/Set/Save.
		//Note: (1) PreFunc's vote is based on the config values captured when the
		//snapshot is taken; a concurrent write that invalidates the snapshot reruns
		//the detection and PreFunc round (callbacks must tolerate repeated calls);
		//(2) do not modify, inside PostFunc, the same key that fired it, or the
		//callbacks will be triggered recursively.
		ConfigTriggerPreFunc PreFunc;   ///< Callback invoked before a config entry is modified; return false to reject the change
		ConfigTriggerPostFunc PostFunc; ///< Callback invoked after a config entry is modified; runs after the write-back and persistence, outside the lock
	};

	/**
	* List of config entry definitions
	*/
	typedef std::vector<ConfigItem> ConfigList;	

	bool IsWildcardKey(const std::string &key);
};

#endif


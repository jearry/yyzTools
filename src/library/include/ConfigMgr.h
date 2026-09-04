/*****************************************************************************
*  ConfigMgr - Configuration manager
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_CONFIG_MGR_H__
#define __XD_CONFIG_MGR_H__

#include <string>
#include <vector>
#include <shared_mutex>
#include "ConfigDef.h"
#include "RunLog.h"
#include "PtreeUtil.h"
#include "StringUtil.h"


namespace yyzlib
{
	class ConfigMgr
	{
	public:
		static ConfigMgr& Instance()
		{
			static ConfigMgr s_instance;
			return s_instance;
		}

		void Init(const ConfigList &def, const tstring & filepath, bool autosave=true);

		//By default every successful modification triggers a save, which costs
		//performance; for batch changes, turn autosave off first and call Save
		//manually afterwards
		template<typename T>
		bool Set(const std::string &key, const T& value)
		{
			bool ret = false;

			T old_value;
			Get(key, old_value);
			ConfigItem item;
			ConfigItem item_wildcard;

			if (old_value != value) {
				bool need_modify = true;

				//First look up the trigger by exact key

				if (FindItem(key, item)) {
					if (item.PreFunc) {
						ret = item.PreFunc(key, old_value, value);

						if (!ret) {
							need_modify = false;
						}
					} else {
						ret = true;
					}
				} else {
					ret = true;
				}

				//Then look up the trigger by wildcard
				if (need_modify) {
					if (FindItemWildcard(key, item_wildcard)) {
						if (item_wildcard.PreFunc) {
							ret = item_wildcard.PreFunc(key, old_value, value);

							if (!ret) {
								need_modify = false;
							}
						} else {
							ret = true;
						}
					}
				}

				if (need_modify) {
					{
						std::unique_lock<std::shared_mutex> lock(m_Mutex);

						SetValue(m_Pt, key, value);
					}

					//Save takes a shared_lock again via GetPropertyTree, so it
					//must be called outside the unique_lock scope (a
					//shared_mutex is not reentrant)
					if (m_AutoSave) {
						ret = Save();
					}else {
						ret = true;
					}

					if (item.PostFunc) {
						item.PostFunc();
					}
					if (item_wildcard.PostFunc) {
						item_wildcard.PostFunc();
					}
				}
			}else {
				ret = true;
			}

			return ret;
		}

		template<typename T>
	bool Get(const std::string &key, T& value)
	{
		std::shared_lock<std::shared_mutex> lock(m_Mutex);

		bool ret = false;

		ConfigItem item;
		T default_value = value;
		if (FindItem(key, item)) {
			//get_if guard: when the default value's type does not match T
			//(e.g. a bool key read via GetInt), keep the caller-provided value
			//instead of taking the variant default, avoiding bad_variant_access
			if (auto* p = std::get_if<T>(&item.DefaultValue)) {
				default_value = *p;
			}
		}

		ret = GetValue(m_Pt, key, value, default_value);

		return ret;
	}

		

		bool FindItem(const std::string &key, ConfigItem &item)
		{
			bool ret = false;
			for (auto it : m_Defines) {
				if (!IsWildcardKey(it.Key) && it.Key == key) {
					item = it;
					ret = true;
					break;
				}
			}

			return ret;
		}

		//Supports trailing wildcards, e.g. XdControl.Device.*
		bool FindItemWildcard(const std::string &key, ConfigItem &item)
		{
			bool ret = false;
			for (auto it : m_Defines) {
				if (IsWildcardKey(it.Key)) {
					std::string k = it.Key.substr(0, it.Key.size() - 1);
					if (StringUtil::starts_with(key, k)){
						item = it;
						ret = true;
						break;
					}
				}				
			}

			return ret;
		}

		
		bool ModifyConfig(const std::string &strs, bool replace);

		int GetInt(const std::string &key)
	{
		int v = 0;
		Get(key, v);
		return v;
	}

	bool GetBool(const std::string &key)
	{
		bool v = false;
		Get(key, v);
		return v;
	}
		float GetFloat(const std::string &key)
		{
			float v = 0.0f;
			Get(key, v);
			return v;
		}
		std::string GetString(const std::string &key)
		{
			std::string v;
			Get(key, v);
			return v;
		}
		StringList GetStringList(const std::string &key)
		{
			StringList v;
			Get(key, v);
			return v;
		}

		StringMap GetStruct(const std::string &key)
		{
			StringMap v;
			Get(key, v);
			return v;
		}

		StringMapList GetStructList(const std::string &key)
		{
			StringMapList v;
			Get(key, v);
			return v;
		}

		yyzlib::ptree GetChild(const std::string &key)
		{
			yyzlib::ptree v;
			Get(key, v);
			return v;
		}

		//Append to a StringList
		bool AddKeyValue(const std::string& key, const std::string& value);

		//Append to a StringMapList
		bool AddKeyValue(const std::string& key, const StringMap& value);

		//Apply the config entries in strs; entries missing from strs are deleted
		//Note: the callback fires only when both a default value and a callback
		//are defined, because the value is read according to the default value's type
		bool SetAllConfig(const std::string &strs)
		{
			return ModifyConfig(strs, true);
		}

		//Merge the config entries in strs (has a flaw: object arrays are merged too)
		//Note: the callback fires only when both a default value and a callback
		//are defined, because the value is read according to the default value's type
		bool MergeConfig(const std::string &strs)
		{
			return ModifyConfig(strs, false);
		}

		std::string GetAllConfig();

		
		bool ModifyPropertyTree(const yyzlib::ptree &pt, bool replace);

		yyzlib::ptree GetPropertyTree();

		void SetAutoSaveFlag(bool v)
		{
			m_AutoSave = v;
		}
		bool Save();
		
	private:
		ConfigMgr(void);
		~ConfigMgr(void);


		bool SaveToDb();
		void SaveToFile();

		void GetWildcardTrigger(const yyzlib::ptree &pt_old, const yyzlib::ptree &pt_new, std::map<std::string, ConfigTriggerPreFunc> &pre_triggers, std::map<std::string, ConfigTriggerPostFunc> &post_triggers, bool replace);

		std::shared_mutex m_Mutex;
		//Serializes AddKeyValue's read-modify-write sequence (Get -> modify ->
		//Set) so concurrent AddKeyValue calls on the same key cannot overwrite
		//each other and lose data
		std::mutex m_OpMutex;
		yyzlib::ptree m_Pt;

		ConfigList m_Defines;
		tstring m_FilePath;

		std::atomic<bool> m_AutoSave;

	};

	//Boolean switch config entries conventionally use int (0/1), leaving room
	//for future three-state extension; reading accepts the legacy string forms
	//"1"/"0"/"true"/"false" (bool type support remains in the variant, but the
	//application layer conventionally does not use it)

#define GET_CONFIG_BOOL(k) ConfigMgr::Instance().GetBool(k)
#define GET_CONFIG_INT(k) ConfigMgr::Instance().GetInt(k)
#define GET_CONFIG_FLOAT(k) ConfigMgr::Instance().GetFloat(k)
#define GET_CONFIG_STRING(k) ConfigMgr::Instance().GetString(k)
#define GET_CONFIG_STRING_LIST(k) ConfigMgr::Instance().GetStringList(k)
#define GET_CONFIG_STRUCT(k) ConfigMgr::Instance().GetStruct(k)
#define GET_CONFIG_STRUCT_LIST(k) ConfigMgr::Instance().GetStructList(k)
#define GET_CONFIG_CHILD(k) ConfigMgr::Instance().GetChild(k)

#define SET_CONFIG(k, v) ConfigMgr::Instance().Set(k, v)
#define ADD_CONFIG(k, v) ConfigMgr::Instance().AddKeyValue(k, v)

};

#endif


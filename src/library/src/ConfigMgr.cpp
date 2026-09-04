/*****************************************************************************
*  Configuration management
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"

#include "RunLog.h"
#include "ConfigMgr.h"
#include "yyzlib.h"


namespace yyzlib
{
	ConfigMgr::ConfigMgr(void)
	{
		m_AutoSave = true;
	}

	ConfigMgr::~ConfigMgr(void)
	{

	}

	// Wildcard keys end with '*' (e.g. "foo.*"); a single trigger then covers a group of sub keys
	bool IsWildcardKey(const std::string &key)
	{
		return !key.empty() && key.back() == '*';
	}

	void ConfigMgr::Init(const ConfigList &def, const tstring & filepath, bool autosave)
	{
		m_AutoSave = autosave;
		m_Defines = def;

		m_FilePath = filepath;

		m_Pt.clear();
		std::string value = LoadFileString(m_FilePath);
		if (!value.empty()) {
			if (!LoadFromString(m_Pt, value)) {
				ErrorMsg("xdconfig init error, %s", value.c_str());
			}
		}
	}	

	bool ConfigMgr::Save()
	{
		yyzlib::ptree pt = GetPropertyTree();

		bool ret = false;
		std::string s = SaveToString(pt);

		ret = SaveFileString(m_FilePath, s);

		return ret;
	}


	// Appends an entry to a StringList
	bool ConfigMgr::AddKeyValue(const std::string& key, const std::string& value)
	{
		std::lock_guard<std::mutex> op(m_OpMutex); // the read-modify-write sequence must be atomic, otherwise concurrent updates overwrite each other

		bool valid = false;
		for (const auto& item : m_Defines) {
			// Set the default value according to the declared type
			if (StringUtil::iequals(item.Key, key)) {
				if (std::holds_alternative<StringList>(item.DefaultValue)) {
					valid = true;
				}
				break;
			}
		}

		// invalid key
		if (!valid) {
			return false;
		}

		StringList v;
		Get(key, v);

		// duplicate data
		StringList::iterator iter = std::find(v.begin(), v.end(), value);
		if (iter != v.end()) {
			return false;
		}

		v.push_back(value);

		bool ret = Set(key, v);

		return ret;
	}

	// Appends an entry to a StringMapList
	bool ConfigMgr::AddKeyValue(const std::string& key, const StringMap& value)
	{
		std::lock_guard<std::mutex> op(m_OpMutex); // same as above: make the read-modify-write sequence atomic

		bool valid = false;
		for (const auto& item : m_Defines) {
			// Set the default value according to the declared type
			if (StringUtil::iequals(item.Key, key)) {
				if (std::holds_alternative<StringMapList>(item.DefaultValue)) {
					valid = true;
				}
				break;
			}
		}

		// invalid key
		if (!valid) {
			return false;
		}

		StringMapList v;
		Get(key, v);

		// duplicate data
		StringMapList::iterator iter = std::find(v.begin(), v.end(), value);
		if (iter != v.end()) {
			return false;
		}

		v.push_back(value);

		bool ret = Set(key, v);

		return ret;
	}

	std::string ConfigMgr::GetAllConfig()
	{
		yyzlib::ptree pt = GetPropertyTree();

		std::string s = SaveToString(pt);

		return s;
	}

	// Collects the triggers of the wildcard keys whose subtree changed
	void ConfigMgr::GetWildcardTrigger(const yyzlib::ptree &pt_old, const yyzlib::ptree &pt_new,
		std::map<std::string, ConfigTriggerPreFunc> &pre_triggers,
		std::map<std::string, ConfigTriggerPostFunc> &post_triggers,
		bool replace)
	{
		yyzlib::ptree default_value;
		for (auto item : m_Defines) {
			if (IsWildcardKey(item.Key)) {
				std::string k = item.Key.substr(0, item.Key.size() - 1);

				while (StringUtil::ends_with(k, ".")) {
					k = k.substr(0, k.size() - 1);
				}

				// Same reasoning as for exact keys: during an incremental merge a subtree missing from the new tree is not going to be touched,
				// and without this check we would compare a default empty tree against the old value and fire the trigger for nothing
				if (!replace && !pt_new.get_child_optional(k)) {
					continue;
				}

				yyzlib::ptree ov = pt_old.get_child(k, default_value);
				yyzlib::ptree nv = pt_new.get_child(k, default_value);

				if (ov != nv) {
					if (item.PreFunc) {
						pre_triggers[item.Key] = item.PreFunc;
					}
					if (item.PostFunc) {
						post_triggers[item.Key] = item.PostFunc;
					}
				}
			}
		}
	}

	bool ConfigMgr::ModifyPropertyTree(const yyzlib::ptree &pt, bool replace)
	{
		bool ret = false;

		// Optimistic concurrency: change detection and the PreFunc callbacks run against a lock-free snapshot of m_Pt, while the write-back
		// validates under the write lock that the snapshot has not been modified concurrently (CAS) and retries with a fresh snapshot when it has.
		// A PreFunc body may therefore call locking APIs such as Get. Note that a retry re-runs PreFunc, so callbacks must tolerate repeated invocation.
		for (int attempt = 0; attempt < 3; attempt++) {
			yyzlib::ptree snapshot;
			{
				// GetPropertyTree() must not be used here: it injects the missing defaults into the returned copy,
				// which makes the CAS check (m_Pt != snapshot) fail forever. What we need is a raw copy of m_Pt;
				// GetValue already falls back to the default value, so a raw snapshot does not change the detection semantics.
				std::shared_lock<std::shared_mutex> lock(m_Mutex);
				snapshot = m_Pt;
			}

			std::map<std::string, ConfigTriggerPostFunc> post_trigger;
			std::map<std::string, ConfigTriggerPostFunc> wildcard_post_trigger;
			bool trigger_ret = true;

			for (auto item : m_Defines) {
				if (!item.DefaultValue.valueless_by_exception() && !IsWildcardKey(item.Key)) {
					// During an incremental merge a key that pt does not carry is not going to be modified at all and must not fire its callback.
					// GetValue returns the default for any missing key, so without the existence check "this key was not supplied"
					// is mistaken for "this key was changed to its default": a narrow setConfig would then spuriously fire the callbacks
					// of every other key, and if a callback reloads the page while the page writes configuration on startup
					// that turns into an infinite loop. Under replace mode a missing key really does mean "fall back to the default",
					// so the key is skipped only in incremental mode.
					if (!replace && !pt.get_child_optional(item.Key)) {
						continue;
					}

					//bool, int, float, std::string, ConfigListValue, yyzlib::ptree

				if (std::holds_alternative<bool>(item.DefaultValue)) {
					bool ov, nv;
					GetValue(snapshot, item.Key, ov, std::get<bool>(item.DefaultValue));

					GetValue(pt, item.Key, nv, std::get<bool>(item.DefaultValue));
					if (ov != nv) {
						if (item.PreFunc) {
							trigger_ret &= item.PreFunc(item.Key, ov, nv);
						}

						if (item.PostFunc) {
							post_trigger[item.Key] = item.PostFunc;
						}
					}
				} else if (std::holds_alternative<int>(item.DefaultValue)) {
						int ov, nv;
						GetValue(snapshot, item.Key, ov, std::get<int>(item.DefaultValue));

						GetValue(pt, item.Key, nv, std::get<int>(item.DefaultValue));
						if (ov != nv) {
							if (item.PreFunc) {
								trigger_ret &= item.PreFunc(item.Key, ov, nv);
							}
							
							if (item.PostFunc) {
								post_trigger[item.Key] = item.PostFunc;
							}							
						}
					} else if (std::holds_alternative<float>(item.DefaultValue)) {
						float ov, nv;
						GetValue(snapshot, item.Key, ov, std::get<float>(item.DefaultValue));

						GetValue(pt, item.Key, nv, std::get<float>(item.DefaultValue));
						if (ov != nv) {
							if (item.PreFunc) {
								trigger_ret &= item.PreFunc(item.Key, ov, nv);
							}

							if (item.PostFunc) {
								post_trigger[item.Key] = item.PostFunc;
							}
						}
					} else if (std::holds_alternative<std::string>(item.DefaultValue)) {
						std::string ov, nv;
						GetValue(snapshot, item.Key, ov, std::get<std::string>(item.DefaultValue));

						GetValue(pt, item.Key, nv, std::get<std::string>(item.DefaultValue));
						if (ov != nv) {
							if (item.PreFunc) {
								trigger_ret &= item.PreFunc(item.Key, ov, nv);
							}

							if (item.PostFunc) {
								post_trigger[item.Key] = item.PostFunc;
							}
						}
					} else if (std::holds_alternative<StringList>(item.DefaultValue)) {
						StringList ov, nv;
						GetValue(snapshot, item.Key, ov, std::get<StringList>(item.DefaultValue));

						GetValue(pt, item.Key, nv, std::get<StringList>(item.DefaultValue));
						if (ov != nv) {
							if (item.PreFunc) {
								trigger_ret &= item.PreFunc(item.Key, ov, nv);
							}

							if (item.PostFunc) {
								post_trigger[item.Key] = item.PostFunc;
							}
						}
					} else if (std::holds_alternative<StringMap>(item.DefaultValue)) {
						StringMap ov, nv;
						GetValue(snapshot, item.Key, ov, std::get<StringMap>(item.DefaultValue));

						GetValue(pt, item.Key, nv, std::get<StringMap>(item.DefaultValue));
						if (ov != nv) {
							if (item.PreFunc) {
								trigger_ret &= item.PreFunc(item.Key, ov, nv);
							}

							if (item.PostFunc) {
								post_trigger[item.Key] = item.PostFunc;
							}
						}
					} else if (std::holds_alternative<StringMapList>(item.DefaultValue)) {
						StringMapList ov, nv;
						GetValue(snapshot, item.Key, ov, std::get<StringMapList>(item.DefaultValue));

						GetValue(pt, item.Key, nv, std::get<StringMapList>(item.DefaultValue));
						if (ov != nv) {
							if (item.PreFunc) {
								trigger_ret &= item.PreFunc(item.Key, ov, nv);
							}

							if (item.PostFunc) {
								post_trigger[item.Key] = item.PostFunc;
							}
						}
					} else if (std::holds_alternative<yyzlib::ptree>(item.DefaultValue)) {
						yyzlib::ptree ov, nv;
						GetValue(snapshot, item.Key, ov, std::get<yyzlib::ptree>(item.DefaultValue));

						GetValue(pt, item.Key, nv, std::get<yyzlib::ptree>(item.DefaultValue));
						if (ov != nv) {
							if (item.PreFunc) {
								trigger_ret &= item.PreFunc(item.Key, ov, nv);
							}

							if (item.PostFunc) {
								post_trigger[item.Key] = item.PostFunc;
							}
						}
					}
				}
			}

			// Handle the wildcard triggers

			std::map<std::string, ConfigTriggerPreFunc> wildcard_pre_trigger;
			GetWildcardTrigger(snapshot, pt, wildcard_pre_trigger, wildcard_post_trigger, replace);

			for (auto i : wildcard_pre_trigger) {
				trigger_ret &= i.second(i.first, "", ""); // wildcard triggers receive empty values
			}

			if (!trigger_ret) {
				ErrorMsg("SetAllConfig trigger run error");
				return false;
			}

			// Validate under the write lock that the snapshot was not modified concurrently; if it was, discard this round and retry with a fresh snapshot
			{
				std::unique_lock<std::shared_mutex> lock(m_Mutex);

				if (m_Pt != snapshot) {
					continue;
				}

				if (replace) {
					m_Pt = pt;
				} else {
					Merge(m_Pt, pt);
				}
			}

			if (m_AutoSave) {
				ret = Save();
			} else {
				ret = true;
			}

			// post triggers (run outside the lock, callbacks may read or write configuration)
			for (auto i : post_trigger) {
				i.second();
			}
			for (auto i : wildcard_post_trigger) {
				i.second();
			}

			return ret;
		}

		ErrorMsg("ModifyPropertyTree: snapshot contention, retried too many times");
		return false;
	}

	bool ConfigMgr::ModifyConfig(const std::string &strs, bool replace)
	{
		yyzlib::ptree pt;
		if (!LoadFromString(pt, strs)) {
			ErrorMsg("SetAllConfig, Load error");
			return false;
		}

		return ModifyPropertyTree(pt, replace);
	}

	yyzlib::ptree ConfigMgr::GetPropertyTree()
	{
		yyzlib::ptree result_pt;

		{
			std::shared_lock<std::shared_mutex> lock(m_Mutex);

			result_pt = m_Pt;
		}
		

		for (const auto& item : m_Defines) {
			if (IsWildcardKey(item.Key)) {
				continue;
			}

			if (!item.DefaultValue.valueless_by_exception()) {
				// Check whether the configuration entry exists and whether it is empty
				bool need_set_default = false;

				try {
					// Try to read the value at this path
					auto child = result_pt.get_child_optional(item.Key);
					if (!child) {
						// the path does not exist
						need_set_default = true;
					} else {
						// Check whether it is empty (for simple values)
						if (child->empty()) {
							// It may be a leaf node, so inspect its data
							std::optional<std::string> value = child->get_value_optional<std::string>();
							if (!value || value->empty()) {
								need_set_default = true;
							}
						}
						// For complex types (arrays, objects) a child count of 0 also counts as empty
						else if (child->size() == 0) {
							need_set_default = true;
						}
					}
				} catch (...) {
					// An exception while reading is treated as "the default value is required"
					need_set_default = true;
				}

				if (need_set_default) {
				// Set the default value according to the declared type
				if (std::holds_alternative<bool>(item.DefaultValue)) {
					SetValue(result_pt, item.Key, std::get<bool>(item.DefaultValue));
				} else if (std::holds_alternative<int>(item.DefaultValue)) {
						SetValue(result_pt, item.Key, std::get<int>(item.DefaultValue));
					} else if (std::holds_alternative<float>(item.DefaultValue)) {
						SetValue(result_pt, item.Key, std::get<float>(item.DefaultValue));
					} else if (std::holds_alternative<std::string>(item.DefaultValue)) {
						SetValue(result_pt, item.Key, std::get<std::string>(item.DefaultValue));
					} else if (std::holds_alternative<StringList>(item.DefaultValue)) {
						SetValue(result_pt, item.Key, std::get<StringList>(item.DefaultValue));
					} else if (std::holds_alternative<StringMap>(item.DefaultValue)) {
						SetValue(result_pt, item.Key, std::get<StringMap>(item.DefaultValue));
					} else if (std::holds_alternative<StringMapList>(item.DefaultValue)) {
						SetValue(result_pt, item.Key, std::get<StringMapList>(item.DefaultValue));
					} else if (std::holds_alternative<yyzlib::ptree>(item.DefaultValue)) {
						SetValue(result_pt, item.Key, std::get<yyzlib::ptree>(item.DefaultValue));
					}
				}
			}
		}

		return result_pt;
	}
}



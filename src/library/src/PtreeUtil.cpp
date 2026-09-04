/*****************************************************************************
*  Property tree helpers
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"

#include "PtreeUtil.h"
#include "TypeDefs.h"

namespace yyzlib
{

	std::string SaveToString(yyzlib::ptree &pt, bool pretty)
	{
		return pt.dump_json(pretty);
	}

	bool LoadFromString(yyzlib::ptree &pt, const std::string &s)
	{
		return yyzlib::ptree::load_json(pt, s);
	}

	bool HasValue(yyzlib::ptree &pt, const std::string &key)
	{
		yyzlib::ptree::assoc_iterator iter = pt.find(key);
		return iter != pt.not_found();
	}

	bool SetValue(yyzlib::ptree &pt, const std::string &key, bool value)
	{
		pt.put(key, value);
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, bool &value, bool default_value)
	{
		value = pt.get(key, default_value);
		return true;
	}

	bool SetValue(yyzlib::ptree &pt, const std::string &key, int value)
	{
		pt.put(key, value);
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, int &value, int default_value)
	{
		value = pt.get(key, default_value);
		return true;
	}

	bool SetValue(yyzlib::ptree &pt, const std::string &key, uint32_t value)
	{
		pt.put(key, value);
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, uint32_t &value, uint32_t default_value)
	{
		value = pt.get(key, default_value);
		return true;
	}

	bool SetValue(yyzlib::ptree& pt, const std::string& key, uint64_t value)
	{
		pt.put(key, value);
		return true;
	}

	bool GetValue(const yyzlib::ptree& pt, const std::string& key, uint64_t& value, uint64_t default_value)
	{
		value = pt.get(key, default_value);
		return true;
	}

	bool SetValue(yyzlib::ptree &pt, const std::string &key, float value)
	{
		pt.put(key, value);
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, float &value, float default_value)
	{
		value = pt.get(key, default_value);
		return true;
	}

	bool SetValue(yyzlib::ptree &pt, const std::string &key, const char *value)
	{
		return SetValue(pt, key, std::string(value));
	}

	bool SetValue(yyzlib::ptree &pt, const std::string &key, const std::string &value)
	{
		pt.put(key, value);
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, std::string &value, const std::string &default_value)
	{
		value = pt.get(key, default_value);
		return true;
	}

	// String list
	bool SetValue(yyzlib::ptree &pt, const std::string &key, const StringList& strs)
	{
		yyzlib::ptree child;
		for (auto item : strs) {
			yyzlib::ptree pitem;
			pitem.put("", item);

			child.push_back(std::make_pair("", pitem));
		}

		if (key.empty()) {
			pt = child;
		}else {
			pt.put_child(key, child);
		}
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, StringList &strs, const StringList& default_strs)
	{
		StringList get_values;
		yyzlib::ptree default_value;
		yyzlib::ptree ptrange;
		if (key.empty()) {
			ptrange = pt;
		}else {
			ptrange = pt.get_child(key, default_value);
		}

		for (auto &item : ptrange) {
			if (item.second.size() == 0) {
				std::string v = item.second.get<std::string>("");
				get_values.push_back(v);
			}
		}
		if (get_values.empty()) {
			strs = default_strs;
		} else {
			strs = get_values;
		}
		return true;
	}

	bool SetValue(yyzlib::ptree &pt, const std::string &key, const StringMap& stru)
	{
		yyzlib::ptree child;
		for (auto item : stru) {
			child.put(item.first, item.second);
		}

		if (key.empty()) {
			pt = child;
		}else {
			pt.put_child(key, child);
		}
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, StringMap &stru, const StringMap& default_stru)
	{
		StringMap get_values;
		yyzlib::ptree default_value;
		yyzlib::ptree ptrange;
		if (key.empty()) {
			ptrange = pt;
		} else {
			ptrange = pt.get_child(key, default_value);
		}
		for (auto &item : ptrange) {
			if (item.second.size() == 0) {
				get_values[item.first] = item.second.data();
			}
		}
		if (get_values.empty()) {
			stru = default_stru;
		} else {
			stru = get_values;
		}
		return true;
	}

	// List of custom structures
	bool SetValue(yyzlib::ptree &pt, const std::string &key, const StringMapList& strus)
	{
		yyzlib::ptree child;
		for (auto stru : strus) {
			yyzlib::ptree pitem;
			for (auto item : stru) {
				pitem.put(item.first, item.second);
			}

			child.push_back(std::make_pair("", pitem));
		}
		if (key.empty()) {
			pt = child;
		} else {
			pt.put_child(key, child);
		}
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, StringMapList &strus, const StringMapList& default_strus)
	{
		StringMapList get_values;
		yyzlib::ptree default_value;
		yyzlib::ptree ptrange;
		if (key.empty()) {
			ptrange = pt;
		} else {
			ptrange = pt.get_child(key, default_value);
		}
		for (auto &items : ptrange) {
			yyzlib::ptree pstru = items.second.get_child("", default_value);

			StringMap stru;
			for (auto &item : pstru) {
				if (item.second.size() == 0) {
					stru[item.first] = item.second.data();
				}
			}
			get_values.push_back(stru);
		}
		if (get_values.empty()) {
			strus = default_strus;
		} else {
			strus = get_values;
		}
		return true;
	}

	//
	bool SetValue(yyzlib::ptree &pt, const std::string &key, const yyzlib::ptree &value)
	{
		if (key.empty()) {
			pt = value;
		} else {
			pt.put_child(key, value);
		}
		return true;
	}

	bool GetValue(const yyzlib::ptree &pt, const std::string &key, yyzlib::ptree &value, const yyzlib::ptree& default_value)
	{
		if (key.empty()) {
			value = pt;
		} else {
			value = pt.get_child(key, default_value);
		}
		return true;
	}


	// Recursively merges two property trees
	void MergeTree(yyzlib::ptree& target, const yyzlib::ptree& source)
	{
		// Copy the data when the source node has any, along with its type
		if (!source.data().empty() || source.type() == yyzlib::NodeType::Null) {
			target.set_data(source.data(), source.type());
		}

		// Recursively merge every child node
		for (const auto& source_child : source) {
			const std::string& key = source_child.first;
			const yyzlib::ptree& source_value = source_child.second;

			// Check whether the target already holds a node with the same key
			auto target_range = target.equal_range(key);

			if (target_range.first == target_range.second) {
				// The key is missing in the target, add it directly
				target.push_back(std::make_pair(key, source_value));
			} else {
				// The key already exists in the target, so decide whether to replace or to merge

				// Special case: when the source node is an array (several children sharing the same empty key), replace the whole subtree
				// so that the array is not corrupted
				bool source_is_array = false;
				for (const auto& child : source_value) {
					if (child.first.empty()) {
						source_is_array = true;
						break;
					}
				}

				// When the source node is an array or a leaf node, replace the target node
				if (source_is_array || source_value.empty()) {
					// Remove every node carrying the same key
					target.erase(key);
					// Add the source node
					target.push_back(std::make_pair(key, source_value));
				} else {
					// Otherwise merge recursively
					// Find the first node with the same key and merge into it recursively
					bool merged = false;
					for (auto it = target_range.first; it != target_range.second && !merged; ++it) {
						// When both the source and the target node are non-leaf nodes, merge recursively
						if (!source_value.empty() && !it->second.empty()) {
							MergeTree(it->second, source_value);
							merged = true;
						}
					}

					// No recursive merge happened, so add it as a new node
					if (!merged) {
						target.push_back(std::make_pair(key, source_value));
					}
				}
			}
		}
	}

	// Simple Merge helper
	void Merge(yyzlib::ptree& object, const yyzlib::ptree& input)
	{
		MergeTree(object, input);
	}

}

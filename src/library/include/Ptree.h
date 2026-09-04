/*****************************************************************************
*  Ptree - Property tree (drop-in compatibility layer replacing
*          boost::property_tree::ptree)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*  Storage model: a node = string data + an ordered child list
*  (duplicate keys allowed). Scalar main data is still kept as text in
*  m_data, plus a type tag m_type; serialization decides whether to
*  quote by type (numbers/booleans are emitted as native JSON values).
*  Reads go through text conversion only (FromString), so reading stays
*  compatible with the old all-string format.
*  JSON parsing / string escaping reuse nlohmann/json.
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_PTREE_H__
#define __XD_PTREE_H__

#include <string>
#include <vector>
#include <utility>
#include <optional>
#include <stdexcept>
#include <sstream>
#include <type_traits>

#include <nlohmann/json.hpp>

namespace yyzlib
{
	//Scalar type tag: decides the serialization output form (String is
	//quoted; the others are emitted as native JSON values)
	enum class NodeType
	{
		String,		//text (emitted quoted)
		Boolean,	//boolean
		Integer,	//integer
		Float,		//floating point
		Null		//null value (emitted as null)
	};

	class ptree
	{
	public:
		using value_type = std::pair<std::string, ptree>;
		using iterator = std::vector<value_type>::iterator;
		using const_iterator = std::vector<value_type>::const_iterator;
		using assoc_iterator = iterator;

		ptree() = default;
		ptree(const ptree&) = default;
		ptree(ptree&&) = default;
		ptree& operator=(const ptree&) = default;
		ptree& operator=(ptree&&) = default;

		//------ Data ------
		const std::string& data() const { return m_data; }
		NodeType type() const { return m_type; }

		//Set data and type together (a writable data() was removed to
		//avoid a bypass that would lose the type)
		void set_data(const std::string& data, NodeType type = NodeType::String)
		{
			m_data = data;
			m_type = type;
		}

		//------ Children ------
		iterator begin() { return m_children.begin(); }
		iterator end() { return m_children.end(); }
		const_iterator begin() const { return m_children.begin(); }
		const_iterator end() const { return m_children.end(); }

		size_t size() const { return m_children.size(); }
		bool empty() const { return m_children.empty(); }

		void clear() { m_data.clear(); m_type = NodeType::String; m_children.clear(); }

		friend bool operator==(const ptree& a, const ptree& b)
		{
			if (a.m_data != b.m_data || a.m_children.size() != b.m_children.size()) return false;
			for (size_t i = 0; i < a.m_children.size(); ++i) {
				if (a.m_children[i].first != b.m_children[i].first) return false;
				if (!(a.m_children[i].second == b.m_children[i].second)) return false;
			}
			return true;
		}

		friend bool operator!=(const ptree& a, const ptree& b) { return !(a == b); }

		void push_back(const value_type& v) { m_children.push_back(v); }

		//Remove all same-named children by path ("a.b.c" dotted path supported)
		bool erase(const std::string& key)
		{
			std::vector<std::string> segs;
			SplitPath(key, segs);
			ptree* parent = WalkToParent(segs);
			if (!parent) return false;
			const std::string& last = segs.back();
			size_t before = parent->m_children.size();
			parent->m_children.erase(std::remove_if(parent->m_children.begin(), parent->m_children.end(),
				[&last](const value_type& v) { return v.first == last; }), parent->m_children.end());
			return parent->m_children.size() != before;
		}

		//Returns not_found() on lookup failure (equivalent to end(),
		//boost-compatible spelling)
		//Note: the returned iterator always belongs to this node's
		//m_children (on a deep hit it points at the first-segment node),
		//otherwise comparing against not_found() would assert in the debug
		//CRT because the iterators would span different vectors
		iterator find(const std::string& key)
		{
			std::vector<std::string> segs;
			SplitPath(key, segs);
			ptree* parent = WalkToParent(segs);
			if (!parent) return end();
			const std::string& last = segs.back();
			bool found = false;
			for (size_t i = 0; i < parent->m_children.size(); ++i) {
				if (parent->m_children[i].first == last) { found = true; break; }
			}
			if (!found) return end();
			if (parent == this) {
				for (size_t i = 0; i < m_children.size(); ++i) {
					if (m_children[i].first == last) return m_children.begin() + i;
				}
			}
			//Deep hit: return the iterator of the first-segment node under
			//this (the caller only uses it for an existence check)
			for (size_t i = 0; i < m_children.size(); ++i) {
				if (m_children[i].first == segs.front()) return m_children.begin() + i;
			}
			return end();
		}

		iterator not_found() { return end(); }
		const_iterator not_found() const { return end(); }

		//const version (supports the find(...) != not_found() spelling on a
		//const ptree); iterator ownership as above
		const_iterator find(const std::string& key) const
		{
			std::vector<std::string> segs;
			SplitPath(key, segs);
			const ptree* parent = WalkToParent(segs);
			if (!parent) return end();
			const std::string& last = segs.back();
			bool found = false;
			for (size_t i = 0; i < parent->m_children.size(); ++i) {
				if (parent->m_children[i].first == last) { found = true; break; }
			}
			if (!found) return end();
			if (parent == this) {
				for (size_t i = 0; i < m_children.size(); ++i) {
					if (m_children[i].first == last) return m_children.begin() + i;
				}
			}
			for (size_t i = 0; i < m_children.size(); ++i) {
				if (m_children[i].first == segs.front()) return m_children.begin() + i;
			}
			return end();
		}

		//Contiguous range of same-named children (used by the Merge logic)
		std::pair<iterator, iterator> equal_range(const std::string& key)
		{
			iterator first = find(key);
			iterator second = first;
			while (second != end() && second->first == key) ++second;
			return { first, second };
		}

		//------ Value access ------
		template<typename T>
		T get_value() const
		{
			return FromString<T>(m_data);
		}

		template<typename T>
		std::optional<T> get_value_optional() const
		{
			try {
				return FromString<T>(m_data);
			}
			catch (...) {
				return std::nullopt;
			}
		}

		template<typename T>
		T get(const std::string& path) const
		{
			const ptree* node = Walk(path);
			if (!node) throw std::runtime_error("ptree: no such path: " + path);
			return FromString<T>(node->m_data);
		}

		template<typename T>
		T get(const std::string& path, const T& default_value) const
		{
			const ptree* node = Walk(path);
			if (!node) return default_value;
			try {
				return FromString<T>(node->m_data);
			}
			catch (...) {
				return default_value;
			}
		}

		//const char* defaults are handled as std::string (matches boost behavior)
		std::string get(const std::string& path, const char* default_value) const
		{
			return get<std::string>(path, std::string(default_value));
		}

		template<typename T>
		std::optional<T> get_optional(const std::string& path) const
		{
			const ptree* node = Walk(path);
			if (!node) return std::nullopt;
			try {
				return FromString<T>(node->m_data);
			}
			catch (...) {
				return std::nullopt;
			}
		}

		//------ Write (overwrite data and keep children if present;
		//create missing levels along the way) ------
		template<typename T>
		void put(const std::string& key, const T& value)
		{
			std::vector<std::string> segs;
			SplitPath(key, segs);
			ptree* node = this;
			for (const auto& seg : segs) {
				node = &node->Obtain(seg);
			}
			node->m_data = ToString(value);
			node->m_type = TypeOf<T>();
		}

		template<typename T>
		void put_value(const T& value)
		{
			m_data = ToString(value);
			m_type = TypeOf<T>();
		}

		//------ Subtrees ------
		ptree& get_child(const std::string& path)
		{
			ptree* node = const_cast<ptree*>(Walk(path));
			if (!node) throw std::runtime_error("ptree: no such child: " + path);
			return *node;
		}

		const ptree& get_child(const std::string& path) const
		{
			const ptree* node = Walk(path);
			if (!node) throw std::runtime_error("ptree: no such child: " + path);
			return *node;
		}

		ptree& get_child(const std::string& path, ptree& default_value)
		{
			ptree* node = const_cast<ptree*>(Walk(path));
			return node ? *node : default_value;
		}

		const ptree& get_child(const std::string& path, const ptree& default_value) const
		{
			const ptree* node = Walk(path);
			return node ? *node : default_value;
		}

		std::optional<ptree> get_child_optional(const std::string& path) const
		{
			const ptree* node = Walk(path);
			if (!node) return std::nullopt;
			return *node;
		}

		//Replace an entire subtree (replaces the first same-named child if any)
		void put_child(const std::string& path, const ptree& pt)
		{
			std::vector<std::string> segs;
			SplitPath(path, segs);
			ptree* parent = CreateToParent(segs);
			const std::string& last = segs.back();
			for (auto& v : parent->m_children) {
				if (v.first == last) {
					v.second = pt;
					return;
				}
			}
			parent->m_children.push_back({ last, pt });
		}

		//Append (does not replace an existing same-named child)
		void add_child(const std::string& path, const ptree& pt)
		{
			std::vector<std::string> segs;
			SplitPath(path, segs);
			ptree* parent = CreateToParent(segs);
			parent->m_children.push_back({ segs.back(), pt });
		}

		//------ JSON serialization ------
		std::string dump_json(bool pretty = true) const
		{
			std::string out;
			WriteNode(out, 0, pretty);
			out += "\n";
			return out;
		}

		//Returns false on parse failure; the contents of out are unspecified
		static bool load_json(ptree& out, const std::string& text)
		{
			nlohmann::ordered_json j = nlohmann::ordered_json::parse(text, nullptr, false);
			if (j.is_discarded()) return false;
			FromJson(j, out);
			return true;
		}

	private:
		std::string m_data;
		NodeType m_type = NodeType::String;
		std::vector<value_type> m_children;

		static void SplitPath(const std::string& key, std::vector<std::string>& segs)
		{
			segs.clear();
			if (key.empty()) return;	//empty path means this node itself
			size_t pos = 0;
			while (true) {
				size_t hit = key.find('.', pos);
				if (hit == std::string::npos) {
					segs.push_back(key.substr(pos));
					return;
				}
				segs.push_back(key.substr(pos, hit - pos));
				pos = hit + 1;
			}
		}

		//Walk to the parent node of the path; the parent of an empty path
		//is this node itself (returns nullptr when segs is empty, meaning
		//the path is invalid)
		ptree* WalkToParent(const std::vector<std::string>& segs)
		{
			if (segs.empty()) return nullptr;
			ptree* cur = this;
			for (size_t i = 0; i + 1 < segs.size(); ++i) {
				cur = cur->Lookup(segs[i]);
				if (!cur) return nullptr;
			}
			return cur;
		}

		const ptree* WalkToParent(const std::vector<std::string>& segs) const
		{
			if (segs.empty()) return nullptr;
			const ptree* cur = this;
			for (size_t i = 0; i + 1 < segs.size(); ++i) {
				cur = cur->Lookup(segs[i]);
				if (!cur) return nullptr;
			}
			return cur;
		}

		ptree* CreateToParent(const std::vector<std::string>& segs)
		{
			if (segs.empty()) return nullptr;
			ptree* cur = this;
			for (size_t i = 0; i + 1 < segs.size(); ++i) {
				cur = &cur->Obtain(segs[i]);
			}
			return cur;
		}

		ptree* Lookup(const std::string& key)
		{
			for (auto& v : m_children) {
				if (v.first == key) return &v.second;
			}
			return nullptr;
		}

		const ptree* Lookup(const std::string& key) const
		{
			for (const auto& v : m_children) {
				if (v.first == key) return &v.second;
			}
			return nullptr;
		}

		ptree& Obtain(const std::string& key)
		{
			ptree* found = Lookup(key);
			if (found) return *found;
			m_children.push_back({ key, ptree() });
			return m_children.back().second;
		}

		//Find a node by dotted path; an empty path returns this node itself
		const ptree* Walk(const std::string& path) const
		{
			if (path.empty()) return this;
			std::vector<std::string> segs;
			SplitPath(path, segs);
			const ptree* cur = this;
			for (const auto& seg : segs) {
				cur = cur->Lookup(seg);
				if (!cur) return nullptr;
			}
			return cur;
		}

		template<typename T>
		static T FromString(const std::string& s)
		{
			if constexpr (std::is_same_v<T, std::string>) {
				return s;
			}
			else if constexpr (std::is_same_v<T, bool>) {
				return s == "true" || s == "1";
			}
			else if constexpr (std::is_integral_v<T>) {
				return static_cast<T>(std::stoll(s));
			}
			else if constexpr (std::is_floating_point_v<T>) {
				return static_cast<T>(std::stod(s));
			}
			else {
				static_assert(!sizeof(T*), "ptree: unsupported value type");
			}
		}

		template<typename T>
		static std::string ToString(const T& value)
		{
			if constexpr (std::is_same_v<T, std::string>) {
				return value;
			}
			else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>) {
				return std::string(value);
			}
			else if constexpr (std::is_same_v<T, bool>) {
				return value ? "true" : "false";
			}
			else if constexpr (std::is_integral_v<T>) {
				return std::to_string(value);
			}
			else if constexpr (std::is_floating_point_v<T>) {
				std::ostringstream os;
				os << value;
				return os.str();
			}
			else {
				static_assert(!sizeof(T*), "ptree: unsupported value type");
			}
		}

		//Type deduction for written values (the bool check must come before
		//integral: bool is itself an integral type)
		template<typename T>
		static NodeType TypeOf()
		{
			if constexpr (std::is_same_v<T, bool>) {
				return NodeType::Boolean;
			}
			else if constexpr (std::is_integral_v<T>) {
				return NodeType::Integer;
			}
			else if constexpr (std::is_floating_point_v<T>) {
				return NodeType::Float;
			}
			else {
				return NodeType::String;
			}
		}

		static void FromJson(const nlohmann::ordered_json& j, ptree& node)
		{
			switch (j.type()) {
			case nlohmann::ordered_json::value_t::null:
				node.m_data.clear();
				node.m_type = NodeType::Null;
				break;
			case nlohmann::ordered_json::value_t::string:
				node.m_data = j.get_ref<const std::string&>();
				node.m_type = NodeType::String;
				break;
			case nlohmann::ordered_json::value_t::boolean:
				node.m_data = j.dump();
				node.m_type = NodeType::Boolean;
				break;
			case nlohmann::ordered_json::value_t::number_integer:
			case nlohmann::ordered_json::value_t::number_unsigned:
				node.m_data = j.dump();	//numbers stay as text; values convert via FromString
				node.m_type = NodeType::Integer;
				break;
			case nlohmann::ordered_json::value_t::number_float:
				node.m_data = j.dump();
				node.m_type = NodeType::Float;
				break;
			case nlohmann::ordered_json::value_t::object:
				for (auto it = j.begin(); it != j.end(); ++it) {
					node.m_children.push_back({ it.key(), ptree() });
					FromJson(it.value(), node.m_children.back().second);
				}
				break;
			case nlohmann::ordered_json::value_t::array:
				for (const auto& item : j) {
					node.m_children.push_back({ std::string(), ptree() });
					FromJson(item, node.m_children.back().second);
				}
				break;
			default:
				break;
			}
		}

		static std::string Quote(const std::string& s)
		{
			return nlohmann::json(s).dump();
		}

		void WriteNode(std::string& out, int level, bool pretty) const
		{
			auto newline = [&](int lv) {
				if (pretty) {
					out += '\n';
					out.append(static_cast<size_t>(lv) * 4, ' ');
				}
			};

			if (m_children.empty()) {
				switch (m_type) {
				case NodeType::Boolean:
				case NodeType::Integer:
				case NodeType::Float:
					//Guard: fall back to string output when the data is empty
					//(abnormal bypass), keeping the JSON valid
					out += m_data.empty() ? Quote(m_data) : m_data;
					break;
				case NodeType::Null:
					out += "null";
					break;
				case NodeType::String:
				default:
					out += Quote(m_data);
					break;
				}
				return;
			}

			//All child keys empty -> array
			bool is_array = true;
			for (const auto& v : m_children) {
				if (!v.first.empty()) { is_array = false; break; }
			}

			out += is_array ? '[' : '{';
			bool first = true;
			for (const auto& v : m_children) {
				if (!first) out += ',';
				first = false;
				newline(level + 1);
				if (!is_array) {
					out += Quote(v.first);
					out += pretty ? ": " : ":";
				}
				v.second.WriteNode(out, level + 1, pretty);
			}
			newline(level);
			out += is_array ? ']' : '}';
		}
	};
}

#endif

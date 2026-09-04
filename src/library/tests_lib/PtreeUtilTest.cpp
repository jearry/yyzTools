/*****************************************************************************
*  yyzlib Ptree (PtreeUtil) - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include "yyzlib.h"
#include <gtest/gtest.h>
#include "PtreeUtil.h"

namespace yyzlib
{
	using namespace yyzlib;

	TEST(YyzlibPtreeUtilTest, ScalarSetGetTest)
	{
		yyzlib::ptree pt;

		// bool
		EXPECT_TRUE(SetValue(pt, "flag", true));
		bool b = false;
		EXPECT_TRUE(GetValue(pt, "flag", b, false));
		EXPECT_TRUE(b);
		EXPECT_TRUE(SetValue(pt, "flag", false));
		EXPECT_TRUE(GetValue(pt, "flag", b, true));
		EXPECT_FALSE(b);

		// int / uint32 / uint64
		EXPECT_TRUE(SetValue(pt, "count", 42));
		int i = 0;
		EXPECT_TRUE(GetValue(pt, "count", i, 0));
		EXPECT_EQ(i, 42);

		EXPECT_TRUE(SetValue(pt, "u32", 4000000000u));
		uint32_t u32 = 0;
		EXPECT_TRUE(GetValue(pt, "u32", u32, 0));
		EXPECT_EQ(u32, 4000000000u);

		EXPECT_TRUE(SetValue(pt, "u64", 9007199254740993ULL));
		uint64_t u64 = 0;
		EXPECT_TRUE(GetValue(pt, "u64", u64, 0));
		EXPECT_EQ(u64, 9007199254740993ULL);

		// float
		EXPECT_TRUE(SetValue(pt, "ratio", 1.5f));
		float f = 0;
		EXPECT_TRUE(GetValue(pt, "ratio", f, 0));
		EXPECT_FLOAT_EQ(f, 1.5f);

		// string (const char* and std::string overloads)
		EXPECT_TRUE(SetValue(pt, "name", "yyz"));
		EXPECT_TRUE(SetValue(pt, "name2", std::string("tools")));
		std::string s;
		EXPECT_TRUE(GetValue(pt, "name", s, ""));
		EXPECT_EQ(s, "yyz");
		EXPECT_TRUE(GetValue(pt, "name2", s, ""));
		EXPECT_EQ(s, "tools");

		// Missing key returns the default value
		EXPECT_TRUE(GetValue(pt, "no_such_key", i, -1));
		EXPECT_EQ(i, -1);
		EXPECT_TRUE(GetValue(pt, "no_such_key", s, "def"));
		EXPECT_EQ(s, "def");
	}

	TEST(YyzlibPtreeUtilTest, HasValueTest)
	{
		yyzlib::ptree pt;
		SetValue(pt, "a.b", 1);

		EXPECT_TRUE(HasValue(pt, "a.b"));
		EXPECT_FALSE(HasValue(pt, "a.c"));
		EXPECT_FALSE(HasValue(pt, "x"));
	}

	TEST(YyzlibPtreeUtilTest, StringListTest)
	{
		yyzlib::ptree pt;
		StringList in{ "x", "y", "z" };
		EXPECT_TRUE(SetValue(pt, "list", in));

		StringList out;
		EXPECT_TRUE(GetValue(pt, "list", out, {}));
		EXPECT_EQ(out, in);

		// Missing key returns the default value
		StringList def{ "d" };
		EXPECT_TRUE(GetValue(pt, "no_list", out, def));
		EXPECT_EQ(out, def);

		// Empty key: the whole tree acts as an array
		yyzlib::ptree arr;
		SetValue(arr, "", in);
		EXPECT_TRUE(GetValue(arr, "", out, {}));
		EXPECT_EQ(out, in);
	}

	TEST(YyzlibPtreeUtilTest, StringMapTest)
	{
		yyzlib::ptree pt;
		StringMap in{ { "k1", "v1" }, { "k2", "v2" } };
		EXPECT_TRUE(SetValue(pt, "map", in));

		StringMap out;
		EXPECT_TRUE(GetValue(pt, "map", out, {}));
		EXPECT_EQ(out.size(), (size_t)2);
		EXPECT_EQ(out["k1"], "v1");
		EXPECT_EQ(out["k2"], "v2");

		// Missing key returns the default value
		StringMap def{ { "d", "1" } };
		EXPECT_TRUE(GetValue(pt, "no_map", out, def));
		EXPECT_EQ(out, def);
	}

	TEST(YyzlibPtreeUtilTest, StringMapListTest)
	{
		yyzlib::ptree pt;
		StringMapList in;
		in.push_back({ { "id", "1" }, { "name", "a" } });
		in.push_back({ { "id", "2" }, { "name", "b" } });
		EXPECT_TRUE(SetValue(pt, "items", in));

		StringMapList out;
		EXPECT_TRUE(GetValue(pt, "items", out, {}));
		EXPECT_EQ(out.size(), (size_t)2);
		EXPECT_EQ(out[0]["id"], "1");
		EXPECT_EQ(out[1]["name"], "b");

		// Missing key returns the default value
		EXPECT_TRUE(GetValue(pt, "no_items", out, in));
		EXPECT_EQ(out.size(), (size_t)2);
	}

	TEST(YyzlibPtreeUtilTest, ChildPtreeTest)
	{
		yyzlib::ptree pt;
		yyzlib::ptree child;
		SetValue(child, "inner", 99);
		EXPECT_TRUE(SetValue(pt, "child", child));

		yyzlib::ptree got;
		EXPECT_TRUE(GetValue(pt, "child", got, {}));
		int v = 0;
		GetValue(got, "inner", v, 0);
		EXPECT_EQ(v, 99);
	}

	// Empty key branch: operate on the root node (StringMap / StringMapList / Ptree containers)
	TEST(YyzlibPtreeUtilTest, EmptyKeyContainerTest)
	{
		// StringMap: the root node is the map
		{
			yyzlib::ptree pt;
			StringMap m{ {"k1", "v1"}, {"k2", "v2"} };
			EXPECT_TRUE(SetValue(pt, "", m));

			StringMap out;
			EXPECT_TRUE(GetValue(pt, "", out, {}));
			EXPECT_EQ(out.size(), (size_t)2);
			EXPECT_EQ(out["k1"], "v1");
			EXPECT_EQ(out["k2"], "v2");
		}

		// StringMapList: the root node is the map array
		{
			yyzlib::ptree pt;
			StringMapList l{ StringMap{ {"a", "1"} }, StringMap{ {"b", "2"} } };
			EXPECT_TRUE(SetValue(pt, "", l));

			StringMapList out;
			EXPECT_TRUE(GetValue(pt, "", out, {}));
			ASSERT_EQ(out.size(), (size_t)2);
			EXPECT_EQ(out[0]["a"], "1");
			EXPECT_EQ(out[1]["b"], "2");
		}

		// Ptree: assign/read the whole tree with an empty key
		{
			yyzlib::ptree src;
			SetValue(src, "x", 5);
			yyzlib::ptree pt;
			EXPECT_TRUE(SetValue(pt, "", src));

			yyzlib::ptree out;
			EXPECT_TRUE(GetValue(pt, "", out, {}));
			EXPECT_EQ(out.get<int>("x", 0), 5);
		}
	}

	TEST(YyzlibPtreeUtilTest, JsonRoundTripTest)
	{
		yyzlib::ptree pt;
		SetValue(pt, "num", 7);
		SetValue(pt, "str", "hello");
		SetValue(pt, "list", StringList{ "a", "b" });
		SetValue(pt, "map.x", 1);

		std::string json = SaveToString(pt, false);
		EXPECT_NE(json.find("hello"), std::string::npos);

		yyzlib::ptree loaded;
		EXPECT_TRUE(LoadFromString(loaded, json));

		int n = 0;
		std::string s;
		EXPECT_TRUE(GetValue(loaded, "num", n, 0));
		EXPECT_EQ(n, 7);
		EXPECT_TRUE(GetValue(loaded, "str", s, ""));
		EXPECT_EQ(s, "hello");

		// Invalid JSON returns false
		yyzlib::ptree bad;
		EXPECT_FALSE(LoadFromString(bad, "{ not json !!"));
	}

	TEST(YyzlibPtreeUtilTest, MergeTest)
	{
		yyzlib::ptree target;
		SetValue(target, "a", 1);
		SetValue(target, "sub.x", 10);
		SetValue(target, "keep", "old");

		yyzlib::ptree input;
		SetValue(input, "a", 2);			// Overwrite leaf
		SetValue(input, "sub.y", 20);		// Recursively merge child objects
		SetValue(input, "add", "new");		// Add new key

		Merge(target, input);

		EXPECT_EQ(target.get<int>("a", 0), 2);
		EXPECT_EQ(target.get<int>("sub.x", 0), 10);	// Existing child key preserved
		EXPECT_EQ(target.get<int>("sub.y", 0), 20);	// New child key merged in
		EXPECT_EQ(target.get<std::string>("keep", ""), "old");
		EXPECT_EQ(target.get<std::string>("add", ""), "new");

		// Arrays are replaced wholesale
		yyzlib::ptree with_arr;
		SetValue(with_arr, "arr", StringList{ "1", "2" });
		yyzlib::ptree new_arr;
		SetValue(new_arr, "arr", StringList{ "3" });
		Merge(with_arr, new_arr);

		StringList out;
		GetValue(with_arr, "arr", out, {});
		EXPECT_EQ(out, (StringList{ "3" }));
	}

	// Source root carries data: MergeTree copies the source root data to the target
	TEST(YyzlibPtreeUtilTest, MergeRootDataTest)
	{
		yyzlib::ptree target;
		SetValue(target, "a", 1);

		yyzlib::ptree input;
		input.put("", std::string("root_value"));	// Assign data on the root node
		SetValue(input, "b", 2);

		Merge(target, input);

		EXPECT_EQ(target.data(), "root_value");		// Root data overwritten
		EXPECT_EQ(target.get<int>("a", 0), 1);		// Existing child key preserved
		EXPECT_EQ(target.get<int>("b", 0), 2);		// New child key merged in
	}

	// Target has a leaf at the same key while the source has an object: cannot merge recursively -> append as a new node
	TEST(YyzlibPtreeUtilTest, MergeLeafToObjectTest)
	{
		yyzlib::ptree target;
		SetValue(target, "a", "leaf");

		yyzlib::ptree input;
		SetValue(input, "a.b", 1);					// a is an object in the source

		Merge(target, input);

		// Original leaf kept, object appended as a second child node with the same key
		EXPECT_EQ(target.get<std::string>("a", ""), "leaf");
		auto range = target.equal_range("a");
		int count = 0;
		bool has_object = false;
		for (auto it = range.first; it != range.second; ++it, ++count) {
			if (!it->second.empty()) has_object = true;
		}
		EXPECT_EQ(count, 2);
		EXPECT_TRUE(has_object);
	}

	TEST(YyzlibPtreeUtilTest, TypeMismatchFallbackTest)
	{
		// Contract: missing keys or type mismatches return the default value (the bool return value only means no exception occurred)
		yyzlib::ptree pt;
		pt.put("bad", std::string("not_a_number"));

		int iv = 99;
		GetValue(pt, "bad", iv, -1);
		EXPECT_EQ(iv, -1);

		float fv = 2.5f;
		GetValue(pt, "bad", fv, 1.5f);
		EXPECT_FLOAT_EQ(fv, 1.5f);

		// bool special case: a garbage string parses to false (FromString<bool> semantics), no fall back to the default
		bool bv = false;
		GetValue(pt, "bad", bv, true);
		EXPECT_FALSE(bv);

		// Missing key falls back to the default value
		int missing = 0;
		GetValue(pt, "no_such_key", missing, 42);
		EXPECT_EQ(missing, 42);

		// Parsing of valid bool literals
		yyzlib::ptree ok;
		ok.put("b", std::string("true"));
		bool parsed = false;
		EXPECT_TRUE(GetValue(ok, "b", parsed, false));
		EXPECT_TRUE(parsed);
	}

	//Typed output: numbers/booleans emit native JSON values, strings are quoted, null emits null
	TEST(YyzlibPtreeUtilTest, TypedOutputTest)
	{
		yyzlib::ptree pt;
		SetValue(pt, "error", 0);
		SetValue(pt, "count", 42);
		SetValue(pt, "flag", true);
		SetValue(pt, "ratio", 1.5f);
		SetValue(pt, "name", std::string("yyz"));
		SetValue(pt, "ver", "1.0");		//Strings that look like numbers must stay strings
		SetValue(pt, "list", StringList{ "a", "b" });

		std::string json = SaveToString(pt, false);

		// Numbers/booleans emitted natively
		EXPECT_NE(json.find("\"error\":0"), std::string::npos);
		EXPECT_NE(json.find("\"count\":42"), std::string::npos);
		EXPECT_NE(json.find("\"flag\":true"), std::string::npos);
		EXPECT_NE(json.find("\"ratio\":1.5"), std::string::npos);
		// Strings stay quoted
		EXPECT_NE(json.find("\"name\":\"yyz\""), std::string::npos);
		EXPECT_NE(json.find("\"ver\":\"1.0\""), std::string::npos);
		// Array elements are strings
		EXPECT_NE(json.find("[\"a\",\"b\"]"), std::string::npos);
	}

	//Legacy (all-string) format read compatibility: read behavior identical to the new format
	TEST(YyzlibPtreeUtilTest, LegacyStringFormatReadTest)
	{
		// Simulate an all-string config written by a legacy version
		const char* legacy = "{\"error\":\"0\",\"count\":\"42\",\"flag\":\"true\",\"name\":\"yyz\",\"ratio\":\"1.5\"}";

		yyzlib::ptree pt;
		EXPECT_TRUE(LoadFromString(pt, legacy));

		// Text reads are compatible
		int n = 0;
		EXPECT_TRUE(GetValue(pt, "count", n, 0));
		EXPECT_EQ(n, 42);
		bool b = false;
		EXPECT_TRUE(GetValue(pt, "flag", b, false));
		EXPECT_TRUE(b);
		std::string s;
		EXPECT_TRUE(GetValue(pt, "name", s, ""));
		EXPECT_EQ(s, "yyz");
		float d = 0;
		EXPECT_TRUE(GetValue(pt, "ratio", d, 0));
		EXPECT_FLOAT_EQ(d, 1.5f);

		// Legacy nodes keep the String type (writing back does not change the form)
		yyzlib::ptree count_node;
		EXPECT_TRUE(GetValue(pt, "count", count_node, {}));
		EXPECT_EQ(count_node.type(), yyzlib::NodeType::String);
	}

	//New format read: FromJson records native types; both reads and roundtrip are correct
	TEST(YyzlibPtreeUtilTest, NativeFormatReadTest)
	{
		const char* native = "{\"error\":0,\"count\":42,\"flag\":true,\"name\":\"yyz\",\"ratio\":1.5,\"nil\":null}";

		yyzlib::ptree pt;
		EXPECT_TRUE(LoadFromString(pt, native));

		EXPECT_EQ(pt.get<int>("count", 0), 42);
		EXPECT_TRUE(pt.get<bool>("flag", false));
		EXPECT_EQ(pt.get<std::string>("name", ""), "yyz");
		EXPECT_DOUBLE_EQ(pt.get<double>("ratio", 0), 1.5);

		// Type markers are correct
		EXPECT_EQ(pt.get_child("count").type(), yyzlib::NodeType::Integer);
		EXPECT_EQ(pt.get_child("flag").type(), yyzlib::NodeType::Boolean);
		EXPECT_EQ(pt.get_child("name").type(), yyzlib::NodeType::String);
		EXPECT_EQ(pt.get_child("nil").type(), yyzlib::NodeType::Null);

		// Serialization preserves native forms; null emits null
		std::string json = SaveToString(pt, false);
		EXPECT_NE(json.find("\"error\":0"), std::string::npos);
		EXPECT_NE(json.find("\"flag\":true"), std::string::npos);
		EXPECT_NE(json.find("\"nil\":null"), std::string::npos);
	}

	//After Merge the type follows the source node (setConfig incremental merge path)
	TEST(YyzlibPtreeUtilTest, MergeTypeTest)
	{
		yyzlib::ptree target;
		SetValue(target, "error", 0);		// Integer
		SetValue(target, "keep", "old");	// String

		yyzlib::ptree input;
		SetValue(input, "error", 1);		// Integer overwrite

		Merge(target, input);

		EXPECT_EQ(target.get_child("error").type(), yyzlib::NodeType::Integer);
		EXPECT_EQ(target.get<int>("error", 0), 1);
		EXPECT_EQ(target.get_child("keep").type(), yyzlib::NodeType::String);
		EXPECT_EQ(target.get<std::string>("keep", ""), "old");
	}

	//Type markers: put / put_value / set_data write paths all record NodeType correctly
	TEST(YyzlibPtreeUtilTest, TypeMarkerTest)
	{
		yyzlib::ptree pt;
		pt.put("b", true);
		pt.put("i", 42);
		pt.put("u", 4000000000u);
		pt.put("ull", 9007199254740993ULL);
		pt.put("f", 1.5f);
		pt.put("s", std::string("x"));
		const char* cc = "y";
		pt.put("cc", cc);			//const char* goes through String
		pt.put("neg", -5);

		EXPECT_EQ(pt.get_child("b").type(), yyzlib::NodeType::Boolean);
		EXPECT_EQ(pt.get_child("i").type(), yyzlib::NodeType::Integer);
		EXPECT_EQ(pt.get_child("u").type(), yyzlib::NodeType::Integer);
		EXPECT_EQ(pt.get_child("ull").type(), yyzlib::NodeType::Integer);
		EXPECT_EQ(pt.get_child("f").type(), yyzlib::NodeType::Float);
		EXPECT_EQ(pt.get_child("s").type(), yyzlib::NodeType::String);
		EXPECT_EQ(pt.get_child("cc").type(), yyzlib::NodeType::String);
		EXPECT_EQ(pt.get_child("neg").type(), yyzlib::NodeType::Integer);

		// put_value
		yyzlib::ptree node;
		node.put_value(7);
		EXPECT_EQ(node.type(), yyzlib::NodeType::Integer);
		node.put_value(false);
		EXPECT_EQ(node.type(), yyzlib::NodeType::Boolean);

		// set_data with explicit type
		node.set_data("99", yyzlib::NodeType::Integer);
		EXPECT_EQ(node.type(), yyzlib::NodeType::Integer);
		EXPECT_EQ(node.data(), "99");
	}

	//Typed output still correct in pretty format (config file write-to-disk path)
	TEST(YyzlibPtreeUtilTest, PrettyTypedOutputTest)
	{
		yyzlib::ptree pt;
		SetValue(pt, "error", 0);
		SetValue(pt, "flag", true);
		SetValue(pt, "name", std::string("yyz"));

		std::string json = SaveToString(pt, true);	// ConfigMgr::Save uses this path

		EXPECT_NE(json.find('\n'), std::string::npos);			// Indeed indented format
		EXPECT_NE(json.find("\"error\": 0"), std::string::npos);	// Space after the colon
		EXPECT_NE(json.find("\"flag\": true"), std::string::npos);
		EXPECT_NE(json.find("\"name\": \"yyz\""), std::string::npos);

		// Pretty output can be read back losslessly
		yyzlib::ptree loaded;
		EXPECT_TRUE(LoadFromString(loaded, json));
		EXPECT_EQ(loaded.get<int>("error", -1), 0);
		EXPECT_TRUE(loaded.get<bool>("flag", false));
		EXPECT_EQ(loaded.get<std::string>("name", ""), "yyz");
	}

	//Native-type roundtrip for array elements (FromJson array branch records types)
	TEST(YyzlibPtreeUtilTest, NativeArrayRoundTripTest)
	{
		const char* src = "[1,-2,3.5,true,null,\"txt\"]";
		yyzlib::ptree pt;
		EXPECT_TRUE(LoadFromString(pt, src));

		std::string out = SaveToString(pt, false);
		EXPECT_NE(out.find("[1,-2,3.5,true,null,\"txt\"]"), std::string::npos);

		// Array element type markers and values (child keys are empty strings, accessed in order)
		ASSERT_EQ(pt.size(), (size_t)6);
		auto it = pt.begin();
		EXPECT_EQ(it->second.type(), yyzlib::NodeType::Integer);
		EXPECT_EQ(it->second.get_value<int>(), 1);
		++it;
		EXPECT_EQ(it->second.get_value<int>(), -2);
		++it;
		EXPECT_EQ(it->second.type(), yyzlib::NodeType::Float);
		++it;
		EXPECT_EQ(it->second.type(), yyzlib::NodeType::Boolean);
		EXPECT_TRUE(it->second.get_value<bool>());
		++it;
		EXPECT_EQ(it->second.type(), yyzlib::NodeType::Null);
		++it;
		EXPECT_EQ(it->second.type(), yyzlib::NodeType::String);
		EXPECT_EQ(it->second.get_value<std::string>(), "txt");
	}

	//Defensive branch: typed nodes with empty data fall back to string output to keep JSON valid
	TEST(YyzlibPtreeUtilTest, EmptyDataTypedNodeTest)
	{
		yyzlib::ptree pt;
		pt.set_data("", yyzlib::NodeType::Integer);
		std::string json = SaveToString(pt, false);
		EXPECT_NE(json.find("\"\""), std::string::npos);	// Falls back to a quoted empty string instead of a bare value (invalid JSON)

		yyzlib::ptree pt2;
		pt2.set_data("", yyzlib::NodeType::Null);
		EXPECT_NE(SaveToString(pt2, false).find("null"), std::string::npos);
	}

	//Overwrite: put on an existing key replaces both data and type
	TEST(YyzlibPtreeUtilTest, OverwriteTypeTest)
	{
		yyzlib::ptree pt;
		pt.put("v", 42);				// Integer
		pt.put("v", std::string("42"));	// Overwritten to String
		EXPECT_EQ(pt.get_child("v").type(), yyzlib::NodeType::String);
		EXPECT_NE(SaveToString(pt, false).find("\"v\":\"42\""), std::string::npos);

		pt.put("v", 42);				// Overwritten back to Integer
		EXPECT_EQ(pt.get_child("v").type(), yyzlib::NodeType::Integer);
		EXPECT_NE(SaveToString(pt, false).find("\"v\":42"), std::string::npos);
	}
}

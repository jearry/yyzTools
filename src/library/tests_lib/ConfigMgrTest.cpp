/*****************************************************************************
*  yyzlib Config Manager - unit tests
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
#include "ConfigMgr.h"

namespace yyzlib
{
	using namespace yyzlib;

	// The singleton accepts Init only once; use a dedicated suite to guarantee init order
	class YyzlibConfigMgrTest : public ::testing::Test
	{
	protected:
		static void SetUpTestSuite()
		{
			TCHAR buf[MAX_PATH] = { 0 };
			GetTempPathW(MAX_PATH, buf);
			m_config_file = tstring(buf) + L"yyzlib_config_test.json";
			FileDelete(m_config_file);

			ConfigList defs = {
				{ "test.int", ConfigValue(10), nullptr, nullptr },
				{ "test.str", ConfigValue(std::string("def")), nullptr, nullptr },
				{ "test.list", ConfigValue(StringList{ "a" }), nullptr, nullptr },
				{ "test.wild.*", ConfigValue(0), nullptr, nullptr },
				{ "test.maps", ConfigValue(StringMapList{}), nullptr, nullptr },
				// All-type branches for the batch path
				{ "test.f", ConfigValue(1.5f), nullptr, nullptr },
				{ "test.s2", ConfigValue(std::string("sdef")), nullptr, nullptr },
				{ "test.sm", ConfigValue(StringMap{}), nullptr, nullptr },
				{ "test.sl", ConfigValue(StringList{}), nullptr, nullptr },
				// Exact-key trigger: negative values rejected, PostFunc counts
				{ "trig.value", ConfigValue(0),
					[](const std::string& key, const ConfigValue& old_v, const ConfigValue& new_v) {
						g_last_key = key;
						g_pre_old = std::get<int>(old_v);
						g_pre_new = std::get<int>(new_v);
						return std::get<int>(new_v) >= 0;	// Negative values rejected
					},
					[]() { ++g_post_count; } },
				// Wildcard-key trigger: only counts calls
				{ "trig.wild.*", ConfigValue(0),
					[](const std::string& key, const ConfigValue&, const ConfigValue&) {
						++g_wild_pre_count;
						return true;
					},
					[]() { ++g_wild_post_count; } },
			};

			ConfigMgr::Instance().Init(defs, m_config_file, false);
			ConfigMgr::Instance().SetAutoSaveFlag(false);
		}

		static tstring m_config_file;

	public:
		static std::string g_last_key;
		static int g_pre_old;
		static int g_pre_new;
		static int g_post_count;
		static int g_wild_pre_count;
		static int g_wild_post_count;
	};

	tstring YyzlibConfigMgrTest::m_config_file;
	std::string YyzlibConfigMgrTest::g_last_key;
	int YyzlibConfigMgrTest::g_pre_old = -1;
	int YyzlibConfigMgrTest::g_pre_new = -1;
	int YyzlibConfigMgrTest::g_post_count = 0;
	int YyzlibConfigMgrTest::g_wild_pre_count = 0;
	int YyzlibConfigMgrTest::g_wild_post_count = 0;

	TEST_F(YyzlibConfigMgrTest, ScalarGetSetTest)
	{
		auto& cfg = ConfigMgr::Instance();

		// Default values
		EXPECT_EQ(cfg.GetInt("test.int"), 10);
		EXPECT_EQ(cfg.GetString("test.str"), "def");

		// Set / Get
		EXPECT_TRUE(cfg.Set("test.int", 99));
		EXPECT_EQ(cfg.GetInt("test.int"), 99);

		EXPECT_TRUE(cfg.Set("test.str", std::string("hello")));
		EXPECT_EQ(cfg.GetString("test.str"), "hello");

		// Undefined key: no default, readable after write
		EXPECT_TRUE(cfg.Set("test.new", 5));
		EXPECT_EQ(cfg.GetInt("test.new"), 5);
		EXPECT_EQ(cfg.GetInt("test.none"), 0);
	}

	TEST_F(YyzlibConfigMgrTest, StringListTest)
	{
		auto& cfg = ConfigMgr::Instance();

		EXPECT_EQ(cfg.GetStringList("test.list"), (StringList{ "a" }));

		StringList v{ "x", "y" };
		EXPECT_TRUE(cfg.Set("test.list", v));
		EXPECT_EQ(cfg.GetStringList("test.list"), v);

		// AddKeyValue appends
		EXPECT_TRUE(cfg.AddKeyValue("test.list", "z"));
		EXPECT_EQ(cfg.GetStringList("test.list"), (StringList{ "x", "y", "z" }));
	}

	TEST_F(YyzlibConfigMgrTest, StructTest)
	{
		auto& cfg = ConfigMgr::Instance();

		StringMap m{ { "k", "v" } };
		EXPECT_TRUE(cfg.Set("test.map", m));
		EXPECT_EQ(cfg.GetStruct("test.map")["k"], "v");

		// AddKeyValue on a list key requires a StringMapList default in the definition
		EXPECT_TRUE(cfg.Set("test.maps", StringMapList{ m }));
		StringMap m2{ { "k2", "v2" } };
		EXPECT_TRUE(cfg.AddKeyValue("test.maps", m2));
		EXPECT_EQ(cfg.GetStructList("test.maps").size(), (size_t)2);

		// Duplicate data is rejected
		EXPECT_FALSE(cfg.AddKeyValue("test.maps", m2));
	}

	TEST_F(YyzlibConfigMgrTest, WildcardKeyTest)
	{
		EXPECT_TRUE(IsWildcardKey("test.wild.*"));
		EXPECT_FALSE(IsWildcardKey("test.int"));

		auto& cfg = ConfigMgr::Instance();

		ConfigItem item;
		EXPECT_FALSE(cfg.FindItem("test.wild.anything", item));		// Exact lookup does not hit wildcard
		EXPECT_TRUE(cfg.FindItemWildcard("test.wild.anything", item));	// Wildcard lookup hits
		EXPECT_EQ(item.Key, "test.wild.*");

		// Wildcard key values are writable and readable
		EXPECT_TRUE(cfg.Set("test.wild.sub", 3));
		EXPECT_EQ(cfg.GetInt("test.wild.sub"), 3);
	}

	TEST_F(YyzlibConfigMgrTest, TriggerTest)
	{
		auto& cfg = ConfigMgr::Instance();
		g_post_count = 0;

		// PreFunc passes + PostFunc fires
		EXPECT_TRUE(cfg.Set("trig.value", 5));
		EXPECT_EQ(g_last_key, "trig.value");
		EXPECT_EQ(g_pre_old, 0);		// old value is the default
		EXPECT_EQ(g_pre_new, 5);
		EXPECT_EQ(cfg.GetInt("trig.value"), 5);
		EXPECT_EQ(g_post_count, 1);

		// PreFunc rejects: Set returns false, value unchanged, PostFunc not fired
		EXPECT_FALSE(cfg.Set("trig.value", -3));
		EXPECT_EQ(g_pre_new, -3);		// PreFunc was indeed called
		EXPECT_EQ(cfg.GetInt("trig.value"), 5);
		EXPECT_EQ(g_post_count, 1);

		// No callbacks fire when the value is unchanged
		EXPECT_TRUE(cfg.Set("trig.value", 5));
		EXPECT_EQ(g_post_count, 1);
	}

	TEST_F(YyzlibConfigMgrTest, WildcardTriggerTest)
	{
		auto& cfg = ConfigMgr::Instance();
		g_wild_pre_count = 0;
		g_wild_post_count = 0;

		// Any child key matching trig.wild.* fires the wildcard callbacks
		EXPECT_TRUE(cfg.Set("trig.wild.sub1", 1));
		EXPECT_EQ(g_wild_pre_count, 1);
		EXPECT_EQ(g_wild_post_count, 1);
		EXPECT_EQ(cfg.GetInt("trig.wild.sub1"), 1);

		EXPECT_TRUE(cfg.Set("trig.wild.sub2", 2));
		EXPECT_EQ(g_wild_pre_count, 2);
		EXPECT_EQ(g_wild_post_count, 2);
	}

	TEST_F(YyzlibConfigMgrTest, ModifyConfigInvalidJsonTest)
	{
		auto& cfg = ConfigMgr::Instance();

		// Invalid JSON: both APIs return false
		EXPECT_FALSE(cfg.MergeConfig("not a json"));
		EXPECT_FALSE(cfg.SetAllConfig("{"));
	}

	TEST_F(YyzlibConfigMgrTest, BatchAllTypesTest)
	{
		auto& cfg = ConfigMgr::Instance();

		// Batch path modifies every key type at once (covers all type branches of ModifyPropertyTree)
		EXPECT_TRUE(cfg.SetAllConfig(
			"{\"test\":{\"int\":1,\"f\":2.5,\"s2\":\"batch\",\"sm\":{\"k\":\"v\"},\"sl\":[\"a\",\"b\"]}}"));

		EXPECT_EQ(cfg.GetInt("test.int"), 1);
		EXPECT_FLOAT_EQ(cfg.GetFloat("test.f"), 2.5f);
		EXPECT_EQ(cfg.GetString("test.s2"), "batch");
		EXPECT_EQ(cfg.GetStruct("test.sm")["k"], "v");
		EXPECT_EQ(cfg.GetStringList("test.sl"), (StringList{ "a", "b" }));
	}

	TEST_F(YyzlibConfigMgrTest, BatchTriggerVetoTest)
	{
		auto& cfg = ConfigMgr::Instance();
		g_post_count = 0;
		cfg.Set("trig.value", 5);
		g_post_count = 0;	// The Set above fires PostFunc once; reset the baseline

		// Batch path sets trig.value negative: PreFunc veto -> whole batch fails, value unchanged
		EXPECT_FALSE(cfg.SetAllConfig("{\"trig\":{\"value\":-1}}"));
		EXPECT_EQ(cfg.GetInt("trig.value"), 5);
		EXPECT_EQ(g_post_count, 0);

		// Batch path sets trig.value positive: succeeds and PostFunc fires
		EXPECT_TRUE(cfg.SetAllConfig("{\"trig\":{\"value\":9}}"));
		EXPECT_EQ(cfg.GetInt("trig.value"), 9);
		EXPECT_EQ(g_post_count, 1);
	}

	TEST_F(YyzlibConfigMgrTest, BatchWildcardTriggerTest)
	{
		auto& cfg = ConfigMgr::Instance();
		g_wild_pre_count = 0;
		g_wild_post_count = 0;

		// Merge mode: no wildcard subtree means no trigger (missing keys are skipped)
		EXPECT_TRUE(cfg.MergeConfig("{\"test\":{\"int\":1}}"));
		EXPECT_EQ(g_wild_pre_count, 0);

		// SetAll mode carries the trig.wild.* child key: wildcard callbacks fire
		EXPECT_TRUE(cfg.SetAllConfig("{\"trig\":{\"wild\":{\"x\":7}}}"));
		EXPECT_GE(g_wild_pre_count, 1);
		EXPECT_GE(g_wild_post_count, 1);
		EXPECT_EQ(cfg.GetInt("trig.wild.x"), 7);
	}

	TEST_F(YyzlibConfigMgrTest, PropertyTreeDefaultsTest)
	{
		auto& cfg = ConfigMgr::Instance();

		// GetPropertyTree injects defaults for defined keys with no value
		yyzlib::ptree pt = cfg.GetPropertyTree();
		EXPECT_EQ(pt.get<int>("test.int", -1), cfg.GetInt("test.int"));
	}

	TEST_F(YyzlibConfigMgrTest, ModifyConfigTest)
	{
		auto& cfg = ConfigMgr::Instance();

		cfg.Set("test.int", 1);
		cfg.Set("test.str", std::string("old"));

		// MergeConfig: only merges existing keys
		EXPECT_TRUE(cfg.MergeConfig("{\"test\":{\"int\":2}}"));
		EXPECT_EQ(cfg.GetInt("test.int"), 2);
		EXPECT_EQ(cfg.GetString("test.str"), "old");

		// SetAllConfig: keys absent from the input are removed
		EXPECT_TRUE(cfg.SetAllConfig("{\"test\":{\"int\":3}}"));
		EXPECT_EQ(cfg.GetInt("test.int"), 3);
		EXPECT_NE(cfg.GetString("test.str"), "old");	// removed, falls back to default "def"

		// GetAllConfig outputs JSON
		std::string all = cfg.GetAllConfig();
		EXPECT_NE(all.find("\"int\""), std::string::npos);
	}

	TEST_F(YyzlibConfigMgrTest, GetFloatAndChildTest)
	{
		auto& cfg = ConfigMgr::Instance();

		EXPECT_TRUE(cfg.Set("test.float", 1.5f));
		EXPECT_FLOAT_EQ(cfg.GetFloat("test.float"), 1.5f);
		EXPECT_FLOAT_EQ(cfg.GetFloat("test.float.none"), 0.0f);

		// GetChild returns the subtree
		yyzlib::ptree child = cfg.GetChild("test");
		EXPECT_EQ(child.get<int>("int", -1), cfg.GetInt("test.int"));
	}

	TEST_F(YyzlibConfigMgrTest, AutoSaveTest)
	{
		auto& cfg = ConfigMgr::Instance();
		FileDelete(m_config_file);

		// Default autosave is controlled by Init's 3rd arg, initialized to false in the suite: Set does not touch disk
		cfg.Set("autosave.key", 1);
		EXPECT_FALSE(FileExists(m_config_file));

		// With autosave on, Set saves automatically
		cfg.SetAutoSaveFlag(true);
		EXPECT_TRUE(cfg.Set("autosave.key", 2));
		EXPECT_TRUE(FileExists(m_config_file));
		EXPECT_NE(LoadFileString(m_config_file).find("autosave"), std::string::npos);

		cfg.SetAutoSaveFlag(false);
		FileDelete(m_config_file);
	}

	TEST_F(YyzlibConfigMgrTest, ReloadTest)
	{
		auto& cfg = ConfigMgr::Instance();
		FileDelete(m_config_file);

		cfg.Set("persist.int", 42);
		EXPECT_TRUE(cfg.Save());

		// Re-Init loads from the file
		ConfigList defs = { { "persist.int", ConfigValue(0), nullptr, nullptr } };
		cfg.Init(defs, m_config_file, false);
		EXPECT_EQ(cfg.GetInt("persist.int"), 42);

		FileDelete(m_config_file);
	}

	TEST_F(YyzlibConfigMgrTest, SaveLoadTest)
	{
		auto& cfg = ConfigMgr::Instance();

		cfg.Set("persist.str", std::string("saved"));
		EXPECT_TRUE(cfg.Save());
		EXPECT_TRUE(FileExists(m_config_file));

		// File content is valid JSON
		std::string json = LoadFileString(m_config_file);
		yyzlib::ptree pt;
		EXPECT_TRUE(LoadFromString(pt, json));
		EXPECT_NE(json.find("saved"), std::string::npos);

		FileDelete(m_config_file);
	}

	//Legacy all-string config upgrade: reads correctly, re-saved keys get native types, untouched keys keep their original form
	//Note: "." in a C++ key is the path separator; "legacy.int" maps to the nested {"legacy": {"int": ...}} in the file
	TEST_F(YyzlibConfigMgrTest, LegacyFormatUpgradeTest)
	{
		auto& cfg = ConfigMgr::Instance();
		FileDelete(m_config_file);

		// Simulate a legacy all-string config file (nested form)
		const char* legacy = "{\n"
			"    \"legacy\": {\n"
			"        \"int\": \"42\",\n"
			"        \"flag\": \"true\",\n"
			"        \"str\": \"hello\"\n"
			"    },\n"
			"    \"nested\": {\n"
			"        \"num\": \"7\"\n"
			"    }\n"
			"}";
		EXPECT_TRUE(SaveFileString(m_config_file, legacy));

		// Read: values from the legacy all-string format are correct
		cfg.Init(ConfigList{}, m_config_file, false);
		EXPECT_EQ(cfg.GetInt("legacy.int"), 42);
		EXPECT_EQ(cfg.GetInt("nested.num"), 7);
		EXPECT_EQ(cfg.GetString("legacy.str"), "hello");
		EXPECT_TRUE(cfg.GetBool("legacy.flag"));	// "true" string read as bool

		// Modify one key and save: the new value is written with its real type, untouched legacy keys keep string form
		EXPECT_TRUE(cfg.Set("legacy.int", 100));
		EXPECT_TRUE(cfg.Save());

		std::string saved = LoadFileString(m_config_file);
		EXPECT_NE(saved.find("\"int\": 100"), std::string::npos);				// int rewritten as native
		EXPECT_NE(saved.find("\"flag\": \"true\""), std::string::npos);		// legacy key untouched, form unchanged
		EXPECT_NE(saved.find("\"str\": \"hello\""), std::string::npos);

		// Read after re-save: values still correct with both forms coexisting
		cfg.Init(ConfigList{}, m_config_file, false);
		EXPECT_EQ(cfg.GetInt("legacy.int"), 100);
		EXPECT_EQ(cfg.GetInt("nested.num"), 7);
		EXPECT_TRUE(cfg.GetBool("legacy.flag"));

		FileDelete(m_config_file);
	}

	//bool config entries: missing key falls back to the bool default; three read forms ("1"/"true"/native true); Set bool writes native form to disk
	TEST_F(YyzlibConfigMgrTest, BoolConfigTest)
	{
		ConfigList defs = {
			{ "boolEnabled", ConfigValue(true), nullptr, nullptr },
		};
		auto& cfg = ConfigMgr::Instance();
		FileDelete(m_config_file);

		// Missing key falls back to the bool default (variant bool branch + get_if guard path)
		cfg.Init(defs, m_config_file, false);
		EXPECT_TRUE(cfg.GetBool("boolEnabled"));

		// Legacy form "1" read
		EXPECT_TRUE(SaveFileString(m_config_file, "{\n    \"boolEnabled\": \"1\"\n}"));
		cfg.Init(defs, m_config_file, false);
		EXPECT_TRUE(cfg.GetBool("boolEnabled"));

		// String form "true" read
		EXPECT_TRUE(SaveFileString(m_config_file, "{\n    \"boolEnabled\": \"true\"\n}"));
		cfg.Init(defs, m_config_file, false);
		EXPECT_TRUE(cfg.GetBool("boolEnabled"));

		// Native true read
		EXPECT_TRUE(SaveFileString(m_config_file, "{\n    \"boolEnabled\": true\n}"));
		cfg.Init(defs, m_config_file, false);
		EXPECT_TRUE(cfg.GetBool("boolEnabled"));

		// Set bool + Save: native false written to disk
		EXPECT_TRUE(cfg.Set("boolEnabled", false));
		EXPECT_TRUE(cfg.Save());
		EXPECT_NE(LoadFileString(m_config_file).find("\"boolEnabled\": false"), std::string::npos);

		// Native false read
		cfg.Init(defs, m_config_file, false);
		EXPECT_FALSE(cfg.GetBool("boolEnabled"));

		// GetInt on a bool key does not crash (get_if guard, falls back to the caller's initial 0)
		EXPECT_EQ(cfg.GetInt("boolEnabled"), 0);

		FileDelete(m_config_file);
	}
}

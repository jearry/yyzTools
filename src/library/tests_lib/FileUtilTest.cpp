/*****************************************************************************
*  yyzlib file utilities - unit tests
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
#include "FileUtil.h"
#include "Text.h"

namespace yyzlib
{
	// Temporary directory used by the tests
	static tstring GetTestTempDir()
	{
		TCHAR buf[MAX_PATH] = { 0 };
		GetTempPathW(MAX_PATH, buf);
		std::filesystem::path dir = std::filesystem::path(buf) / L"yyzlib_file_test";
		std::error_code ec;
		std::filesystem::remove_all(dir, ec);
		return dir.wstring();
	}

	static tstring g_test_dir = GetTestTempDir();

	TEST(YyzlibFileUtilTest, PathParseTest)
	{
		tstring full = L"C:\\dir1\\dir2\\readme.txt";

		EXPECT_EQ(GetFileName(full), L"readme.txt");
		EXPECT_EQ(GetStemName(full), L"readme");
		EXPECT_EQ(GetFileExtension(full), L".txt");
		EXPECT_EQ(GetFileDirectory(full), L"C:\\dir1\\dir2");

		// no extension
		EXPECT_EQ(GetFileName(L"C:\\dir\\noext"), L"noext");
		EXPECT_EQ(GetFileExtension(L"C:\\dir\\noext"), L"");

		// relative path
		EXPECT_EQ(GetFileName(L"sub/file.cfg"), L"file.cfg");
	}

	TEST(YyzlibFileUtilTest, SaveLoadStringTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\str.txt";

		EXPECT_TRUE(SaveFileString(file, "hello\nworld"));
		EXPECT_TRUE(FileExists(file));

		std::string content = LoadFileString(file);
		EXPECT_EQ(content, "hello\nworld\n");	// LoadFileString appends '\n' per line

		EXPECT_TRUE(FileDelete(file));
		EXPECT_FALSE(FileExists(file));
	}

	TEST(YyzlibFileUtilTest, SaveLoadBinaryTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\bin.dat";

		std::vector<uint8_t> data = { 0x00, 0x01, 0xFF, 0xFE, 'A', 'B' };
		EXPECT_TRUE(SaveFileBinary(file, data));

		std::vector<uint8_t> loaded = LoadFileBinary(file);
		EXPECT_EQ(loaded, data);

		EXPECT_EQ(GetFileSize(file), (int64_t)data.size());
		FileDelete(file);
	}

	TEST(YyzlibFileUtilTest, AppendTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\append.bin";

		std::vector<uint8_t> d1 = { 1, 2 };
		std::vector<uint8_t> d2 = { 3, 4 };

		EXPECT_TRUE(AppendFileBinary(file, d1));
		EXPECT_TRUE(AppendFileBinary(file, d2));
		EXPECT_EQ(LoadFileBinary(file), (std::vector<uint8_t>{ 1, 2, 3, 4 }));

		EXPECT_TRUE(AppendFileString(file, "abc"));
		EXPECT_TRUE(AppendFileString(file, "def"));

		std::string s((char*)LoadFileBinary(file).data(), 10);
		EXPECT_EQ(s, std::string("\x1\x2\x3\x4" "abcdef", 10));

		// Appending empty data returns true and leaves the file untouched
		std::vector<uint8_t> empty;
		EXPECT_TRUE(AppendFileBinary(file, empty));
		EXPECT_TRUE(AppendFileString(file, ""));
		EXPECT_EQ(GetFileSize(file), 10);

		FileDelete(file);
	}

	TEST(YyzlibFileUtilTest, LoadFileChunkTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\chunk.bin";
		SaveFileBinary(file, { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });

		std::vector<uint8_t> data;

		// normal range
		EXPECT_TRUE(LoadFileChunk(file, 2, 3, data));
		EXPECT_EQ(data, (std::vector<uint8_t>{ 2, 3, 4 }));

		// an out-of-range length is clamped
		EXPECT_TRUE(LoadFileChunk(file, 8, 100, data));
		EXPECT_EQ(data, (std::vector<uint8_t>{ 8, 9 }));

		// an out-of-range offset fails
		EXPECT_FALSE(LoadFileChunk(file, 100, 10, data));

		// a non-existent file fails
		EXPECT_FALSE(LoadFileChunk(g_test_dir + L"\\no_such_file.bin", 0, 10, data));

		FileDelete(file);
	}

	TEST(YyzlibFileUtilTest, FileOpTest)
	{
		DirCreate(g_test_dir);
		tstring src = g_test_dir + L"\\src.txt";
		tstring dest = g_test_dir + L"\\dest.txt";

		SaveFileString(src, "data");

		// copy
		EXPECT_TRUE(FileCopy(src, dest));
		EXPECT_TRUE(FileExists(dest));

		// move
		tstring moved = g_test_dir + L"\\moved.txt";
		EXPECT_TRUE(FileMove(dest, moved));
		EXPECT_FALSE(FileExists(dest));
		EXPECT_TRUE(FileExists(moved));

		// fails when the source does not exist
		EXPECT_FALSE(FileCopy(g_test_dir + L"\\none.txt", dest));
		EXPECT_FALSE(FileMove(g_test_dir + L"\\none.txt", dest));
		EXPECT_FALSE(FileDelete(g_test_dir + L"\\none.txt"));

		FileDelete(src);
		FileDelete(moved);
	}

	TEST(YyzlibFileUtilTest, DirOpTest)
	{
		tstring dir = g_test_dir + L"\\a\\b\\c";

		EXPECT_TRUE(DirCreate(dir));
		EXPECT_TRUE(IsDir(dir));
		EXPECT_FALSE(IsRegularFile(dir));

		// returns false when it already exists (KISS: do not create it twice)
		EXPECT_FALSE(DirCreate(dir));

		EXPECT_TRUE(DirDelete(g_test_dir));
		EXPECT_FALSE(FileExists(g_test_dir));
		EXPECT_FALSE(DirDelete(g_test_dir));	// returns false when it does not exist
	}

	TEST(YyzlibFileUtilTest, ListTest)
	{
		DirCreate(g_test_dir);
		SaveFileString(g_test_dir + L"\\a.txt", "a");
		SaveFileString(g_test_dir + L"\\b.log", "b");
		DirCreate(g_test_dir + L"\\sub");

		// ListDirectory returns file names, not full paths
		TStringList files, dirs;
		EXPECT_TRUE(ListDirectory(g_test_dir, files, dirs));
		EXPECT_EQ(files.size(), (size_t)2);
		EXPECT_EQ(dirs.size(), (size_t)1);
		EXPECT_NE(std::find(files.begin(), files.end(), tstring(L"a.txt")), files.end());
		EXPECT_NE(std::find(dirs.begin(), dirs.end(), tstring(L"sub")), dirs.end());

		// GetFileList returns absolute paths
		TStringList ext_files;
		GetFileList(ext_files, g_test_dir, L".txt");
		EXPECT_EQ(ext_files.size(), (size_t)1);
		EXPECT_NE(ext_files[0].find(L"a.txt"), tstring::npos);

		// GetDirList
		TStringList sub_dirs;
		GetDirList(sub_dirs, g_test_dir);
		EXPECT_EQ(sub_dirs.size(), (size_t)1);

		// non-existent directory
		EXPECT_FALSE(ListDirectory(g_test_dir + L"\\none", files, dirs));

		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, NormalizeAndRelativeTest)
	{
		EXPECT_EQ(NormalizePath(L"C:\\a\\\\b/..\\c.txt", false), L"C:\\a\\c.txt");

		tstring abs = NormalizePath(L"sub\\file.txt", true);
		EXPECT_FALSE(abs.empty());
		EXPECT_NE(abs.find(L":"), tstring::npos);	// absolute paths carry a drive letter

		EXPECT_EQ(GetRelativePath(L"C:\\a\\b", L"C:\\a\\b\\c\\d.txt"), L"c\\d.txt");
		EXPECT_EQ(GetRelativePath(L"C:\\a\\b", L"C:\\a\\b"), L".");
	}

	TEST(YyzlibFileUtilTest, TypeJudgeTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\type.txt";
		SaveFileString(file, "x");

		EXPECT_TRUE(IsRegularFile(file));
		EXPECT_FALSE(IsDir(file));
		EXPECT_TRUE(IsDir(g_test_dir));
		EXPECT_FALSE(IsRegularFile(g_test_dir));

		// non-existent path: both return false
		tstring none = g_test_dir + L"\\none.txt";
		EXPECT_FALSE(IsRegularFile(none));
		EXPECT_FALSE(IsDir(none));
		EXPECT_FALSE(FileExists(none));

		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, LoadFailTest)
	{
		DirCreate(g_test_dir);
		tstring none = g_test_dir + L"\\none.txt";

		// non-existent file: read returns empty and size returns -1
		EXPECT_TRUE(LoadFileString(none).empty());
		EXPECT_TRUE(LoadFileBinary(none).empty());
		EXPECT_EQ(GetFileSize(none), (int64_t)-1);

		// reading a directory as a file: size returns 0, not -1
		EXPECT_EQ(GetFileSize(g_test_dir), (int64_t)0);

		// saving fails when the parent directory does not exist (no implicit creation)
		EXPECT_FALSE(SaveFileString(g_test_dir + L"\\no_dir\\f.txt", "x"));

		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, CopyOverwriteTest)
	{
		DirCreate(g_test_dir);
		tstring src = g_test_dir + L"\\src.txt";
		tstring dest = g_test_dir + L"\\dest.txt";

		SaveFileString(src, "new");
		SaveFileString(dest, "old");

		// overwrite an existing target
		EXPECT_TRUE(FileCopy(src, dest));
		EXPECT_EQ(LoadFileString(dest), "new\n");

		FileDelete(src);
		FileDelete(dest);
		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, RecycleBinTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\recycle.txt";
		SaveFileString(file, "x");

		EXPECT_TRUE(DeleteFileToRecycleBin(file));
		EXPECT_FALSE(FileExists(file));

		// a non-existent file fails
		EXPECT_FALSE(DeleteFileToRecycleBin(g_test_dir + L"\\none.txt"));

		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, HideFileTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\hide.txt";
		SaveFileString(file, "x");

		EXPECT_TRUE(HideFile(file));
		DWORD attr = GetFileAttributesW(file.c_str());
		EXPECT_TRUE(attr & FILE_ATTRIBUTE_HIDDEN);

		// hiding twice is idempotent
		EXPECT_TRUE(HideFile(file));

		// a non-existent file fails
		EXPECT_FALSE(HideFile(g_test_dir + L"\\none.txt"));

		DirDelete(g_test_dir);
	}

	// ---- MappedFileView (read-only memory mapping, RAII) ----

	TEST(YyzlibFileUtilTest, MappedFileViewOpenTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\map.bin";
		std::vector<uint8_t> data = { 0x10, 0x20, 0x30, 0x40, 0x50 };
		SaveFileBinary(file, data);

		MappedFileView view;
		EXPECT_FALSE(view.IsOpen());
		EXPECT_EQ(view.Size(), (size_t)0);
		EXPECT_EQ(view.View(), nullptr);

		EXPECT_TRUE(view.Open(file));
		EXPECT_TRUE(view.IsOpen());
		EXPECT_EQ(view.Size(), data.size());
		ASSERT_NE(view.View(), nullptr);
		EXPECT_EQ(0, memcmp(view.View(), data.data(), data.size()));
		EXPECT_EQ(view.LastError(), 0u);

		FileDelete(file);
		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, MappedFileViewFailTest)
	{
		MappedFileView view;

		// Create the directory first so that the "file not found" error code stays deterministic (a missing directory would report ERROR_PATH_NOT_FOUND)
		DirCreate(g_test_dir);

		// non-existent file: CreateFile fails
		EXPECT_FALSE(view.Open(g_test_dir + L"\\no_such_map.bin"));
		EXPECT_FALSE(view.IsOpen());
		EXPECT_EQ(view.LastError(), ERROR_FILE_NOT_FOUND);

		// opening a directory as a file: GENERIC_READ on a directory fails
		EXPECT_FALSE(view.Open(g_test_dir));
		EXPECT_FALSE(view.IsOpen());
		EXPECT_NE(view.LastError(), 0u);

		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, MappedFileViewExpectedSizeTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\map_size.bin";
		SaveFileBinary(file, { 1, 2, 3, 4 });

		MappedFileView view;

		// expectedSize matches: success
		EXPECT_TRUE(view.Open(file, 4));
		EXPECT_TRUE(view.IsOpen());
		view.Release();

		// expectedSize does not match: fails with ERROR_FILE_INVALID (TOCTOU guard)
		EXPECT_FALSE(view.Open(file, 5));
		EXPECT_FALSE(view.IsOpen());
		EXPECT_EQ(view.LastError(), ERROR_FILE_INVALID);

		// expectedSize < 0 disables the size check
		EXPECT_TRUE(view.Open(file, -1));
		EXPECT_TRUE(view.IsOpen());

		FileDelete(file);
		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, MappedFileViewReleaseTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\map_rel.bin";
		SaveFileBinary(file, { 9, 8, 7 });

		MappedFileView view;
		EXPECT_TRUE(view.Open(file));

		view.Release();
		EXPECT_FALSE(view.IsOpen());
		EXPECT_EQ(view.Size(), (size_t)0);
		EXPECT_EQ(view.View(), nullptr);

		// releasing twice is idempotent
		EXPECT_NO_THROW(view.Release());

		// Open can be called again after Release
		EXPECT_TRUE(view.Open(file));
		EXPECT_TRUE(view.IsOpen());

		FileDelete(file);
		DirDelete(g_test_dir);
	}

	TEST(YyzlibFileUtilTest, MappedFileViewMoveTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\map_move.bin";
		std::vector<uint8_t> data = { 'a', 'b', 'c', 'd' };
		SaveFileBinary(file, data);

		// move construction: ownership is transferred, the source no longer holds it
		MappedFileView v1;
		EXPECT_TRUE(v1.Open(file));
		MappedFileView v2(std::move(v1));
		EXPECT_TRUE(v2.IsOpen());
		EXPECT_EQ(v2.Size(), data.size());
		EXPECT_EQ(0, memcmp(v2.View(), data.data(), data.size()));
		EXPECT_FALSE(v1.IsOpen());
		EXPECT_EQ(v1.View(), nullptr);

		// move assignment: the target releases its old resource before taking over
		MappedFileView v3;
		EXPECT_TRUE(v3.Open(file, 4));
		v3 = std::move(v2);
		EXPECT_TRUE(v3.IsOpen());
		EXPECT_EQ(v3.Size(), data.size());
		EXPECT_FALSE(v2.IsOpen());

		// self-assignment guard (built indirectly through a reference, so the if branch is not taken either)
		MappedFileView& ref = v3;
		v3 = std::move(ref);
		EXPECT_TRUE(v3.IsOpen());

		FileDelete(file);
		DirDelete(g_test_dir);
	}

	// The destructor releases automatically (no leak and no crash on either the normal or the exception path is enough)
	TEST(YyzlibFileUtilTest, MappedFileViewDestructorTest)
	{
		DirCreate(g_test_dir);
		tstring file = g_test_dir + L"\\map_dtor.bin";
		SaveFileBinary(file, { 1 });

		{
			MappedFileView view;
			EXPECT_TRUE(view.Open(file));
		}	// the destructor runs when the scope ends

		// The destructor has released the handle and the view (no leak, no crash) and the file can be deleted
		EXPECT_TRUE(FileDelete(file));
		DirDelete(g_test_dir);
	}
}

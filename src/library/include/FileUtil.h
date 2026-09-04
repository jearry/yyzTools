/*****************************************************************************
*  File utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_FILE_UTIL_H__
#define __XD_FILE_UTIL_H__

#include <cstdint>

#include "TypeDefs.h"

namespace yyzlib
{

	void GetFileList(TStringList &files, const tstring & file_path, const tstring & ext);
	void GetDirList(TStringList &dirs, const tstring & file_path);

	bool ListDirectory(const tstring& dirpath, TStringList& files, TStringList& directories);

	std::string LoadFileString(const tstring &filename);
	bool SaveFileString(const tstring &filename, const std::string &str);

	std::vector<std::uint8_t> LoadFileBinary(const tstring &filename);
	bool SaveFileBinary(const tstring &filename, const std::vector<std::uint8_t> &data);

	bool HideFile(const tstring& filename);

	tstring GetFileDirectory(const tstring& file_full_path);

	tstring GetFileName(const tstring& file_full_path);
	tstring GetStemName(const tstring& file_full_path);
	tstring GetFileExtension(const tstring& file_full_path);

	bool FileExists(const tstring& file_full_path);

	bool IsRegularFile(const tstring& file_full_path);
	bool IsDir(const tstring& file_full_path);

	bool FileDelete(const tstring& file_full_path);

	bool FileMove(const tstring& src_file_full_path, const tstring& dest_file_full_path);

	bool FileCopy(const tstring& src_file_full_path, const tstring& dest_file_full_path);

	bool DirCreate(const tstring& destdir);
	bool DirDelete(const tstring& destdir);

	tstring NormalizePath(const tstring& raw_path, bool asbolute);

	// Additional file operations
	int64_t GetFileSize(const tstring& filepath);

	bool DeleteFileToRecycleBin(const tstring& filepath);
	
	tstring GetRelativePath(const tstring& basepath, const tstring& filepath);

	bool LoadFileChunk(const tstring& filepath, int64_t offset, int64_t length, std::vector<uint8_t>& data);
	bool AppendFileBinary(const tstring& filepath, const std::vector<uint8_t>& data);
	bool AppendFileString(const tstring& filepath, const std::string& data);

	//Read-only memory-mapped file view (RAII): holds the file handle + view;
	//destructor/Release frees both (UnmapViewOfFile + CloseHandle). The mapping
	//handle is closed right after the view is created (the view remains valid).
	//When expectedSize >= 0, a size sanity check is performed — the file may be
	//atomically replaced between open and map (TOCTOU). On failure, LastError()
	//returns the GetLastError from the first failing call (CloseHandle would
	//clobber gle, so it is captured first internally)
	class MappedFileView
	{
	public:
		MappedFileView() = default;
		~MappedFileView();
		MappedFileView(const MappedFileView&) = delete;
		MappedFileView& operator=(const MappedFileView&) = delete;
		MappedFileView(MappedFileView&& other) noexcept;
		MappedFileView& operator=(MappedFileView&& other) noexcept;

		bool Open(const tstring& file, int64_t expectedSize = -1);
		void Release();

		void* View() const { return m_view; }
		size_t Size() const { return m_size; }
		bool IsOpen() const { return m_view != nullptr; }
		DWORD LastError() const { return m_gle; }

	private:
		HANDLE m_hFile = INVALID_HANDLE_VALUE;
		void* m_view = nullptr;
		size_t m_size = 0;
		DWORD m_gle = 0;
	};
}

#endif


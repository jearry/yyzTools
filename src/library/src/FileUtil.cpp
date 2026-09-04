/*****************************************************************************
*  File system utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "FileUtil.h"
#include "Text.h"
#include "RunLog.h"
#include <shlobj.h>
#include <shellapi.h>

namespace yyzlib
{

	void GetFileList(TStringList &files, const tstring & file_path, const tstring & ext)
	{
		if (IsDir(file_path)) { // Must be a directory: FileExists also matches regular files, and constructing a directory_iterator would throw

			std::filesystem::directory_iterator end_iter;
			std::filesystem::directory_iterator itr(file_path);

			for (; itr != end_iter; ++itr) {
				if (IsRegularFile(itr->path().wstring())) {
					if (StringUtil::iequals(itr->path().extension().wstring(), ext)) {
						files.push_back(std::filesystem::absolute(itr->path()).wstring());
					}
				}
			}
		}
	}

	void GetDirList(TStringList &dirs, const tstring & file_path)
	{
		if (IsDir(file_path)) { // Same as GetFileList: must be a directory

			std::filesystem::directory_iterator end_iter;
			std::filesystem::directory_iterator itr(file_path);

			for (; itr != end_iter; ++itr) {
				if (IsDir(itr->path().wstring())) {
					dirs.push_back(std::filesystem::absolute(itr->path()).wstring());
				}
			}
		}
	}

	bool ListDirectory(const tstring& dirpath, TStringList& files, TStringList& directories)
	{
		files.clear();
		directories.clear();

		if (!FileExists(dirpath) || !IsDir(dirpath)) {
			return false;
		}

		try {
			std::filesystem::directory_iterator end_iter;
			std::filesystem::directory_iterator iter(dirpath);

			for (; iter != end_iter; ++iter) {
				if (IsRegularFile(iter->path().wstring())) {
					files.push_back(iter->path().filename().wstring());
				} else if (IsDir(iter->path().wstring())) {
					directories.push_back(iter->path().filename().wstring());
				}
			}
			return true;
		} catch (const std::exception&) {
			return false;
		}
	}

	std::string LoadFileString(const tstring &filename)
	{
		std::string ret;
		std::ifstream is(filename);
		std::string line;

		while (getline(is, line)) {
			ret += line + "\n";
		}

		return ret;
	}

	std::vector<std::uint8_t> LoadFileBinary(const tstring &filename)
	{
		std::vector<std::uint8_t> ret;
		
		do {

			std::error_code ig;
			if (!FileExists(filename)) {
				break;
			}

			size_t size = (size_t)std::filesystem::file_size(filename, ig);

			ret.resize(size);

			FILE* fp = NULL;
			errno_t err = _tfopen_s(&fp, filename.c_str(), _T("rb"));

			if (err != 0 || fp == NULL) {
				break;
			}
			fread(&ret[0], 1, size, fp);
			fclose(fp);
		} while (0);

		return ret;	
	}

	bool SaveFileBinary(const tstring &filename, const std::vector<std::uint8_t> &data)
	{
		bool ret = false;
		do {
			FILE* fp = NULL;
			errno_t err = _tfopen_s(&fp, filename.c_str(), _T("wb"));
			if (err != 0 || fp == NULL) {
				break;
			}
			fwrite(&data[0], 1, data.size(), fp);
			fclose(fp);

			ret = true;
		} while (0);

		return ret;
	}


	bool SaveFileString(const tstring &filename, const std::string &str)
	{
		bool ret = false;
		try {
			std::ofstream os(filename);
			if (os.is_open()) {
				os << str;

				ret = os.good();
			}
		}catch(std::exception &e) {
			ErrorMsg("SaveFileString error, %s", Text::AcpToUtf8(e.what()).c_str());
		}

		return ret;
	}
	

	bool HideFile(const tstring & filename)
	{
		DWORD dwResult = ::GetFileAttributes(filename.c_str());
		if (INVALID_FILE_ATTRIBUTES == dwResult) {
			return false;
		}
		if (!(FILE_ATTRIBUTE_HIDDEN & dwResult)) {
			if (INVALID_FILE_ATTRIBUTES == ::SetFileAttributes(filename.c_str(), dwResult | FILE_ATTRIBUTE_HIDDEN)) {
				return false;
			}
			return true;
		} else {
			return true;
		}
	}

	tstring GetFileDirectory(const tstring& file_full_path)
	{
		std::filesystem::path file_path(file_full_path);

		std::filesystem::path parent_dir = file_path.parent_path();

		return parent_dir.wstring();
	}

	tstring GetFileName(const tstring& file_full_path)
	{
		std::filesystem::path file_path(file_full_path);

		return file_path.filename().wstring();
	}

	tstring GetStemName(const tstring& file_full_path)
	{
		std::filesystem::path file_path(file_full_path);

		return file_path.stem().wstring();
	}

	tstring GetFileExtension(const tstring& file_full_path)
	{
		std::filesystem::path file_path(file_full_path);

		return file_path.extension().wstring();
	}

	bool FileExists(const tstring& file_full_path)
	{
		std::error_code ig;
		return std::filesystem::exists(file_full_path, ig);
	}

	bool IsDir(const tstring& file_full_path)
	{
		std::error_code ig;
		return std::filesystem::is_directory(file_full_path, ig);
	}

	bool IsRegularFile(const tstring& file_full_path)
	{
		std::error_code ig;
		return std::filesystem::is_regular_file(file_full_path, ig);
	}

	bool FileDelete(const tstring& file_full_path)
	{
		bool ret = false;
		
		if (FileExists(file_full_path)) {
			std::error_code ig;
			ret = std::filesystem::remove(file_full_path, ig);
		} 
		return ret;
	}

	
	bool FileMove(const tstring& src_file_full_path, const tstring& dest_file_full_path)
	{
		bool ret = false;
		
		if (FileExists(src_file_full_path)) {
			std::error_code ig;
			std::filesystem::rename(src_file_full_path, dest_file_full_path, ig);
			ret = !ig;
		} 
		return ret;
	}

	bool FileCopy(const tstring& src_file_full_path, const tstring& dest_file_full_path)
	{
		bool ret = false;
		
		if (FileExists(src_file_full_path)) {
			std::error_code ig;
			std::filesystem::copy_file(src_file_full_path, dest_file_full_path, std::filesystem::copy_options::overwrite_existing, ig);
			ret = !ig;
		} 
		return ret;
	}

	bool DirCreate(const tstring& destdir)
	{
		bool ret = false;
		if (!FileExists(destdir)) {
			std::error_code ig;
			ret = std::filesystem::create_directories(destdir, ig);
		} 
		return ret;
	}

	bool DirDelete(const tstring& destdir)
	{
		bool ret = false;
		
		if (FileExists(destdir)) {
			std::error_code ig;
			ret = std::filesystem::remove_all(destdir, ig) > 0;
		}
		return ret;
	}

	
	tstring NormalizePath(const tstring& raw_path, bool asbolute)
	{
		tstring ret = raw_path;
		try {
			std::filesystem::path p(raw_path);

			std::filesystem::path normalized = p.lexically_normal();

			if (asbolute) {
				normalized = std::filesystem::absolute(normalized);
			}

			ret = normalized.wstring();
		} catch (const std::exception& ) {

		}

		return ret;
	}

	// ========== Additional file operation functions ==========

	int64_t GetFileSize(const tstring& filepath)
	{
		std::error_code ec;
		int64_t size = std::filesystem::file_size(filepath, ec);
		if (ec) {
			return -1;
		}
		return size;
	}

	bool DeleteFileToRecycleBin(const tstring& filePath)
	{
		SHFILEOPSTRUCT fileOp;
		ZeroMemory(&fileOp, sizeof(fileOp));

		//Supports multiple files; a single file entry ends with \0, the whole list ends with \0\0
		std::vector<TCHAR> fp(filePath.begin(), filePath.end());
		fp.push_back(_T('\0'));
		fp.push_back(_T('\0'));

		fileOp.hwnd = NULL; // Can be your window handle
		fileOp.wFunc = FO_DELETE;
		fileOp.pFrom = &fp[0];
		fileOp.pTo = NULL;
		fileOp.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;

		int result = SHFileOperation(&fileOp);

		return (result == 0);
	}


	tstring GetRelativePath(const tstring& basepath, const tstring& filepath)
	{
		try {
			std::filesystem::path base(basepath);
			std::filesystem::path file(filepath);
			return std::filesystem::relative(file, base).wstring();
		} catch (const std::exception&) {
			return filepath;
		}
	}

	bool LoadFileChunk(const tstring& filepath, int64_t offset, int64_t length, std::vector<uint8_t>& data)
	{
		data.clear();

		if (!FileExists(filepath) || !IsRegularFile(filepath)) {
			return false;
		}

		FILE* fp = nullptr;
		errno_t err = _tfopen_s(&fp, filepath.c_str(), _T("rb"));
		if (err != 0 || fp == nullptr) {
			return false;
		}

		// Get the file size
		_fseeki64(fp, 0, SEEK_END);
		int64_t fileSize = _ftelli64(fp);

		// Adjust offset and length
		if (offset < 0) offset = 0;
		if (offset >= fileSize) {
			fclose(fp);
			return false;
		}
		if (length <= 0 || offset + length > fileSize) {
			length = fileSize - offset;
		}

		// Read data at the given offset
		_fseeki64(fp, offset, SEEK_SET);
		data.resize(static_cast<size_t>(length));
		size_t readSize = fread(&data[0], 1, static_cast<size_t>(length), fp);
		fclose(fp);

		if (readSize != static_cast<size_t>(length)) {
			data.resize(readSize);
		}

		return true;
	}

	bool AppendFileBinary(const tstring& filepath, const std::vector<uint8_t>& data)
	{
		if (data.empty()) {
			return true;
		}

		FILE* fp = nullptr;
		errno_t err = _tfopen_s(&fp, filepath.c_str(), _T("ab"));
		if (err != 0 || fp == nullptr) {
			return false;
		}

		size_t writeSize = fwrite(&data[0], 1, data.size(), fp);
		fclose(fp);

		return (writeSize == data.size());
	}

	bool AppendFileString(const tstring& filepath, const std::string& data)
	{
		if (data.empty()) {
			return true;
		}

		try {
			std::ofstream os(filepath, std::ios::app);
			if (!os.is_open()) {
				return false;
			}
			os << data;
			os.close();
			return true;
		} catch (const std::exception&) {
			return false;
		}
	}

	

	// ========== End of additional file operation functions ==========

	MappedFileView::~MappedFileView()
	{
		Release();
	}

	MappedFileView::MappedFileView(MappedFileView&& other) noexcept
		: m_hFile(other.m_hFile), m_view(other.m_view), m_size(other.m_size), m_gle(other.m_gle)
	{
		other.m_hFile = INVALID_HANDLE_VALUE;
		other.m_view = nullptr;
		other.m_size = 0;
		other.m_gle = 0;
	}

	MappedFileView& MappedFileView::operator=(MappedFileView&& other) noexcept
	{
		if (this != &other) {
			Release();
			m_hFile = other.m_hFile;
			m_view = other.m_view;
			m_size = other.m_size;
			m_gle = other.m_gle;
			other.m_hFile = INVALID_HANDLE_VALUE;
			other.m_view = nullptr;
			other.m_size = 0;
			other.m_gle = 0;
		}
		return *this;
	}

	bool MappedFileView::Open(const tstring& file, int64_t expectedSize)
	{
		Release();
		m_gle = 0;
		m_hFile = CreateFileW(file.c_str(), GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (m_hFile == INVALID_HANDLE_VALUE) { m_gle = GetLastError(); return false; }

		LARGE_INTEGER sz = { 0 };
		if (!GetFileSizeEx(m_hFile, &sz)) { m_gle = GetLastError(); Release(); return false; }
		if (expectedSize >= 0 && sz.QuadPart != expectedSize) { m_gle = ERROR_FILE_INVALID; Release(); return false; }
		m_size = (size_t)sz.QuadPart;

		HANDLE hMapping = CreateFileMappingW(m_hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
		if (!hMapping) { m_gle = GetLastError(); Release(); return false; }
		m_view = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
		DWORD mapGle = m_view ? 0 : GetLastError();   // Fetch the gle first; CloseHandle would overwrite it
		CloseHandle(hMapping);   // Once the view exists the mapping handle can be closed; the view stays valid
		if (!m_view) { m_gle = mapGle; Release(); return false; }
		return true;
	}

	void MappedFileView::Release()
	{
		if (m_view) { UnmapViewOfFile(m_view); m_view = nullptr; }
		m_size = 0;
		if (m_hFile != INVALID_HANDLE_VALUE) { CloseHandle(m_hFile); m_hFile = INVALID_HANDLE_VALUE; }
	}

}



/*****************************************************************************
*  RunLog - Logging
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

/*
 * Note: log files are saved in UTF-8 encoding
 */

#ifndef __XD_LOG_H__
#define __XD_LOG_H__

#include <string>

//#if defined(XD_LOG_DLL_EXPORT)
//#define XD_DLL_API __declspec( dllexport )  
//#else
//#define XD_DLL_API __declspec( dllimport )  


//#endif

namespace yyzlib
{
	enum SeverityLevel
	{
		SL_TRACE,
		SL_DEBUG,
		SL_INFO,
		SL_WARNING,
		SL_ERROR,
		SL_FATAL
	};

#define SS_TRACE SL_TRACE
#define SS_DEBUG SL_DEBUG
#define SS_INFO SL_INFO
#define SS_WARNING SL_WARNING
#define SS_ERROR SL_ERROR
#define SS_FATAL SL_FATAL

	class RunLog
	{
	public:
		
		/**
		* Log initialization
		* @param[in] level log filter level
		*/
	 static void Init(SeverityLevel level, const std::wstring &filename);

		//Avoids the error caused by the log object being destroyed first
		//when logging inside another object's destructor
		static void Uninit();
		/**
		* Change the log filter level
		* @param[in] level log filter level
		*/
		static void SetLogLevel(SeverityLevel level);

		/**
		* Formatted log record
		* @param[in] level log filter level
		* @param[in] format log format string
		* @param[in] ... arguments
		*/
		static void __cdecl LogFmt(SeverityLevel level, const char* format, ...);

		/**
		* Formatted log records with fixed severity levels
		* @param[in] format log format string
		* @param[in] ... arguments
		*/
		static void __cdecl Trace(const char* format, ...);
		static void __cdecl Debug(const char* format, ...);
		static void __cdecl Info(const char* format, ...);
		static void __cdecl Warn(const char* format, ...);
		static void __cdecl Error(const char* format, ...);
		static void __cdecl Fatal(const char* format, ...);

	private:
		 
		static void LogString(SeverityLevel level, const std::string &s);
	};
	

	/**
	* Log function aliases (compatible with the old function names)
	* All content is output in UTF-8 encoding
	*/
	#define TraceMsg(format, ...) RunLog::Trace(format, ##   __VA_ARGS__)
	#define DebugMsg(format, ...) RunLog::Debug(format, ##   __VA_ARGS__)
	#define InfoMsg(format, ...) RunLog::Info(format, ##   __VA_ARGS__)
	#define WarnMsg(format, ...) RunLog::Warn(format, ##   __VA_ARGS__)
	#define ErrorMsg(format, ...) RunLog::Error(format, ##   __VA_ARGS__)
	#define FatalMsg(format, ...) RunLog::Fatal(format, ##   __VA_ARGS__)

	
	#define LOG_EVERY_N_VARNAME(base, line) LOG_EVERY_N_VARNAME_CONCAT(base, line)
	#define LOG_EVERY_N_VARNAME_CONCAT(base, line) base ## line

	#define LOG_OCCURRENCES LOG_EVERY_N_VARNAME(occurrences_, __LINE__)
	#define LOG_OCCURRENCES_MOD_N LOG_EVERY_N_VARNAME(occurrences_mod_n_, __LINE__)

	//Log once every n occurrences
	#define TraceMsgEver(n, format, ...) \
	  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
	  ++LOG_OCCURRENCES; \
	  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
	  if (LOG_OCCURRENCES_MOD_N == 1) \
		RunLog::Trace(format, ##   __VA_ARGS__)

	#define DebugMsgEver(n, format, ...) \
	  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
	  ++LOG_OCCURRENCES; \
	  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
	  if (LOG_OCCURRENCES_MOD_N == 1) \
		RunLog::Debug(format, ##   __VA_ARGS__)

	#define InfoMsgEver(n, format, ...) \
	  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
	  ++LOG_OCCURRENCES; \
	  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
	  if (LOG_OCCURRENCES_MOD_N == 1) \
		RunLog::Info(format, ##   __VA_ARGS__)

	#define WarnMsgEver(n, format, ...) \
	  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
	  ++LOG_OCCURRENCES; \
	  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
	  if (LOG_OCCURRENCES_MOD_N == 1) \
		RunLog::Warn(format, ##   __VA_ARGS__)

	#define ErrorMsgEver(n, format, ...) \
	  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
	  ++LOG_OCCURRENCES; \
	  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
	  if (LOG_OCCURRENCES_MOD_N == 1) \
		RunLog::Error(format, ##   __VA_ARGS__)

	#define FatalMsgEver(n, format, ...) \
	  static int LOG_OCCURRENCES = 0, LOG_OCCURRENCES_MOD_N = 0; \
	  ++LOG_OCCURRENCES; \
	  if (++LOG_OCCURRENCES_MOD_N > n) LOG_OCCURRENCES_MOD_N -= n; \
	  if (LOG_OCCURRENCES_MOD_N == 1) \
		RunLog::Fatal(format, ##   __VA_ARGS__)

	//Log only the first n occurrences
	#define TraceMsgFirst(n, format, ...) \
		static int LOG_OCCURRENCES = 0; \
		if (LOG_OCCURRENCES <= n) \
		++LOG_OCCURRENCES; \
		if (LOG_OCCURRENCES <= n) \
		RunLog::Trace(format, ##   __VA_ARGS__)

	#define DebugMsgFirst(n, format, ...) \
	  	static int LOG_OCCURRENCES = 0; \
		if (LOG_OCCURRENCES <= n) \
		++LOG_OCCURRENCES; \
		if (LOG_OCCURRENCES <= n) \
		RunLog::Debug(format, ##   __VA_ARGS__)

	#define InfoMsgFirst(n, format, ...) \
	  	static int LOG_OCCURRENCES = 0; \
		if (LOG_OCCURRENCES <= n) \
		++LOG_OCCURRENCES; \
		if (LOG_OCCURRENCES <= n) \
		RunLog::Info(format, ##   __VA_ARGS__)

	#define WarnMsgFirst(n, format, ...) \
	  	static int LOG_OCCURRENCES = 0; \
		if (LOG_OCCURRENCES <= n) \
		++LOG_OCCURRENCES; \
		if (LOG_OCCURRENCES <= n) \
		RunLog::Warn(format, ##   __VA_ARGS__)

	#define ErrorMsgFirst(n, format, ...) \
	  	static int LOG_OCCURRENCES = 0; \
		if (LOG_OCCURRENCES <= n) \
		++LOG_OCCURRENCES; \
		if (LOG_OCCURRENCES <= n) \
		RunLog::Error(format, ##   __VA_ARGS__)

	#define FatalMsgFirst(n, format, ...) \
		static int LOG_OCCURRENCES = 0; \
		if (LOG_OCCURRENCES <= n) \
		++LOG_OCCURRENCES; \
		if (LOG_OCCURRENCES <= n) \
		RunLog::Fatal(format, ##   __VA_ARGS__)
};

#endif


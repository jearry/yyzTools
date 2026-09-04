/*****************************************************************************
*  DateTimeUtil - Date/time utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __XD_DATE_TIME_UTIL_H__
#define __XD_DATE_TIME_UTIL_H__

#include "TypeDefs.h"
#include "CommonUtil.h"

#include "Text.h"

namespace yyzlib
{
    extern const TCHAR* DEFAULT_DATETIME_FORMAT;
    extern const TCHAR* DEFAULT_DATE_FORMAT;
    extern const TCHAR* DEFAULT_TIME_FORMAT;

    // Basic conversion functions --------------------------------------
    // Convert a Windows FILETIME (UTC) to a Unix time_t (UTC timestamp)
    // Param utcFileTime: Windows file-time structure (UTC)
    // Returns: the corresponding Unix timestamp (UTC, seconds)
    time_t FileTimeToUnixTimeUTC(const FILETIME& utcFileTime);

    // Convenience overload: a file time given as a count of 100-nanosecond
    // units (USN_RECORD.TimeStamp / ULARGE_INTEGER.QuadPart)
    inline time_t FileTimeToUnixTimeUTC(int64_t ft100ns)
    {
        ULARGE_INTEGER uli;
        uli.QuadPart = (ULONGLONG)ft100ns;
        FILETIME ft;
        ft.dwLowDateTime = uli.LowPart;
        ft.dwHighDateTime = uli.HighPart;
        return FileTimeToUnixTimeUTC(ft);
    }

    // Convert a Chrome microsecond timestamp (UTC) to a Windows FILETIME (UTC)
    // Param utcChromeMicroseconds: Chrome timestamp (microseconds since
    // January 1, 1601)
    // Returns: the corresponding Windows file-time structure
    FILETIME ChromeTimeToFileTimeUTC(int64_t utcChromeMicroseconds);

    // Convert a SYSTEMTIME to a FILETIME (with selectable time zone)
    // Param systemTime: system-time structure
    // Param isUTC: whether the input time is UTC (true = UTC, false = local)
    // Returns: the converted file-time structure (UTC)
    FILETIME SystemTimeToFileTimeEx(const SYSTEMTIME& systemTime, bool isUTC);

    // Convert a FILETIME to a SYSTEMTIME (with selectable output time zone)
    // Param utcFileTime: Windows file-time structure (UTC)
    // Param isUTC: whether to output UTC (true = UTC, false = local)
    // Returns: the converted system-time structure
    SYSTEMTIME FileTimeToSystemTimeEx(const FILETIME& utcFileTime, bool isUTC);

    // Formatting output functions -------------------------------------
    // Format a UTC timestamp as a local-time string
    // Param utcTime: Unix timestamp (UTC, seconds)
    // Param format: time format string (default "%Y-%m-%d %H:%M:%S")
    // Returns: the formatted time string (local time zone)
    tstring FormatUTCTimeAsLocalString(time_t utcTime, LPCTSTR format = DEFAULT_DATETIME_FORMAT);

    // Format a FILETIME (UTC) as a local-time string
    // Param utcFileTime: Windows file-time structure (UTC)
    // Param format: time format string
    // Returns: the formatted time string (local time zone)
    tstring FormatFileTimeAsLocalString(const FILETIME& utcFileTime, LPCTSTR format = DEFAULT_DATETIME_FORMAT);


    // Format a SYSTEMTIME as a string (with selectable time zone)
    // Param systemTime: system-time structure
    // Param isUTC: whether the input time is UTC
    // Param format: time format string
    // Returns: the formatted time string
    tstring FormatSystemTimeAsString(const SYSTEMTIME& systemTime, bool isUTC, LPCTSTR format = DEFAULT_DATETIME_FORMAT);


    // Format a Chrome microsecond timestamp (UTC) as a local-time string
    // Param utcChromeMicroseconds: Chrome timestamp (microseconds)
    // Returns: the formatted time string (local time zone)
    tstring FormatChromeTimeAsLocalString(int64_t utcChromeMicroseconds);


    // Common shortcut formatting functions -----------------------------
    // Get a local date string for a UTC time (format: YYYY-MM-DD)
    // Param utcTime: Unix timestamp (UTC, seconds)
    // Returns: the formatted date string
    tstring GetLocalDateFromUTC(time_t utcTime = time(NULL), LPCTSTR format = DEFAULT_DATE_FORMAT);


    // Get a local time string for a UTC time (format: HH:MM:SS)
    // Param utcTime: Unix timestamp (UTC, seconds)
    // Returns: the formatted time string
    tstring GetLocalTimeFromUTC(time_t utcTime = time(NULL), LPCTSTR format = DEFAULT_TIME_FORMAT);

    // Get a local date-time string for a UTC time
    // (format: YYYY-MM-DD HH:MM:SS)
    // Param utcTime: Unix timestamp (UTC, seconds)
    // Returns: the formatted date-time string
    tstring GetLocalDateTimeFromUTC(time_t utcTime = time(NULL), LPCTSTR format = DEFAULT_DATETIME_FORMAT);

    // Format a SYSTEMTIME (local time) as a string
    // Param localSystemTime: local system-time structure
    // Returns: the formatted date-time string
    tstring FormatLocalSystemTimeAsString(const SYSTEMTIME& localSystemTime, LPCTSTR format = DEFAULT_DATETIME_FORMAT);

    // Format a SYSTEMTIME (UTC) as a string
    // Param utcSystemTime: UTC system-time structure
    // Returns: the formatted date-time string
    tstring FormatUTCSystemTimeAsString(const SYSTEMTIME& utcSystemTime, LPCTSTR format = DEFAULT_DATETIME_FORMAT);
}


#endif



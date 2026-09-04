/*****************************************************************************
*  DateTimeUtil - Date & time utilities
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "DateTimeUtil.h"


namespace yyzlib
{
	const TCHAR* DEFAULT_DATETIME_FORMAT = _T("%Y-%m-%d %H:%M:%S");
	const TCHAR* DEFAULT_DATE_FORMAT = _T("%Y-%m-%d");
	const TCHAR* DEFAULT_TIME_FORMAT = _T("%H:%M:%S");
    

    // Basic conversion functions --------------------------------------
    // Converts a Windows FILETIME (UTC) to a Unix time_t (UTC timestamp)
    // Param utcFileTime: Windows file time structure (UTC)
    // Returns: the corresponding Unix timestamp (UTC, in seconds)
    time_t FileTimeToUnixTimeUTC(const FILETIME& utcFileTime)
    {
        ULARGE_INTEGER uli = { 0 };
        uli.LowPart = utcFileTime.dwLowDateTime;
        uli.HighPart = utcFileTime.dwHighDateTime;
        // Number of 100-nanosecond intervals between 1601-01-01 and 1970-01-01
        const ULONGLONG EPOCH_DIFFERENCE = 116444736000000000ULL;
        return static_cast<time_t>((uli.QuadPart - EPOCH_DIFFERENCE) / 10000000ULL);
    }

    // Converts a Chrome timestamp in microseconds (UTC) to a Windows FILETIME (UTC)
    // Param utcChromeMicroseconds: Chrome timestamp (microseconds since 1601-01-01)
    // Returns: the corresponding Windows file time structure
    FILETIME ChromeTimeToFileTimeUTC(int64_t utcChromeMicroseconds)
    {
        ULARGE_INTEGER uli = { 0 };
        uli.QuadPart = utcChromeMicroseconds * 10; // Convert microseconds to 100-nanosecond units
        FILETIME utcFileTime{};
        utcFileTime.dwLowDateTime = uli.LowPart;
        utcFileTime.dwHighDateTime = uli.HighPart;
        return utcFileTime;
    }

    // Converts a SYSTEMTIME to a FILETIME (with timezone support)
    // Param systemTime: system time structure
    // Param isUTC: whether the input time is UTC (true=UTC, false=local time)
    // Returns: the converted file time structure (UTC)
    FILETIME SystemTimeToFileTimeEx(const SYSTEMTIME& systemTime, bool isUTC)
    {
        FILETIME fileTime = {0};
        SystemTimeToFileTime(&systemTime, &fileTime);
        // If the input is local time, convert it to UTC
        if (!isUTC) {
            FILETIME utcFileTime{};
            LocalFileTimeToFileTime(&fileTime, &utcFileTime);
            return utcFileTime;
        }
        return fileTime;
    }

    // Converts a FILETIME to a SYSTEMTIME (with output timezone support)
    // Param utcFileTime: Windows file time structure (UTC)
    // Param isUTC: whether the output should be UTC (true=UTC, false=local)
    // Returns: the converted system time structure
    SYSTEMTIME FileTimeToSystemTimeEx(const FILETIME& utcFileTime, bool isUTC)
    {
        FILETIME targetFileTime = utcFileTime;
        if (!isUTC) {
            FILETIME localFileTime{};
            FileTimeToLocalFileTime(&utcFileTime, &localFileTime);
            targetFileTime = localFileTime;
        }
        SYSTEMTIME systemTime{};
        FileTimeToSystemTime(&targetFileTime, &systemTime);
        return systemTime;
    }

    // Formatting helpers -------------------------------------------------
    // Formats a UTC timestamp as a local time string
    // Param utcTime: Unix timestamp (UTC, in seconds)
    // Param format: time format string (defaults to "%Y-%m-%d %H:%M:%S")
    // Returns: the formatted time string (local time zone)
    tstring FormatUTCTimeAsLocalString(time_t utcTime, LPCTSTR format)
    {
        TCHAR buf[128] = {0};
        tm localTm{};
        // Safely converts a UTC timestamp to a local tm structure
        if (localtime_s(&localTm, &utcTime) == 0) {
            _tcsftime(buf, _countof(buf), format, &localTm);
        }
        return buf;
    }

    // Formats a FILETIME (UTC) as a local time string
    // Param utcFileTime: Windows file time structure (UTC)
    // Param format: time format string
    // Returns: the formatted time string (local time zone)
    tstring FormatFileTimeAsLocalString(const FILETIME& utcFileTime, LPCTSTR format)
    {
        time_t utcTime = FileTimeToUnixTimeUTC(utcFileTime);
        return FormatUTCTimeAsLocalString(utcTime, format);
    }


    // Formats a SYSTEMTIME as a string (with a selectable time zone)
    // Param systemTime: system time structure
    // Param isUTC: whether the input time is UTC
    // Param format: time format string
    // Returns: the formatted time string
    tstring FormatSystemTimeAsString(const SYSTEMTIME& systemTime, bool isUTC, LPCTSTR format)
    {
        FILETIME utcFileTime = SystemTimeToFileTimeEx(systemTime, isUTC);
        return FormatFileTimeAsLocalString(utcFileTime, format);
    }


    // Formats a Chrome timestamp in microseconds (UTC) as a local time string
    // Param utcChromeMicroseconds: Chrome timestamp (in microseconds)
    // Returns: the formatted time string (local time zone)
    tstring FormatChromeTimeAsLocalString(int64_t utcChromeMicroseconds)
    {
        FILETIME utcFileTime = ChromeTimeToFileTimeUTC(utcChromeMicroseconds);
        return FormatFileTimeAsLocalString(utcFileTime);
    }


    // Common shortcut formatters ---------------------------------------------
    // Returns the local date string of a UTC timestamp (format: YYYY-MM-DD)
    // Param utcTime: Unix timestamp (UTC, in seconds)
    // Returns: the formatted date string
    tstring GetLocalDateFromUTC(time_t utcTime, LPCTSTR format)
    {
        return FormatUTCTimeAsLocalString(utcTime, format);
    }


    // Returns the local time string of a UTC timestamp (format: HH:MM:SS)
    // Param utcTime: Unix timestamp (UTC, in seconds)
    // Returns: the formatted time string
    tstring GetLocalTimeFromUTC(time_t utcTime, LPCTSTR format)
    {
        return FormatUTCTimeAsLocalString(utcTime, format);
    }


    // Returns the local date and time string of a UTC timestamp (format: YYYY-MM-DD HH:MM:SS)
    // Param utcTime: Unix timestamp (UTC, in seconds)
    // Returns: the formatted date and time string
    tstring GetLocalDateTimeFromUTC(time_t utcTime, LPCTSTR format)
    {
        return FormatUTCTimeAsLocalString(utcTime, format);
    }


    // Formats a SYSTEMTIME (local time) as a string
    // Param localSystemTime: system time structure in local time
    // Returns: the formatted date and time string
    tstring FormatLocalSystemTimeAsString(const SYSTEMTIME& localSystemTime, LPCTSTR format)
    {
        return FormatSystemTimeAsString(localSystemTime, false, format);
    }


    // Formats a SYSTEMTIME (UTC) as a string
    // Param utcSystemTime: system time structure in UTC
    // Returns: the formatted date and time string
    tstring FormatUTCSystemTimeAsString(const SYSTEMTIME& utcSystemTime, LPCTSTR format)
    {
        return FormatSystemTimeAsString(utcSystemTime, true, format);
    }
}



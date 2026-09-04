/*****************************************************************************
*  Precompiled header
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __LIBRARY_STDINC_H__
#define __LIBRARY_STDINC_H__

//std c
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <stdint.h>
#include <malloc.h>
#include <memory.h>
#include <time.h>
#include <assert.h>
#define _USE_MATH_DEFINES
#include <math.h>

//std c++
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <utility>
#include <tuple>
#include <memory>
#include <locale>
#include <future>
#include <algorithm>
#include <functional>
#include <atomic>
#include <mutex>
#include <shared_mutex>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <format>
#include <optional>
#include <variant>
#include <fstream>

//win
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <sdkddkver.h>
#include <windows.h>
#include <dwmapi.h>
#include <psapi.h>
#include <VersionHelpers.h>
#include <WinUser.h>
#include <ShellScalingApi.h>
#include <ShellAPI.h>
#include <Shlobj.h>
#include <SetupApi.h>
#include <cfgmgr32.h>
#include <Iphlpapi.h>
#include <tlhelp32.h>
#include <Winver.h>
#include <wininet.h>
#include <lm.h>
#include <objbase.h>
#include <atlcomcli.h>

#include <initguid.h>
#include <conio.h>
#include <commctrl.h>
#include <DbgHelp.h>
#include <Devicetopology.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <shobjidl.h>
#include <ShlGuid.h>
#include <gdiplus.h>


#include <WinSock2.h>

#include <wrl.h>
#include <wrl/client.h>

//Shared core utilities
#include "StringUtil.h"
#include "Ptree.h"



#endif

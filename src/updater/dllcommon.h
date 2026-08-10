/*****************************************************************************
*  资源通用处理
*  Copyright (C) 2026 广州云蜗智能科技有限公司
*****************************************************************************/

#ifndef __DLLCOMMON_H_VERSION__
#define __DLLCOMMON_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif


#define _ANSISTRING(text) #text
#define ANSISTRING(text) _ANSISTRING(text)

#define _WIDESTRING(text) L##text
#define WIDESTRING(text) _WIDESTRING(text)

#define PRESET_UNICODE_STRING(symbol, buffer) \
        UNICODE_STRING symbol = \
            { \
            sizeof(WIDESTRING(buffer)) - sizeof(WCHAR), \
            sizeof(WIDESTRING(buffer)), \
            WIDESTRING(buffer) \
            };

#define CREATE_XVER(maj,min,small,build) maj ## , ## min ## , ## small ##, ## build
#define CREATE_FVER(maj,min,small,build) maj ## . ## min ## . ## small ##. ## build
#define CREATE_PVER(maj,min,small) maj ## . ## min ## . ## small

#if DBG
    #ifdef ADVANCED_DEBUG
    #define DebugPrint DbgPrint("[%s] %s (line: %d)\n", __##FILE##__, __##FUNCTION##__, __##LINE##__); DbgPrint
    #else
    #define DebugPrint DbgPrint
    #endif
#else
#define DebugPrint /##/DbgPrint
#endif

#endif // __DLLCOMMON_H_VERSION__

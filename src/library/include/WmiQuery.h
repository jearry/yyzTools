/*****************************************************************************
*  WMI queries
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#pragma once

#include <Wbemidl.h>

#include <comdef.h>

namespace yyzlib
{
    class WmiQuery
    {
    public:
        explicit WmiQuery();
        ~WmiQuery();

        std::wstring GetCmdLine(DWORD pid);
    private:
        IWbemLocator* m_pLoc = nullptr;
		IWbemServices* m_pSvc = nullptr;
    };
}


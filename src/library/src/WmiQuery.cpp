/*****************************************************************************
*  WMI queries
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "stdinc.h"
#include "WmiQuery.h"

#include <comdef.h>
#include <Wbemidl.h>


namespace yyzlib
{

    WmiQuery::WmiQuery()
    {
        HRESULT hres;
        std::wstring commandLine;

        hres = CoCreateInstance(
            CLSID_WbemLocator,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            (LPVOID*)&m_pLoc
        );
        if (SUCCEEDED(hres)) {
            hres = m_pLoc->ConnectServer(
                _bstr_t(L"ROOT\\CIMV2"),
                NULL,
                NULL,
                0, NULL, 0, 0, &m_pSvc
            );
            if (SUCCEEDED(hres)) {
                hres = CoSetProxyBlanket(
                    m_pSvc,
                    RPC_C_AUTHN_WINNT,
                    RPC_C_AUTHZ_NONE,
                    NULL,
                    RPC_C_AUTHN_LEVEL_CALL,
                    RPC_C_IMP_LEVEL_IMPERSONATE,
                    NULL,
                    EOAC_NONE
                );
            }
            
        }
    }


    WmiQuery::~WmiQuery()
    {
        if (m_pSvc) {
            m_pSvc->Release();
        }

        if (m_pLoc) {
            m_pLoc->Release();
        }
    }

    std::wstring WmiQuery::GetCmdLine(DWORD pid)
    {
        HRESULT hres;
        std::wstring commandLine;

        IEnumWbemClassObject* pEnumerator = NULL;
        if (m_pSvc == NULL) { // Guard against a null-pointer crash when ConnectServer/CoCreateInstance failed
            return commandLine;
        }
        hres = m_pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t(std::string("SELECT CommandLine FROM Win32_Process WHERE ProcessId = " + std::to_string(pid)).c_str()),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator
        );
        if (SUCCEEDED(hres)) {
            IWbemClassObject* pclsObj;
            ULONG uReturn = 0;
            if (pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn) == S_OK && uReturn > 0) {
                VARIANT vtProp;
                hres = pclsObj->Get(L"CommandLine", 0, &vtProp, 0, 0);
                if (SUCCEEDED(hres) && vtProp.vt == VT_BSTR) {
                    commandLine = vtProp.bstrVal;
                }
                VariantClear(&vtProp);
                pclsObj->Release();
            }
            pEnumerator->Release();
        }
        return commandLine;
    }
}


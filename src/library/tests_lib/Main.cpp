/*****************************************************************************
*  yyzlib unit test entry point
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"

int main(int argc, char** argv)
{
	// WmiQuery and friends depend on COM; without initialization the cases raise an access violation
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	::testing::InitGoogleTest(&argc, argv);
	int ret = RUN_ALL_TESTS();
	if (SUCCEEDED(hr))
		CoUninitialize();
	return ret;
}

/*****************************************************************************
*  Resource definitions
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __DLLVERSION_H_VERSION__
#define __DLLVERSION_H_VERSION__ 100

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include "..\public\buildnumber.h"

#define PRD_MAJVER             _VERSION_MAJ
#define PRD_MINVER             _VERSION_MIN
#define PRD_SMALL              _VERSION_SMALL
#define PRD_BUILD              _VERSION_BUILD
#define FILE_MAJVER            _VERSION_MAJ
#define FILE_MINVER            _VERSION_MIN
#define FILE_SMALL             _VERSION_SMALL
#define FILE_BUILD             _VERSION_BUILD
#define DRV_YEAR               _VERSION_YEAR
#define TEXT_WEBSITE           www.yyztools.com
#define TEXT_PRODUCTNAME       yyzInputHint
#define TEXT_FILEDESC          yyzTools yyzInputHint
#define TEXT_COMPANY           www.yyztools.com
#define TEXT_MODULE            yyzInputHint.exe
#define TEXT_COPYRIGHT         Copyright (C) DRV_YEAR TEXT_COMPANY
#define TEXT_INTERNALNAME      yyzInputHint.exe

#endif // __DLLVERSION_H_VERSION__

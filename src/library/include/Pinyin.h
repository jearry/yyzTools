/*****************************************************************************
*  Pinyin
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#ifndef __YYZLIB_PINYIN_H__
#define __YYZLIB_PINYIN_H__

#include <stdexcept>
#include <vector>
#include <string>

namespace yyzlib
{
    typedef short Int16;
    typedef int Int32;
    typedef long long int Int64;
    class Pinyin
    {
        /// <summary>
        /// The minimum integer value of a valid Chinese character.
        /// The maximum integer value of a valid Chinese character.
        /// These three ranges: [12295, 12295], [19968, 40869] and [59418, 59491]
        /// </summary>
        private:
            static const Int32 MinValue = 12295;
            static const Int32 Part1MinValue = 19968;
            static const Int32 Part1MaxValue = 40869;
            static const Int32 Part2MinValue = 59418;
            static const Int32 Part2MaxValue = 59491;

            static const char* PinyinTable[]; 
            static const Int64 PinyinCodes[];
            static const Int64 PinyinPart1[];
            static const Int64 PinyinPart2[];

            static const Int64 ling = 175;

            static std::vector<std::string> DecodePinyins(Int64 pinyinCode);

        public:
            static bool IsChinese(wchar_t chr);

            //Input must be a Chinese character: both simplified and traditional are supported
			//Returns all pinyins: polyphonic characters yield multiple pinyins, without tones
			//Example: U+957F -> chang zhang
            static std::vector<std::string> GetPinyins(wchar_t chr);

    };
};

#endif

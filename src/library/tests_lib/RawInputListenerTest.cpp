/*****************************************************************************
*  yyzlib raw input listener - unit tests
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/

#include "pch.h"
#include <gtest/gtest.h>
#include <atomic>
#include "RawInputListener.h"

namespace yyzlib
{
	// Pump messages for a while: the WndProc of RawInputListener is only called from the message loop of the same thread
	static void PumpMessages(DWORD durationMs)
	{
		DWORD start = GetTickCount();
		MSG msg;
		while (GetTickCount() - start < durationMs) {
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			Sleep(10);
		}
	}

	TEST(YyzlibRawInputListenerTest, InitBothTest)
	{
		RawInputListener listener;
		bool ok = listener.Init(GetModuleHandle(nullptr),
			[](DWORD vk, bool pressed) {},
			[](WPARAM msg) {});
		EXPECT_TRUE(ok);
		EXPECT_TRUE(listener.Uninit());
	}

	TEST(YyzlibRawInputListenerTest, InitKeyboardOnlyTest)
	{
		RawInputListener listener;
		bool ok = listener.Init(GetModuleHandle(nullptr),
			[](DWORD vk, bool pressed) {}, nullptr);
		EXPECT_TRUE(ok);
		EXPECT_TRUE(listener.Uninit());
	}

	TEST(YyzlibRawInputListenerTest, InitMouseOnlyTest)
	{
		RawInputListener listener;
		bool ok = listener.Init(GetModuleHandle(nullptr),
			nullptr, [](WPARAM msg) {});
		EXPECT_TRUE(ok);
		EXPECT_TRUE(listener.Uninit());
	}

	TEST(YyzlibRawInputListenerTest, KeyboardCallbackTriggerTest)
	{
		// VK_F13 has no default system behavior, so injecting it has no side effects
		std::atomic<int> downCount = 0;
		std::atomic<int> upCount = 0;
		std::atomic<USHORT> lastVk = 0;

		RawInputListener listener;
		ASSERT_TRUE(listener.Init(GetModuleHandle(nullptr),
			[&](DWORD vk, bool pressed) {
				lastVk = (USHORT)vk;
				pressed ? ++downCount : ++upCount;
			},
			nullptr));

		INPUT input[2] = {};
		input[0].type = INPUT_KEYBOARD;
		input[0].ki.wVk = VK_F13;
		input[0].ki.dwFlags = 0;						// key down
		input[1].type = INPUT_KEYBOARD;
		input[1].ki.wVk = VK_F13;
		input[1].ki.dwFlags = KEYEVENTF_KEYUP;			// key up
		ASSERT_EQ(SendInput(2, input, sizeof(INPUT)), 2u);

		PumpMessages(1000);

		EXPECT_EQ(lastVk.load(), (USHORT)VK_F13);
		EXPECT_GE(downCount.load(), 1);
		EXPECT_GE(upCount.load(), 1);

		EXPECT_TRUE(listener.Uninit());
	}

	TEST(YyzlibRawInputListenerTest, MouseCallbackTriggerTest)
	{
		std::atomic<int> moveCount = 0;

		RawInputListener listener;
		ASSERT_TRUE(listener.Init(GetModuleHandle(nullptr),
			nullptr,
			[&](WPARAM msg) {
				if (msg == WM_MOUSEMOVE) ++moveCount;
			}));

		POINT origin{};
		GetCursorPos(&origin);

		INPUT input[2] = {};
		input[0].type = INPUT_MOUSE;
		input[0].mi.dx = 5;	input[0].mi.dy = 5;
		input[0].mi.dwFlags = MOUSEEVENTF_MOVE;			// relative move
		input[1].type = INPUT_MOUSE;
		input[1].mi.dx = -5;	input[1].mi.dy = -5;
		input[1].mi.dwFlags = MOUSEEVENTF_MOVE;			// move back to the origin
		ASSERT_EQ(SendInput(2, input, sizeof(INPUT)), 2u);

		PumpMessages(1000);

		SetCursorPos(origin.x, origin.y);	// restore the cursor just in case
		EXPECT_GE(moveCount.load(), 1);

		EXPECT_TRUE(listener.Uninit());
	}

	TEST(YyzlibRawInputListenerTest, MouseButtonCallbackTriggerTest)
	{
		std::atomic<int> events = 0;

		RawInputListener listener;
		ASSERT_TRUE(listener.Init(GetModuleHandle(nullptr),
			nullptr,
			[&](WPARAM msg) {
				switch (msg) {
				case WM_LBUTTONDOWN: case WM_LBUTTONUP:
				case WM_RBUTTONDOWN: case WM_RBUTTONUP:
				case WM_MBUTTONDOWN: case WM_MBUTTONUP:
				case WM_MOUSEWHEEL:
					++events;
					break;
				}
			}));

		INPUT input[4] = {};
		input[0].type = INPUT_MOUSE;
		input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
		input[1].type = INPUT_MOUSE;
		input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
		input[2].type = INPUT_MOUSE;
		input[2].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
		input[3].type = INPUT_MOUSE;
		input[3].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
		ASSERT_EQ(SendInput(4, input, sizeof(INPUT)), 4u);

		INPUT wheel[1] = {};
		wheel[0].type = INPUT_MOUSE;
		wheel[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
		wheel[0].mi.mouseData = WHEEL_DELTA;
		ASSERT_EQ(SendInput(1, wheel, sizeof(INPUT)), 1u);

		PumpMessages(1000);
		// Left and right button down plus up, and one wheel event: at least 5 events
		EXPECT_GE(events.load(), 5);

		EXPECT_TRUE(listener.Uninit());
	}
}

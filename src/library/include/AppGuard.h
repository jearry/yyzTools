/*****************************************************************************
*  AppGuard - Companion-tool launch protection (header-only, no extra
*             linking or project changes required)
*  Copyright (c) 2026 yyzTools
*  SPDX-License-Identifier: MIT
*
*  Goal: companion exes under Platform\yyzTools must not run standalone
*  once detached from the host program. Three layers of defense:
*    1) HMAC-SHA256 launch ticket: the issuer (host process / companion
*       tool) sets it via the TICKET_GUARD environment variable before
*       spawning the child; the ticket carries issuer pid + timestamp +
*       nonce and stays valid for 10 seconds. It cannot be forged
*       without the key (embedded in this header, split and XOR-masked).
*    2) Parent process verification: the issuer pid inside the ticket
*       must equal the real parent pid, and the parent image name must
*       be on the whitelist (host process + 8 companion tools) — a
*       direct double-click (parent = explorer) or a forged environment
*       variable (no key) is rejected either way.
*    3) Job watchdog: the host process owns a named KILL_ON_JOB_CLOSE
*       job; companion tools join it after passing verification — when
*       the host exits or crashes, all companion tools terminate.
*       Third-party Platform child processes (aria2c/ffmpeg/openssl
*       etc.) are created suspended at their creation sites, assigned
*       into the job, then resumed.
*
*  Usage:
*    - Host process startup (e.g. PreRun) calls AppGuard::InitHostJob()
*      once;
*    - Call AppGuard::ArmTicket() before every "launch companion tool"
*      call site;
*    - Each of the 8 companion tools calls AppGuard::VerifyOrExit() as
*      the first line of wWinMain;
*    - Third-party Platform child processes are uniformly created via
*      AppGuard::CreateProcessIntoHostJob() (create suspended, assign
*      into the job, then resume);
*    - Debug builds (NDEBUG undefined): VerifyOrExit returns
*      immediately, for standalone debugging.
*
*  @author   Jearry.Zhou
*  @version  1.0.0
*  @date     2026-09-03
*****************************************************************************/
#pragma once

#include <windows.h>
#include <tlhelp32.h>
#include <bcrypt.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <string>
#include <vector>

#include "HashUtil.h"


namespace yyzlib
{

    namespace AppGuard
    {
        // Ticket environment variable name and watchdog job name
        constexpr const wchar_t* kTicketEnv = L"TICKET_GUARD";
        constexpr const wchar_t* kJobName   = L"Local\\yyzTools::AppGuardJob";

        // Ticket validity (seconds): time window from issuance to
        // verification at the child process entry point
        constexpr long kTicketTtlSec = 10;

        // Parent process image name whitelist: host process + all
        // companion tools (case-insensitive comparison)
        inline const wchar_t* const* HostWhiteList()
        {
            static const wchar_t* list[] = {
                L"yyzTools.exe",
                L"yyzBrowser.exe",
                L"yyzCmd.exe",
                L"yyzFileSearch.exe",
                L"yyzInputHint.exe",
                L"yyzMouseFinder.exe",
                L"yyzScreenCap.exe",
                L"yyzScreenRec.exe",
                L"yyzWallpaper.exe",
            };
            return list;
        }
        inline size_t HostWhiteListCount() { return 9; }

        //---------------------------------------------------------------------
        // Embedded HMAC key (32 bytes). PLACEHOLDER VALUES — replace them
        // with your own before production use. The two halves are stored
        // XOR-masked with fixed masks and recombined at runtime, which only
        // raises the bar against casual strings/hex-dump scanning; it is
        // NOT real protection once published. Prefer injecting the key at
        // build time (e.g. from a private header or CI secret) instead of
        // committing real key material here.
        //---------------------------------------------------------------------
        inline const unsigned char* SecretKey()
        {
            static const unsigned char partA[16] = {
                0x36,0x8F,0x27,0x54,0x9C,0x0A,0xD1,0x6B,
                0xE3,0x74,0x18,0xC5,0x2D,0xFA,0x60,0x93
            };
            static const unsigned char maskA[16] = {
                0x7C,0x21,0xB8,0x4E,0xD6,0x9F,0x03,0xA5,
                0x1B,0xE9,0x52,0x77,0xC0,0x36,0x8D,0xF4
            };
            static const unsigned char partB[16] = {
                0x5D,0x0C,0xE1,0x73,0x28,0xBF,0x94,0x6A,
                0x07,0x53,0xCC,0x1F,0xB2,0x8E,0x41,0xD9
            };
            static const unsigned char maskB[16] = {
                0xA9,0x63,0x1D,0xF8,0x40,0x2B,0xE7,0x5C,
                0x90,0xD4,0x6E,0x81,0x07,0x3A,0xC6,0x25
            };
            static unsigned char key[32];
            static bool ready = [] {
                for (int i = 0; i < 16; ++i) {
                    key[i]      = (unsigned char)(partA[i] ^ maskA[i]);
                    key[16 + i] = (unsigned char)(partB[i] ^ maskB[i]);
                }
                return true;
            }();
            (void)ready;
            return key;
        }

        //---------------------------------------------------------------------
        // HMAC-SHA256 (delegates to yyzlib HashUtil; key is the embedded
        // SecretKey)
        //---------------------------------------------------------------------
        inline bool HmacSha256(const char* data, size_t len, unsigned char out[32])
        {
            return yyzlib::HmacSha256(SecretKey(), 32, data, len, out);
        }

        inline void BytesToHex(const unsigned char* p, size_t n, char* out /* n*2+1 */)
        {
            static const char* kHex = "0123456789abcdef";
            for (size_t i = 0; i < n; ++i) {
                out[i * 2]     = kHex[p[i] >> 4];
                out[i * 2 + 1] = kHex[p[i] & 0xF];
            }
            out[n * 2] = '\0';
        }

        // Constant-time comparison (assumes equal lengths), against
        // timing side channels
        inline bool ConstTimeEq(const char* a, const char* b, size_t n)
        {
            unsigned char diff = 0;
            for (size_t i = 0; i < n; ++i) diff |= (unsigned char)(a[i] ^ b[i]);
            return diff == 0;
        }

        //---------------------------------------------------------------------
        // Ticket issuance: v1.<issuer pid>.<unix seconds>.<nonce 8hex>.<hmac 64hex>
        // Signed payload "yyzg|v1|<pid>|<ts>|<nonce>"
        //---------------------------------------------------------------------
        inline bool BuildTicket(unsigned long pid, long long ts, char* out /* >= 128 */)
        {
            unsigned long nonce = 0;
            // Prefer CNG randomness; fall back to rand on failure (the
            // payload already carries pid/ts, so randomness demands are low)
            BCRYPT_ALG_HANDLE alg = nullptr;
            if (BCRYPT_SUCCESS(BCryptOpenAlgorithmProvider(&alg, BCRYPT_RNG_ALGORITHM, nullptr, 0))) {
                BCryptGenRandom(alg, (PUCHAR)&nonce, sizeof(nonce), 0);
                BCryptCloseAlgorithmProvider(alg, 0);
            } else {
                nonce = (unsigned long)(GetTickCount() ^ GetCurrentProcessId());
            }

            char nonceHex[9];
            BytesToHex((const unsigned char*)&nonce, sizeof(nonce), nonceHex);

            char payload[96];
            snprintf(payload, sizeof(payload), "yyzg|v1|%lu|%lld|%s", pid, ts, nonceHex);

            unsigned char mac[32];
            if (!HmacSha256(payload, strlen(payload), mac)) return false;

            char macHex[65];
            BytesToHex(mac, 32, macHex);

            snprintf(out, 128, "v1.%lu.%lld.%s.%s", pid, ts, nonceHex, macHex);
            return true;
        }

        // Current unix seconds (issued and verified on the same machine,
        // so clocks agree)
        inline long long UnixNow()
        {
            FILETIME ft;
            GetSystemTimeAsFileTime(&ft);
            ULARGE_INTEGER u;
            u.LowPart = ft.dwLowDateTime;
            u.HighPart = ft.dwHighDateTime;
            return (long long)((u.QuadPart - 116444736000000000ULL) / 10000000ULL);
        }

        //---------------------------------------------------------------------
        // Issuer API: builds a fresh ticket and writes it into the current
        // process environment. Child processes spawned afterwards via
        // CreateProcess/ShellExecute inherit the variable by default.
        // The previous ticket is naturally invalidated by being overwritten
        // (10s window); no explicit cleanup needed.
        //---------------------------------------------------------------------
        inline bool ArmTicket()
        {
            char ticket[128];
            if (!BuildTicket(GetCurrentProcessId(), UnixNow(), ticket)) return false;

            wchar_t wticket[128];
            MultiByteToWideChar(CP_UTF8, 0, ticket, -1, wticket, 128);
            return SetEnvironmentVariableW(kTicketEnv, wticket) != FALSE;
        }

        //---------------------------------------------------------------------
        // Host process API: creates the watchdog job (KILL_ON_JOB_CLOSE) and
        // holds the handle until exit. Normal exit or crash of the host
        // (all handles closed) terminates every child tool in the job.
        //---------------------------------------------------------------------
        inline bool InitHostJob()
        {
            HANDLE job = CreateJobObjectW(nullptr, kJobName);
            if (!job) return false;

            JOBOBJECT_EXTENDED_LIMIT_INFORMATION eli = {};
            eli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
            if (!SetInformationJobObject(job, JobObjectExtendedLimitInformation,
                    &eli, sizeof(eli))) {
                CloseHandle(job);
                return false;
            }

            // Static hold: never closed for the process lifetime
            static HANDLE s_keep = job;
            (void)s_keep;
            return true;
        }

        //---------------------------------------------------------------------
        // Verification helper: get the parent pid (process snapshot walk,
        // pure Win32)
        //---------------------------------------------------------------------
        inline DWORD GetParentPid(DWORD pid)
        {
            DWORD parent = 0;
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap == INVALID_HANDLE_VALUE) return 0;

            PROCESSENTRY32W pe = {};
            pe.dwSize = sizeof(pe);
            if (Process32FirstW(snap, &pe)) {
                do {
                    if (pe.th32ProcessID == pid) {
                        parent = pe.th32ParentProcessID;
                        break;
                    }
                } while (Process32NextW(snap, &pe));
            }
            CloseHandle(snap);
            return parent;
        }

        // Is the parent process image on the whitelist (yyzTools.exe +
        // 8 companion tools)
        inline bool IsHostImage(DWORD pid)
        {
            HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
            if (!h) return false;

            wchar_t path[MAX_PATH] = {};
            DWORD size = MAX_PATH;
            BOOL ok = QueryFullProcessImageNameW(h, 0, path, &size);
            CloseHandle(h);
            if (!ok) return false;

            const wchar_t* name = wcsrchr(path, L'\\');
            name = name ? name + 1 : path;

            const wchar_t* const* list = HostWhiteList();
            for (size_t i = 0; i < HostWhiteListCount(); ++i) {
                if (_wcsicmp(name, list[i]) == 0) return true;
            }
            return false;
        }

        // Assign the given process into the watchdog job (silently skipped
        // when the job does not exist: keeps unit_tests, standalone
        // debugging and other job-less scenarios working).
        // For processes already running that only need a late assign
        // (JoinHostJob reuses this function); anything that can be created
        // suspended should go through CreateProcessIntoHostJob instead.
        inline void AssignToHostJob(HANDLE hProcess)
        {
            HANDLE job = OpenJobObjectW(JOB_OBJECT_ASSIGN_PROCESS, FALSE, kJobName);
            if (job) {
                AssignProcessToJobObject(job, hProcess);
                CloseHandle(job);
            }
        }

        // Try to join the host process watchdog job (silently skipped when
        // the job does not exist: Debug / standalone debugging scenarios)
        inline void JoinHostJob()
        {
            AssignToHostJob(GetCurrentProcess());
        }

        

        // Unified creation entry for third-party Platform child processes:
        // create with CREATE_SUSPENDED → assign into the watchdog job →
        // resume.
        // Differences from CreateProcessW: appName/process/thread
        // attributes/environment are fixed to nullptr;
        // creationFlags need not contain CREATE_SUSPENDED (appended
        // internally); si is filled in by the caller (pipe stdio / SW_HIDE
        // etc. vary by scenario).
        // Returns false on failure with both pi handles closed; GetLastError
        // retains the failure reason.
        inline bool CreateProcessIntoHostJob(wchar_t* cmdLine, BOOL inheritHandles,
            DWORD creationFlags, const wchar_t* workDir, STARTUPINFOW* si,
            PROCESS_INFORMATION* pi)
        {
            bool ret = false;
            do{
                if (!CreateProcessW(nullptr, cmdLine, nullptr, nullptr, inheritHandles,
                    creationFlags | CREATE_SUSPENDED, nullptr, workDir, si, pi)) {
                    break;
                }

                //Job assignment failure still counts as success (only the
                // cooperative-exit mechanism is lost)
                HANDLE job = OpenJobObjectW(JOB_OBJECT_ASSIGN_PROCESS, FALSE, kJobName);
                if (job) {
                    BOOL assigned = AssignProcessToJobObject(job, pi->hProcess);
                    CloseHandle(job);
                }

                if (ResumeThread(pi->hThread) == (DWORD)-1) {
                    //Main thread still suspended: the process must be
                    // terminated, otherwise a permanently suspended zombie
                    // outside the job's protection is left behind
                    TerminateProcess(pi->hProcess, EXIT_FAILURE);
                    CloseHandle(pi->hThread);
                    CloseHandle(pi->hProcess);
                    pi->hThread = nullptr;
                    pi->hProcess = nullptr;
                    break;
                }

                ret = true;

            }while(0);

            return ret;
        }

        // Application-level launch entry for companion tools / third-party
        // exes (exe + args + show command). Internally goes through
        // CreateProcessIntoHostJob: created suspended, assigned into the
        // watchdog job, then resumed.
        // If wait is true, blocks until the child exits (classic launcher
        // semantics);
        // when phProcess is non-null the process handle is handed over to
        // the caller (otherwise closed internally).
        // Returns the pid, or 0 on failure (GetLastError retains the reason).
        inline DWORD LaunchAppIntoHostJob(const wchar_t* exe, const wchar_t* args,
            const wchar_t* workDir, WORD showCmd, bool wait, HANDLE* phProcess = nullptr,
            DWORD creationFlags = 0)
        {
            std::wstring cmd = L"\"";
            cmd += exe;
            cmd += L"\"";
            if (args && *args) {
                cmd += L" ";
                cmd += args;
            }

            std::vector<wchar_t> cmdLine(cmd.begin(), cmd.end());
            cmdLine.push_back(L'\0');

            STARTUPINFOW si = {};
            si.cb = sizeof(si);
            si.dwFlags = STARTF_USESHOWWINDOW;
            si.wShowWindow = showCmd;

            PROCESS_INFORMATION pi = {};
            if (!CreateProcessIntoHostJob(cmdLine.data(), FALSE, creationFlags, workDir, &si, &pi)) {
                return 0;
            }

            DWORD pid = pi.dwProcessId;
            if (wait) {
                WaitForSingleObject(pi.hProcess, INFINITE);
            }

            CloseHandle(pi.hThread);
            if (phProcess) {
                *phProcess = pi.hProcess;
            } else {
                CloseHandle(pi.hProcess);
            }
            return pid;
        }

        //---------------------------------------------------------------------
        // Child tool entry verification: on failure shows a message box and
        // terminates the process; on success joins the watchdog job.
        // Only effective in Release (NDEBUG); Debug passes through for
        // standalone debugging.
        //---------------------------------------------------------------------
        inline void VerifyOrExit()
        {
#ifndef NDEBUG
            return;
#endif
            wchar_t ticket[160] = {};
            DWORD n = GetEnvironmentVariableW(kTicketEnv, ticket, 160);
            bool pass = (n > 0 && n < 160);

            unsigned long issuerPid = 0;
            long long ts = 0;

            if (pass) {
                // v1.<pid>.<ts>.<nonce>.<sig>: parse field by field and
                // re-verify the signature
                char mb[160] = {};
                WideCharToMultiByte(CP_UTF8, 0, ticket, -1, mb, sizeof(mb), nullptr, nullptr);

                char pidS[16] = {}, tsS[24] = {}, nonceS[16] = {}, sigS[80] = {};
                int filled = sscanf_s(mb, "v1.%15[^.].%23[^.].%15[^.].%79s",
                    pidS, (unsigned)sizeof(pidS), tsS, (unsigned)sizeof(tsS),
                    nonceS, (unsigned)sizeof(nonceS), sigS, (unsigned)sizeof(sigS));
                pass = (filled == 4);

                if (pass) {
                    issuerPid = strtoul(pidS, nullptr, 10);
                    ts = _atoi64(tsS);

                    char payload[96];
                    snprintf(payload, sizeof(payload), "yyzg|v1|%lu|%lld|%s", issuerPid, ts, nonceS);

                    unsigned char mac[32];
                    char macHex[65];
                    if (HmacSha256(payload, strlen(payload), mac)) {
                        BytesToHex(mac, 32, macHex);
                        pass = ConstTimeEq(macHex, sigS, 65);
                    } else {
                        pass = false;
                    }
                }

                // Validity: |now - ts| <= TTL
                if (pass) {
                    long long now = UnixNow();
                    long long delta = now - ts;
                    if (delta < 0) delta = -delta;
                    pass = (delta <= kTicketTtlSec);
                }

                // Issuer must equal the real parent process, and the parent
                // image must be on the whitelist
                if (pass) {
                    DWORD ppid = GetParentPid(GetCurrentProcessId());
                    pass = (ppid != 0) && (ppid == issuerPid) && IsHostImage(ppid);
                }
            }

            if (!pass) {
                #if 0
                wchar_t name[MAX_PATH] = L"yyzTools";
                GetModuleFileNameW(nullptr, name, MAX_PATH);
                const wchar_t* base = wcsrchr(name, L'\\');
                base = base ? base + 1 : name;

                wchar_t msg[256];
                _snwprintf_s(msg, _TRUNCATE,
                    L"%s is a companion component of yyzTools and cannot run independently. \nPlease launch this component using yyzTools. ", base);
                MessageBoxW(nullptr, msg, base, MB_OK | MB_ICONERROR);
                #endif
                ExitProcess(3);
            }

            JoinHostJob();
        }
    }

}

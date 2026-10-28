#include <windows.h>
#include <shellapi.h>
#include <stdio.h>
#include <wchar.h>

static DWORD find_taskmgr_pid(void)
{
    HWND h = FindWindowW(L"TaskManagerWindow", NULL);
    if (h) {
        DWORD pid = 0;
        GetWindowThreadProcessId(h, &pid);
        return pid;
    }
    return 0;
}

static void print_last_error(const wchar_t *what)
{
    DWORD e = GetLastError();
    wprintf(L"%ls failed: error %lu\n", what, e);
}

static BOOL is_elevated(void)
{
    BOOL ret = FALSE;
    HANDLE tok = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &tok)) {
        TOKEN_ELEVATION elev;
        DWORD len = 0;
        if (GetTokenInformation(tok, TokenElevation, &elev, sizeof(elev), &len))
            ret = elev.TokenIsElevated != 0;
        CloseHandle(tok);
    }
    return ret;
}

static BOOL relaunch_elevated(void)
{
    wchar_t exe[MAX_PATH];
    GetModuleFileNameW(NULL, exe, MAX_PATH);

    wchar_t *cmd = GetCommandLineW();
    wchar_t buf[32768];
    swprintf(buf, sizeof(buf) / sizeof(buf[0]), L"%ls", cmd);

    HINSTANCE r = ShellExecuteW(NULL, L"runas", exe, buf, NULL, SW_SHOWNORMAL);
    return ((INT_PTR)r > 32);
}

int wmain(int argc, wchar_t **argv)
{
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    wchar_t *slash = wcsrchr(exePath, L'\\');
    if (slash)
        *(slash + 1) = 0;
    else
        return 1;

    wchar_t dllPath[MAX_PATH];
    if (argc >= 3) {
        wcscpy(dllPath, argv[2]);
    } else {
        wcscpy(dllPath, exePath);
        wcscat(dllPath, L"CpuTempTile.dll");
    }

    if (GetFileAttributesW(dllPath) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"DLL not found: %ls\n", dllPath);
        return 1;
    }

    DWORD pid = 0;
    if (argc >= 2 && argv[1][0] != L'\0')
        pid = (DWORD)_wtoi(argv[1]);
    else
        pid = find_taskmgr_pid();

    if (!pid) {
        wprintf(L"Task Manager not running; launching it...\n");
        STARTUPINFOW si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        if (!CreateProcessW(L"C:\\Windows\\System32\\Taskmgr.exe", NULL,
                            NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            print_last_error(L"CreateProcess");
            return 1;
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);

        for (int i = 0; i < 50 && !pid; i++) {
            Sleep(100);
            pid = find_taskmgr_pid();
        }
    }

    if (!pid) {
        wprintf(L"Could not locate the Task Manager process.\n");
        return 1;
    }

    wprintf(L"Injecting %ls into PID %lu\n", dllPath, pid);

    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                  PROCESS_VM_OPERATION | PROCESS_VM_WRITE |
                                  PROCESS_VM_READ,
                              FALSE, pid);
    if (!proc) {
        DWORD e = GetLastError();
        if (e == ERROR_ACCESS_DENIED && !is_elevated()) {
            wprintf(L"Task Manager is elevated; requesting elevation...\n");
            if (relaunch_elevated()) {
                wprintf(L"Elevated instance launched. Approve the UAC prompt.\n");
                return 0;
            }
            wprintf(L"Elevation was cancelled. Run this tool as Administrator.\n");
            return 1;
        }
        print_last_error(L"OpenProcess");
        return 1;
    }

    SIZE_T sz = (wcslen(dllPath) + 1) * sizeof(wchar_t);
    void *remote = VirtualAllocEx(proc, NULL, sz, MEM_COMMIT | MEM_RESERVE,
                                  PAGE_READWRITE);
    if (!remote) {
        print_last_error(L"VirtualAllocEx");
        CloseHandle(proc);
        return 1;
    }

    if (!WriteProcessMemory(proc, remote, dllPath, sz, NULL)) {
        print_last_error(L"WriteProcessMemory");
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    LPTHREAD_START_ROUTINE loadLib = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
    if (!loadLib) {
        print_last_error(L"GetProcAddress(LoadLibraryW)");
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    HANDLE thr = CreateRemoteThread(proc, NULL, 0, loadLib, remote, 0, NULL);
    if (!thr) {
        print_last_error(L"CreateRemoteThread");
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }

    WaitForSingleObject(thr, 10000);

    DWORD exitCode = 0;
    GetExitCodeThread(thr, &exitCode);

    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(thr);
    CloseHandle(proc);

    wprintf(L"LoadLibraryW returned 0x%lX (%s)\n", exitCode,
            exitCode ? L"success" : L"failed");
    return exitCode ? 0 : 1;
}

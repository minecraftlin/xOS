#include "file_manager.h"
#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include <vector>
#include <string>
#include <filesystem>
#include <dwmapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")

LRESULT CALLBACK FileMgrProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList = nullptr;
    static HWND hDelBtn = nullptr;
    switch (msg) {
    case WM_CREATE: {
        hList = CreateWindowW(WC_LISTBOXW, nullptr, WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            10, 10, 260, 200, hwnd, (HMENU)1, nullptr, nullptr);
        hDelBtn = CreateWindowW(L"BUTTON", L"删除文本文档", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            280, 10, 100, 30, hwnd, (HMENU)2, nullptr, nullptr);
        // 枚举当前目录下所有txt文件
        WIN32_FIND_DATAW fd;
        HANDLE hFind = FindFirstFileW(L"*.txt", &fd);
        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)fd.cFileName);
                }
            } while (FindNextFileW(hFind, &fd));
            FindClose(hFind);
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 1 && HIWORD(wParam) == LBN_DBLCLK) {
            int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                wchar_t buf[MAX_PATH];
                SendMessageW(hList, LB_GETTEXT, sel, (LPARAM)buf);
                ShellExecuteW(hwnd, L"open", buf, nullptr, nullptr, SW_SHOW);
            }
        } else if (LOWORD(wParam) == 2) { // 删除按钮
            int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                wchar_t buf[MAX_PATH];
                SendMessageW(hList, LB_GETTEXT, sel, (LPARAM)buf);
                if (MessageBoxW(hwnd, L"确定要删除该文本文档吗？", buf, MB_OKCANCEL|MB_ICONQUESTION) == IDOK) {
                    DeleteFileW(buf);
                    SendMessageW(hList, LB_DELETESTRING, sel, 0);
                }
            }
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowFileManagerWindow(HWND parent) {
    static const wchar_t CLASS_NAME[] = L"FileManagerWndClass";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = FileMgrProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.hIcon = LoadIconW(GetModuleHandleW(nullptr), L"IDI_ICON_FILEMGR");
        RegisterClassW(&wc);
        registered = true;
    }
    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, CLASS_NAME, L"文件管理器", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    // 高斯模糊
    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE; bb.fEnable = TRUE;
    DwmEnableBlurBehindWindow(hwnd, &bb);
    ShowWindow(hwnd, SW_SHOW);
}

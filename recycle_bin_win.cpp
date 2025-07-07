#include "recycle_bin_win.h"
#include <windows.h>
#include <vector>
#include <string>
#include <shlwapi.h>

struct RecycleFileInfo {
    std::wstring name;
};

static std::vector<RecycleFileInfo> recycleFiles;

void RefreshRecycleFileList() {
    recycleFiles.clear();
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(L"recycle_bin/*.txt", &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            recycleFiles.push_back({fd.cFileName});
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
}

LRESULT CALLBACK RecycleBinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hList = nullptr;
    static HWND hRestoreBtn = nullptr, hDeleteBtn = nullptr;
    switch (msg) {
    case WM_CREATE: {
        hList = CreateWindowW(L"LISTBOX", nullptr, WS_CHILD | WS_VISIBLE | LBS_NOTIFY | WS_VSCROLL,
            10, 10, 260, 200, hwnd, (HMENU)1, nullptr, nullptr);
        hRestoreBtn = CreateWindowW(L"BUTTON", L"恢复", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            280, 10, 80, 30, hwnd, (HMENU)2, nullptr, nullptr);
        hDeleteBtn = CreateWindowW(L"BUTTON", L"彻底删除", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            280, 50, 80, 30, hwnd, (HMENU)3, nullptr, nullptr);
        RefreshRecycleFileList();
        for (const auto& f : recycleFiles) {
            SendMessageW(hList, LB_ADDSTRING, 0, (LPARAM)f.name.c_str());
        }
        break;
    }
    case WM_COMMAND: {
        if (LOWORD(wParam) == 2) { // 恢复
            int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                wchar_t buf[MAX_PATH];
                SendMessageW(hList, LB_GETTEXT, sel, (LPARAM)buf);
                wchar_t srcPath[MAX_PATH], dstPath[MAX_PATH];
                swprintf(srcPath, MAX_PATH, L"recycle_bin/%s", buf);
                swprintf(dstPath, MAX_PATH, L"%s", buf);
                MoveFileW(srcPath, dstPath);
                SendMessageW(hList, LB_DELETESTRING, sel, 0);
            }
        } else if (LOWORD(wParam) == 3) { // 彻底删除
            int sel = (int)SendMessageW(hList, LB_GETCURSEL, 0, 0);
            if (sel != LB_ERR) {
                wchar_t buf[MAX_PATH];
                SendMessageW(hList, LB_GETTEXT, sel, (LPARAM)buf);
                wchar_t srcPath[MAX_PATH];
                swprintf(srcPath, MAX_PATH, L"recycle_bin/%s", buf);
                DeleteFileW(srcPath);
                SendMessageW(hList, LB_DELETESTRING, sel, 0);
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

void ShowRecycleBinWindow(HWND parent) {
    static const wchar_t CLASS_NAME[] = L"RecycleBinWndClass";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = RecycleBinProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        RegisterClassW(&wc);
        registered = true;
    }
    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, CLASS_NAME, L"回收站", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    ShowWindow(hwnd, SW_SHOW);
}

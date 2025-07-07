#include <windows.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <stdio.h>
#include "file_manager.h"
#include "snake.h"
#include "recycle_bin_win.h"
#include <vector>
#include <filesystem>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

// 图标区域坐标
#define ICON_SIZE 64
#define ICON_MARGIN 30
RECT fileMgrIcon = {50, 100, 50+ICON_SIZE, 100+ICON_SIZE};
RECT snakeIcon = {150, 100, 150+ICON_SIZE, 100+ICON_SIZE};
RECT newTxtIcon = {250, 100, 250+ICON_SIZE, 100+ICON_SIZE};
RECT newVSCodeDirIcon = {350, 100, 350+ICON_SIZE, 100+ICON_SIZE};
RECT recycleBinIcon = {450, 100, 450+ICON_SIZE, 100+ICON_SIZE};

// txt文件图标结构体
struct TxtIcon {
    RECT rect;
    wchar_t name[MAX_PATH];
};
static std::vector<TxtIcon> txtIcons;

// 回收站图标结构体
struct RecycleIcon {
    RECT rect;
    wchar_t name[MAX_PATH];
};
static std::vector<RecycleIcon> recycleIcons;

// 判断点是否在图标区域
BOOL IsInRect(RECT r, int x, int y) {
    return x >= r.left && x <= r.right && y >= r.top && y <= r.bottom;
}

// 新建文本文档
void CreateNewTextFile(HWND hwnd) {
    wchar_t path[MAX_PATH] = L"text.txt";
    int idx = 1;
    // 避免重名，自动编号
    while (PathFileExistsW(path)) {
        swprintf(path, MAX_PATH, L"text(%d).txt", idx++);
    }
    FILE* fp = _wfopen(path, L"w, ccs=UTF-8");
    if (fp) {
        fwprintf(fp, L"新建文本文档\n");
        fclose(fp);
        MessageBoxW(hwnd, L"已创建新文本文档！", L"提示", MB_OK);
    } else {
        MessageBoxW(hwnd, L"创建失败！", L"错误", MB_OK|MB_ICONERROR);
    }
}

void RefreshRecycleIcons() {
    recycleIcons.clear();
    int x = 450, y = 200, count = 0;
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(L"recycle_bin/*.txt", &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            RecycleIcon icon;
            icon.rect = {x, y, x+ICON_SIZE, y+ICON_SIZE};
            wcsncpy(icon.name, fd.cFileName, MAX_PATH);
            recycleIcons.push_back(icon);
            x += ICON_SIZE + 20;
            if (++count % 4 == 0) { x = 450; y += ICON_SIZE + 40; }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
}

void RefreshTxtIcons() {
    txtIcons.clear();
    int x = 50, y = 200, count = 0;
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(L"*.txt", &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            TxtIcon icon;
            icon.rect = {x, y, x+ICON_SIZE, y+ICON_SIZE};
            wcsncpy(icon.name, fd.cFileName, MAX_PATH);
            txtIcons.push_back(icon);
            x += ICON_SIZE + 20;
            if (++count % 4 == 0) { x = 50; y += ICON_SIZE + 40; }
        } while (FindNextFileW(hFind, &fd));
        FindClose(hFind);
    }
    RefreshRecycleIcons();
}

// 简单输入框窗口（非模态）
HWND hInputBox = NULL, hInputEdit = NULL, hInputBtn = NULL;
wchar_t g_inputText[MAX_PATH] = L"";

LRESULT CALLBACK InputBoxProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowW(L"STATIC", L"文件夹名:", WS_CHILD|WS_VISIBLE, 10, 10, 80, 20, hwnd, NULL, NULL, NULL);
        hInputEdit = CreateWindowW(L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_BORDER|ES_AUTOHSCROLL, 90, 10, 150, 20, hwnd, (HMENU)1, NULL, NULL);
        hInputBtn = CreateWindowW(L"BUTTON", L"确定", WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON, 60, 40, 60, 25, hwnd, (HMENU)2, NULL, NULL);
        break;
    case WM_COMMAND:
        if (LOWORD(wParam) == 2) {
            GetWindowTextW(hInputEdit, g_inputText, MAX_PATH);
            DestroyWindow(hwnd);
            hInputBox = NULL;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        hInputBox = NULL;
        break;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

void ShowInputBox(HWND parent) {
    if (hInputBox) return;
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = InputBoxProc;
    wc.hInstance = GetModuleHandleW(NULL);
    wc.lpszClassName = L"InputBoxClass";
    RegisterClassW(&wc);
    hInputBox = CreateWindowExW(WS_EX_TOOLWINDOW, L"InputBoxClass", L"输入文件夹名称", WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        300, 200, 270, 120, parent, NULL, GetModuleHandleW(NULL), NULL);
    ShowWindow(hInputBox, SW_SHOW);
    SetFocus(hInputEdit);
}

// Window Procedure function
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        // 彩色桌面背景
        HBRUSH bgBrush = CreateSolidBrush(RGB(240, 248, 255)); // 淡蓝色
        FillRect(hdc, &ps.rcPaint, bgBrush);
        DeleteObject(bgBrush);
        // 欢迎文字（深蓝色）
        SetTextColor(hdc, RGB(30, 60, 180));
        SetBkMode(hdc, TRANSPARENT);
        static const wchar_t welcomeText[] = L"LinOS - 欢迎使用";
        TextOutW(hdc, 20, 20, welcomeText, wcslen(welcomeText));
        // 文件管理器图标（绿色）
        HBRUSH fileMgrBrush = CreateSolidBrush(RGB(120, 200, 120));
        FillRect(hdc, &fileMgrIcon, fileMgrBrush);
        DeleteObject(fileMgrBrush);
        Rectangle(hdc, fileMgrIcon.left, fileMgrIcon.top, fileMgrIcon.right, fileMgrIcon.bottom);
        SetTextColor(hdc, RGB(0, 120, 0));
        TextOutW(hdc, fileMgrIcon.left, fileMgrIcon.bottom+5, L"文件管理器", 6);
        // 贪吃蛇图标（橙色）
        HBRUSH snakeBrush = CreateSolidBrush(RGB(255, 180, 80));
        FillRect(hdc, &snakeIcon, snakeBrush);
        DeleteObject(snakeBrush);
        Rectangle(hdc, snakeIcon.left, snakeIcon.top, snakeIcon.right, snakeIcon.bottom);
        SetTextColor(hdc, RGB(200, 100, 0));
        TextOutW(hdc, snakeIcon.left, snakeIcon.bottom+5, L"贪吃蛇", 3);
        // 新建文本文档图标（天蓝色）
        HBRUSH txtBrush = CreateSolidBrush(RGB(120, 180, 255));
        FillRect(hdc, &newTxtIcon, txtBrush);
        DeleteObject(txtBrush);
        Rectangle(hdc, newTxtIcon.left, newTxtIcon.top, newTxtIcon.right, newTxtIcon.bottom);
        SetTextColor(hdc, RGB(0, 80, 180));
        TextOutW(hdc, newTxtIcon.left, newTxtIcon.bottom+5, L"新建文本文档", 7);
        // 新建VSCode文件夹图标（紫色）
        HBRUSH vsBrush = CreateSolidBrush(RGB(180, 120, 255));
        FillRect(hdc, &newVSCodeDirIcon, vsBrush);
        DeleteObject(vsBrush);
        Rectangle(hdc, newVSCodeDirIcon.left, newVSCodeDirIcon.top, newVSCodeDirIcon.right, newVSCodeDirIcon.bottom);
        SetTextColor(hdc, RGB(80, 0, 180));
        TextOutW(hdc, newVSCodeDirIcon.left, newVSCodeDirIcon.bottom+5, L"新建VSCode文件夹", 11);
        // 回收站图标（灰色）
        HBRUSH recycleBrush = CreateSolidBrush(RGB(180, 180, 180));
        FillRect(hdc, &recycleBinIcon, recycleBrush);
        DeleteObject(recycleBrush);
        Rectangle(hdc, recycleBinIcon.left, recycleBinIcon.top, recycleBinIcon.right, recycleBinIcon.bottom);
        SetTextColor(hdc, RGB(80, 80, 80));
        TextOutW(hdc, recycleBinIcon.left, recycleBinIcon.bottom+5, L"回收站", 3);
        // 绘制回收站内txt文件图标（灰蓝色）
        for (const auto& icon : recycleIcons) {
            HBRUSH recIconBrush = CreateSolidBrush(RGB(180, 200, 220));
            FillRect(hdc, &icon.rect, recIconBrush);
            DeleteObject(recIconBrush);
            Rectangle(hdc, icon.rect.left, icon.rect.top, icon.rect.right, icon.rect.bottom);
            SetTextColor(hdc, RGB(60, 80, 120));
            TextOutW(hdc, icon.rect.left, icon.rect.bottom+5, icon.name, wcslen(icon.name));
        }
        // 绘制所有txt文件图标（粉色）
        for (const auto& icon : txtIcons) {
            HBRUSH txtIconBrush = CreateSolidBrush(RGB(255, 180, 220));
            FillRect(hdc, &icon.rect, txtIconBrush);
            DeleteObject(txtIconBrush);
            Rectangle(hdc, icon.rect.left, icon.rect.top, icon.rect.right, icon.rect.bottom);
            SetTextColor(hdc, RGB(180, 0, 100));
            TextOutW(hdc, icon.rect.left, icon.rect.bottom+5, icon.name, wcslen(icon.name));
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        if (IsInRect(fileMgrIcon, x, y)) {
            ShowFileManagerWindow(hwnd);
        } else if (IsInRect(snakeIcon, x, y)) {
            ShowSnakeWindow(hwnd);
        } else if (IsInRect(newTxtIcon, x, y)) {
            CreateNewTextFile(hwnd);
            RefreshTxtIcons();
            InvalidateRect(hwnd, NULL, TRUE);
        } else if (IsInRect(newVSCodeDirIcon, x, y)) {
            g_inputText[0] = 0;
            ShowInputBox(hwnd);
        } else if (IsInRect(recycleBinIcon, x, y)) {
            ShowRecycleBinWindow(hwnd);
        } else {
            // 检查是否点击了txt文件图标（右键删除，左键打开）
            if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
                for (const auto& icon : txtIcons) {
                    if (IsInRect(icon.rect, x, y)) {
                        if (MessageBoxW(hwnd, L"确定要移入回收站吗？", icon.name, MB_OKCANCEL|MB_ICONQUESTION) == IDOK) {
                            wchar_t newPath[MAX_PATH];
                            swprintf(newPath, MAX_PATH, L"recycle_bin/%s", icon.name);
                            MoveFileW(icon.name, newPath);
                            RefreshTxtIcons();
                            InvalidateRect(hwnd, NULL, TRUE);
                        }
                        break;
                    }
                }
                // 回收站内操作
                for (const auto& icon : recycleIcons) {
                    if (IsInRect(icon.rect, x, y)) {
                        int op = MessageBoxW(hwnd, L"恢复(是)  彻底删除(否)  取消(取消)", icon.name, MB_YESNOCANCEL|MB_ICONQUESTION);
                        if (op == IDYES) {
                            wchar_t oldPath[MAX_PATH];
                            swprintf(oldPath, MAX_PATH, L"%s", icon.name);
                            wchar_t srcPath[MAX_PATH];
                            swprintf(srcPath, MAX_PATH, L"recycle_bin/%s", icon.name);
                            MoveFileW(srcPath, oldPath);
                        } else if (op == IDNO) {
                            wchar_t srcPath[MAX_PATH];
                            swprintf(srcPath, MAX_PATH, L"recycle_bin/%s", icon.name);
                            DeleteFileW(srcPath);
                        }
                        RefreshTxtIcons();
                        InvalidateRect(hwnd, NULL, TRUE);
                        break;
                    }
                }
            } else {
                for (const auto& icon : txtIcons) {
                    if (IsInRect(icon.rect, x, y)) {
                        ShellExecuteW(hwnd, L"open", icon.name, nullptr, nullptr, SW_SHOW);
                        break;
                    }
                }
            }
        }
        return 0;
    }
    case WM_COMMAND:
        // 检查输入框关闭后是否有输入
        if (!hInputBox && wcslen(g_inputText) > 0) {
            CreateDirectoryW(g_inputText, NULL);
            ShellExecuteW(hwnd, L"open", L"code", g_inputText, NULL, SW_SHOWNORMAL);
            RefreshTxtIcons();
            InvalidateRect(hwnd, NULL, TRUE);
            g_inputText[0] = 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_CREATE:
        RefreshTxtIcons();
        RefreshRecycleIcons();
        break;
    case WM_ACTIVATE:
        if (wParam != WA_INACTIVE) {
            RefreshTxtIcons();
            RefreshRecycleIcons();
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"exeOSWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(hInstance, L"IDI_ICON_MAIN");

    if (!RegisterClassW(&wc)) {
        MessageBoxW(nullptr, L"Failed to register window class!", L"Error", MB_ICONERROR);
        return 0;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_APPWINDOW, // 增加App窗口风格
        CLASS_NAME,
        L"exeOS",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        nullptr, nullptr, hInstance, nullptr);

    if (hwnd == nullptr) {
        MessageBoxW(nullptr, L"Failed to create window!", L"Error", MB_ICONERROR);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
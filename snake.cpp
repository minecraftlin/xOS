#include "snake.h"
#include <windows.h>
#include <deque>
#include <ctime>
#include <dwmapi.h>

#define SNAKE_SIZE 20
#define SNAKE_INIT_LEN 2
#define SNAKE_SPEED 150 // 速度更慢
#define BOARD_W 18
#define BOARD_H 18

#pragma comment(lib, "dwmapi.lib")

struct Point { int x, y; };
static std::deque<Point> snake;
static Point food;
static int dir = 2; // 0上 1下 2左 3右
static bool gameOver = false;

void PlaceFood() {
    srand((unsigned)time(0) + snake.size());
    while (1) {
        food.x = rand() % BOARD_W;
        food.y = rand() % BOARD_H;
        bool onSnake = false;
        for (auto& p : snake) if (p.x == food.x && p.y == food.y) { onSnake = true; break; }
        if (!onSnake) break;
    }
}

void SnakeReset() {
    snake.clear();
    for (int i = 0; i < SNAKE_INIT_LEN; ++i) snake.push_back({BOARD_W/2+i, BOARD_H/2});
    dir = 2; gameOver = false;
    PlaceFood();
}

void SnakeMove() {
    if (gameOver) return;
    Point head = snake.front();
    switch (dir) {
        case 0: head.y--; break;
        case 1: head.y++; break;
        case 2: head.x--; break;
        case 3: head.x++; break;
    }
    // 撞墙
    if (head.x < 0 || head.x >= BOARD_W || head.y < 0 || head.y >= BOARD_H) { gameOver = true; return; }
    // // 自咬（已注释，简单模式）
    // for (auto& p : snake) if (p.x == head.x && p.y == head.y) { gameOver = true; return; }
    snake.push_front(head);
    if (head.x == food.x && head.y == food.y) {
        PlaceFood();
    } else {
        snake.pop_back();
    }
}

void DrawSnake(HDC hdc) {
    // 画蛇
    for (auto& p : snake) Rectangle(hdc, p.x*SNAKE_SIZE, p.y*SNAKE_SIZE, (p.x+1)*SNAKE_SIZE, (p.y+1)*SNAKE_SIZE);
    // 画食物
    HBRUSH hBrush = CreateSolidBrush(RGB(255,0,0));
    RECT r = {food.x*SNAKE_SIZE, food.y*SNAKE_SIZE, (food.x+1)*SNAKE_SIZE, (food.y+1)*SNAKE_SIZE};
    FillRect(hdc, &r, hBrush);
    DeleteObject(hBrush);
}

LRESULT CALLBACK SnakeProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE:
        SnakeReset();
        SetTimer(hwnd, 1, SNAKE_SPEED, NULL);
        break;
    case WM_TIMER:
        SnakeMove();
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    case WM_KEYDOWN:
        if (gameOver && wParam == VK_SPACE) { SnakeReset(); InvalidateRect(hwnd, NULL, TRUE); break; }
        if (wParam == VK_UP && dir != 1) dir = 0;
        else if (wParam == VK_DOWN && dir != 0) dir = 1;
        else if (wParam == VK_LEFT && dir != 3) dir = 2;
        else if (wParam == VK_RIGHT && dir != 2) dir = 3;
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        DrawSnake(hdc);
        if (gameOver) TextOutW(hdc, 100, 180, L"游戏结束，空格重开", 10);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ShowSnakeWindow(HWND parent) {
    static const wchar_t CLASS_NAME[] = L"SnakeWndClass";
    static bool registered = false;
    if (!registered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = SnakeProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.hIcon = LoadIconW(GetModuleHandleW(nullptr), L"IDI_ICON_SNAKE");
        RegisterClassW(&wc);
        registered = true;
    }
    HWND hwnd = CreateWindowExW(WS_EX_APPWINDOW, CLASS_NAME, L"贪吃蛇", WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, SNAKE_SIZE*BOARD_W+16, SNAKE_SIZE*BOARD_H+39, parent, nullptr, GetModuleHandleW(nullptr), nullptr);
    // 高斯模糊
    DWM_BLURBEHIND bb = {0};
    bb.dwFlags = DWM_BB_ENABLE; bb.fEnable = TRUE;
    DwmEnableBlurBehindWindow(hwnd, &bb);
    ShowWindow(hwnd, SW_SHOW);
    SetFocus(hwnd);
}

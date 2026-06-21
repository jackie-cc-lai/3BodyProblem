#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cmath>

#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

namespace {

constexpr int kDefaultWidth = 900;
constexpr int kDefaultHeight = 700;
constexpr int kMargin = 55;

constexpr double kXMin = -10.0;
constexpr double kXMax = 10.0;
constexpr double kYMin = -10.0;
constexpr double kYMax = 10.0;
constexpr double kGridStep = 1.0;

int worldToScreenX(double x, int width) {
    const int plotWidth = width - 2 * kMargin;
    return kMargin + static_cast<int>((x - kXMin) / (kXMax - kXMin) * plotWidth);
}

int worldToScreenY(double y, int height) {
    const int plotHeight = height - 2 * kMargin;
    return height - kMargin - static_cast<int>((y - kYMin) / (kYMax - kYMin) * plotHeight);
}

void drawText(HDC hdc, int x, int y, const char* text) {
    const int length = static_cast<int>(std::strlen(text));
    TextOutA(hdc, x, y, text, length);
}

void drawGrid(HDC hdc, int width, int height) {
    RECT background = {0, 0, width, height};
    FillRect(hdc, &background, static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)));

    HPEN gridPen = CreatePen(PS_SOLID, 1, RGB(215, 215, 215));
    HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(30, 30, 30));
    HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(160, 160, 160));
    HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, gridPen));

    SelectObject(hdc, borderPen);
    MoveToEx(hdc, kMargin, kMargin, nullptr);
    LineTo(hdc, width - kMargin, kMargin);
    LineTo(hdc, width - kMargin, height - kMargin);
    LineTo(hdc, kMargin, height - kMargin);
    LineTo(hdc, kMargin, kMargin);

    for (double x = std::ceil(kXMin / kGridStep) * kGridStep; x <= kXMax + 1e-9; x += kGridStep) {
        const int sx = worldToScreenX(x, width);
        const bool isAxis = std::fabs(x) < 1e-9;
        SelectObject(hdc, isAxis ? axisPen : gridPen);
        MoveToEx(hdc, sx, kMargin, nullptr);
        LineTo(hdc, sx, height - kMargin);
    }

    for (double y = std::ceil(kYMin / kGridStep) * kGridStep; y <= kYMax + 1e-9; y += kGridStep) {
        const int sy = worldToScreenY(y, height);
        const bool isAxis = std::fabs(y) < 1e-9;
        SelectObject(hdc, isAxis ? axisPen : gridPen);
        MoveToEx(hdc, kMargin, sy, nullptr);
        LineTo(hdc, width - kMargin, sy);
    }

    SelectObject(hdc, GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(70, 70, 70));

    char label[32];
    const int originX = worldToScreenX(0.0, width);
    const int originY = worldToScreenY(0.0, height);

    for (double x = std::ceil(kXMin / kGridStep) * kGridStep; x <= kXMax + 1e-9; x += kGridStep) {
        if (std::fabs(x) < 1e-9) {
            continue;
        }
        std::snprintf(label, sizeof(label), "%g", x);
        const int sx = worldToScreenX(x, width);
        drawText(hdc, sx - 8, height - kMargin + 8, label);
    }

    for (double y = std::ceil(kYMin / kGridStep) * kGridStep; y <= kYMax + 1e-9; y += kGridStep) {
        if (std::fabs(y) < 1e-9) {
            continue;
        }
        std::snprintf(label, sizeof(label), "%g", y);
        const int sy = worldToScreenY(y, height);
        drawText(hdc, 8, sy - 8, label);
    }

    SetTextColor(hdc, RGB(20, 20, 20));
    drawText(hdc, width - kMargin + 10, originY - 18, "x");
    drawText(hdc, originX + 8, kMargin - 22, "y");
    drawText(hdc, originX + 6, originY + 6, "0");

    SelectObject(hdc, oldPen);
    DeleteObject(gridPen);
    DeleteObject(axisPen);
    DeleteObject(borderPen);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT client;
            GetClientRect(hwnd, &client);
            drawGrid(hdc, client.right, client.bottom);
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_SIZE:
            InvalidateRect(hwnd, nullptr, TRUE);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

}  // namespace

int main() {
    std::cout << "Opening coordinate grid window. Close the window to exit.\n";

    HINSTANCE instance = GetModuleHandle(nullptr);
    const wchar_t* className = L"CoordGridWindow";

    WNDCLASSW windowClass = {};
    windowClass.lpfnWndProc = WndProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = className;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&windowClass)) {
        std::cerr << "Failed to register window class.\n";
        return EXIT_FAILURE;
    }

    HWND hwnd = CreateWindowW(
        className,
        L"Coordinate Grid",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        kDefaultWidth,
        kDefaultHeight,
        nullptr,
        nullptr,
        instance,
        nullptr);

    if (!hwnd) {
        std::cerr << "Failed to create window.\n";
        return EXIT_FAILURE;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return static_cast<int>(msg.wParam);
}

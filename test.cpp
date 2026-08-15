#define OEMRESOURCE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdlib>

// Оконная процедура для полноэкранных оверлеев
LRESULT CALLBACK OverlayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Вспомогательная функция создания оверлейного окна
HWND CreateOverlayWindow(HINSTANCE hInstance) {
    const wchar_t CLASS_NAME[] = L"RecordingOverlayClass";
    static bool isRegistered = false;

    if (!isRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = OverlayProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
        RegisterClassW(&wc);
        isRegistered = true;
    }

    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        CLASS_NAME, L"Overlay", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    return hwnd;
}

void PIZDA(HINSTANCE hInstance)
{
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event('D', 0, 0, 0);
    keybd_event('D', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
}

void GlitchScreenOverlay(HINSTANCE hInstance, const std::wstring& overlayText, int durationMs = 2000) {
    const wchar_t CLASS_NAME[] = L"GlitchTransparentClass";
    static bool isRegistered = false;

    if (!isRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = OverlayProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassW(&wc);
        isRegistered = true;
    }

    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        CLASS_NAME, L"GlitchOverlay", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return;

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    HDC hdcWindow = GetDC(hwnd);
    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RECT fullScreenRect = { 0, 0, width, height };

    // Создаем крупный шрифт для надписи
    HFONT hFont = CreateFontW(
        64, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Impact"
    );

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < durationMs) {

        // 1. Очищаем прошлый кадр в прозрачный цвет
        FillRect(hdcWindow, &fullScreenRect, blackBrush);

        // 2. Рисуем цветные прямоугольники помех
        int blockCount = rand() % 5 + 3;
        for (int i = 0; i < blockCount; ++i) {
            int rx = rand() % width;
            int ry = rand() % height;
            int rw = rand() % 400 + 100;
            int rh = rand() % 100 + 20;

            BYTE r = rand() % 255 + 1;
            BYTE g = rand() % 255 + 1;
            BYTE b = rand() % 255 + 1;

            HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
            RECT rect = { rx, ry, rx + rw, ry + rh };
            FillRect(hdcWindow, &rect, brush);
            DeleteObject(brush);
        }

        // 3. Выводим текст сверху экрана
        HFONT hOldFont = (HFONT)SelectObject(hdcWindow, hFont);
        SetBkMode(hdcWindow, TRANSPARENT);
        SetTextColor(hdcWindow, RGB(255, 0, 0)); // Красный цвет текста

        RECT textRect = { 0, 40, width, 150 }; // Область сверху (отступ 40px)
        DrawTextW(
            hdcWindow,
            overlayText.c_str(),
            -1,
            &textRect,
            DT_CENTER | DT_SINGLELINE | DT_NOCLIP
        );

        SelectObject(hdcWindow, hOldFont);

        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    DeleteObject(hFont);
    ReleaseDC(hwnd, hdcWindow);
    DestroyWindow(hwnd);
}

// 2. Инверсия экрана через снимки (видима на записи)
void InvertScreenOverlay(HINSTANCE hInstance, int durationMs = 2000) {
    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // Делаем скриншот рабочего стола
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // Копируем с инверсией цвета (NOTSRCCOPY)
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, x, y, NOTSRCCOPY);

    // Создаем поверх окно с этой инвертированной картинкой
    HWND hwnd = CreateOverlayWindow(hInstance);
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

    HDC hdcWin = GetDC(hwnd);
    BitBlt(hdcWin, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));

    ReleaseDC(hwnd, hdcWin);
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
    DestroyWindow(hwnd);
}

// 3. Дрожание активного окна
void ShakeWindow(HWND hwnd, int durationMs = 2000) {
    if (!hwnd) hwnd = GetForegroundWindow();
    if (!hwnd) return;

    RECT rc;
    GetWindowRect(hwnd, &rc);
    int origX = rc.left;
    int origY = rc.top;

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < durationMs) {

        int offsetX = (rand() % 31) - 15;
        int offsetY = (rand() % 31) - 15;

        SetWindowPos(hwnd, HWND_TOPMOST, origX + offsetX, origY + offsetY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    SetWindowPos(hwnd, HWND_TOPMOST, origX, origY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}

// 4. Гигантский курсор
static HCURSOR CreateGiantCursor(int size = 256) {
    HDC hdc = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBmp = CreateCompatibleBitmap(hdc, size, size);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
    RECT rect = { 0, 0, size, size };
    FillRect(hdcMem, &rect, brush);
    DeleteObject(brush);

    ICONINFO ii = { 0 };
    ii.fIcon = FALSE;
    ii.xHotspot = size / 2;
    ii.yHotspot = size / 2;
    ii.hbmMask = hBmp;
    ii.hbmColor = hBmp;

    HCURSOR hGiantCursor = CreateIconIndirect(&ii);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdc);

    return hGiantCursor;
}

void SetHugeCursorForDuration(int durationMs = 3000) {
    HCURSOR hGiant = CreateGiantCursor(256);
    SetSystemCursor(hGiant, OCR_NORMAL);

    std::this_thread::sleep_for(std::chrono::milliseconds(durationMs));

    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    std::cout << "=== Тест эффектов (подходит для записи экрана) ===" << std::endl;
    std::cout << "[Shift + F1] - Дрожание окна" << std::endl;
    std::cout << "[Shift + F2] - Инверсия экрана (записывается)" << std::endl;
    std::cout << "[Shift + F3] - Огромный курсор" << std::endl;
    std::cout << "[Shift + F4] - Помехи на экране (записываются)\n" << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shiftPressed && (GetAsyncKeyState(VK_F1) & 0x8000)) {
            std::cout << "[ТЕСТ] Дрожание окна..." << std::endl;
            ShakeWindow(GetForegroundWindow(), 2000);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        if (shiftPressed && (GetAsyncKeyState(VK_F2) & 0x8000)) {
            std::cout << "[ТЕСТ] Инверсия..." << std::endl;
            InvertScreenOverlay(hInstance, 2000);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        if (shiftPressed && (GetAsyncKeyState(VK_F3) & 0x8000)) {
            std::cout << "[ТЕСТ] Гигантский курсор..." << std::endl;
            SetHugeCursorForDuration(3000);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        if (shiftPressed && (GetAsyncKeyState(VK_F4) & 0x8000)) {
            std::cout << "[ТЕСТ] Помехи..." << std::endl;
            GlitchScreenOverlay(hInstance, L"VAS VZLAMIVAET JOPA VAS VZLAMIVAET JOPA", 2000);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        if (shiftPressed && (GetAsyncKeyState(VK_F5) & 0x8000)) {
            std::cout << "[ТЕСТ] PIZDA..." << std::endl;
            PIZDA(hInstance);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }

    return 0;
}
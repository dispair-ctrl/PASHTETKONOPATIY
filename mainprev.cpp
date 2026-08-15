#define WIN32_LEAN_AND_MEAN  
#define OEMRESOURCE
#include <windows.h>  
#include <mmsystem.h>  
#include <magnification.h>  
#include <iostream>  
#include <random>  
#include <chrono>  
#include <thread>  
#include <vector>  
#include <atomic>
#include <string>

#include "resource.h"

#pragma comment(lib, "winmm.lib")  
#pragma comment(lib, "magnification.lib")  
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ============================================================================
// ИДЕНТИФИКАТОРЫ РЕСУРСОВ
// ============================================================================

// Изображения для скримера (screamerpics: 1.bmp - 25.bmp)
const std::vector<WORD> IMAGES = {
    IDB_PHOTO1,  IDB_PHOTO2,  IDB_PHOTO3,  IDB_PHOTO4,  IDB_PHOTO5,
    IDB_PHOTO6,  IDB_PHOTO7,  IDB_PHOTO8,  IDB_PHOTO9,  IDB_PHOTO10,
    IDB_PHOTO11, IDB_PHOTO12, IDB_PHOTO13, IDB_PHOTO14, IDB_PHOTO15,
    IDB_PHOTO16, IDB_PHOTO17, IDB_PHOTO18, IDB_PHOTO19, IDB_PHOTO20,
    IDB_PHOTO21, IDB_PHOTO22, IDB_PHOTO23, IDB_PHOTO24, IDB_PHOTO25
};

// Звуки для скримера (screamersounds: 1.wav - 22.wav)
const std::vector<WORD> SCREAMER_SOUNDS = {
    IDR_SCREAMERSOUND1,  IDR_SCREAMERSOUND2,  IDR_SCREAMERSOUND3,  IDR_SCREAMERSOUND4,
    IDR_SCREAMERSOUND5,  IDR_SCREAMERSOUND6,  IDR_SCREAMERSOUND7,  IDR_SCREAMERSOUND8,
    IDR_SCREAMERSOUND9,  IDR_SCREAMERSOUND10, IDR_SCREAMERSOUND11, IDR_SCREAMERSOUND12,
    IDR_SCREAMERSOUND13, IDR_SCREAMERSOUND14, IDR_SCREAMERSOUND15, IDR_SCREAMERSOUND16,
    IDR_SCREAMERSOUND17, IDR_SCREAMERSOUND18, IDR_SCREAMERSOUND19, IDR_SCREAMERSOUND20,
    IDR_SCREAMERSOUND21, IDR_SCREAMERSOUND22
};

// Звуки для флешки (flashbangs: 1.wav - 4.wav)
const std::vector<WORD> FLASHBANG_SOUNDS = {
    IDR_FLASHBANGSOUND1, IDR_FLASHBANGSOUND2, IDR_FLASHBANGSOUND3, IDR_FLASHBANGSOUND4
};

// Длинные звуки (longsounds: 1.wav - 14.wav)
const std::vector<WORD> LONG_SOUNDS = {
    IDR_LONGSOUND1,  IDR_LONGSOUND2,  IDR_LONGSOUND3,  IDR_LONGSOUND4,  IDR_LONGSOUND5,
    IDR_LONGSOUND6,  IDR_LONGSOUND7,  IDR_LONGSOUND8,  IDR_LONGSOUND9,  IDR_LONGSOUND10,
    IDR_LONGSOUND11, IDR_LONGSOUND12, IDR_LONGSOUND13, IDR_LONGSOUND14
};

// Список всех типов системных курсоров для замены на гигантский квадрат
const DWORD SYSTEM_CURSOR_IDS[] = {
    OCR_NORMAL, OCR_IBEAM, OCR_WAIT, OCR_CROSS, OCR_UP,
    OCR_SIZENWSE, OCR_SIZENESW, OCR_SIZEWE, OCR_SIZENS,
    OCR_SIZEALL, OCR_NO, OCR_HAND, OCR_APPSTARTING
};

// ============================================================================
// НАСТРОЙКИ РЕЖИМОВ И ИНТЕНСИВНОСТИ СОБЫТИЙ
// ============================================================================

enum FrequencyMode {
    MODE_LOW = 0,
    MODE_MEDIUM = 1,
    MODE_HIGH = 2,
    MODE_DEATH = 3,
    MODE_DEATHPLUS = 4
};

struct ModeChances {
    int flashbang;   // Шанс 1 из N в секунду
    int screamer;    // Шанс 1 из N в секунду
    int longSound;   // Шанс 1 из N в секунду
    int glitch;      // Шанс 1 из N в секунду
    int shake;       // Шанс 1 из N в секунду
    int invert;      // Шанс 1 из N в секунду
    int giantCursor; // Шанс 1 из N в секунду
    int pizda;       // Шанс 1 из N в секунду
    int zoom;        // Шанс 1 из N в секунду
    int stripe;      // Шанс 1 из N в секунду (Черная вертикальная полоса)
    int skillCheck;  // Шанс 1 из N в секунду (Скиллчек DBD)
    const wchar_t* description;
};

// Вероятности для каждого режима
const ModeChances G_MODE_CHANCES[] = {
    // 0: Низкая
    { 200, 500, 400, 350, 300, 400, 350, 600, 350, 350, 450, L"Режим: НИЗКАЯ частота событий" },
    // 1: Средняя
    { 100, 250, 200, 180, 150, 200, 180, 300, 180, 180, 250, L"Режим: СРЕДНЯЯ частота событий" },
    // 2: Высокая (Шанс флешки 1/50)
    {  50, 120, 100,  90,  75, 100,  90, 150,  90,  90, 120, L"Режим: ВЫСОКАЯ частота (Флешка 1/50)" },
    // 3: Смерть (Шанс флешки 1/25)
    {  25,  40,  50,  30,  25,  35,  30,  50,  30,  30,  45, L"Режим: 💀 СМЕРТЬ! (Флешка 1/25)" },
    // 4: смерть++
    {  15,  25,  30,  17,  15,  26,  17,  30,  17,  17,  25, L"Режим: 💀💀💀 СМЕРТЬ++!" }
};

// Глобальные состояния (потокобезопасные атомарные переменные)
std::atomic<bool> g_isEnabled(true);
std::atomic<FrequencyMode> g_currentMode(MODE_MEDIUM);
std::atomic<bool> g_isRunning(true);
std::atomic<bool> g_cancelEffects(false); // Флаг мгновенной отмены эффектов
std::atomic<bool> g_isEffectActive(false); // Защита от одновременного запуска нескольких эффектов

WORD selectedImageId = 0;
WORD selectedSoundId = 0;

// Идентификаторы элементов управления GUI
#define IDC_TOGGLE_BTN        1001
#define IDC_RADIO_LOW         1002
#define IDC_RADIO_MED         1003
#define IDC_RADIO_HIGH        1004
#define IDC_RADIO_DEATH       1005
#define IDC_RADIO_DEATHPLUS   1008
#define IDC_STATUS_LABEL      1006
#define IDC_BTN_CANCEL_EFFECT 1007

#ifdef _DEBUG
#define IDC_BTN_TEST_LONGSOUND 2001
#define IDC_BTN_TEST_SCREAMER  2002
#define IDC_BTN_TEST_FLASHBANG 2003
#define IDC_BTN_TEST_GLITCH    2004
#define IDC_BTN_TEST_SHAKE     2005
#define IDC_BTN_TEST_INVERT    2006
#define IDC_BTN_TEST_CURSOR    2007
#define IDC_BTN_TEST_PIZDA     2008
#define IDC_BTN_TEST_ZOOM      2009
#define IDC_BTN_TEST_STRIPE    2010
#define IDC_BTN_TEST_SKILLCHECK 2011
#endif

HWND g_hwndStatus = NULL;
HBRUSH g_hDarkBrush = NULL;
HFONT g_hGuiFont = NULL;

// ============================================================================
// ФУНКЦИЯ МГНОВЕННОГО СБРОСА ВСЕХ ЭФФЕКТОВ
// ============================================================================

void CancelAllEffects() {
    g_cancelEffects.store(true);

    // 1. Останавливаем все звуки
    PlaySoundW(NULL, NULL, 0);

    // 2. Восстанавливаем оригинальные системные курсоры Windows
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

    // 3. Отменяем инверсию цвета и зум Magnification API
    if (MagInitialize()) {
        MAGCOLOREFFECT identityMatrix = {
             1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  0.0f,  1.0f
        };
        MagSetFullscreenColorEffect(&identityMatrix);
        MagSetFullscreenTransform(1.0f, 0, 0);
        MagUninitialize();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    g_cancelEffects.store(false);
    g_isEffectActive.store(false);
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ И ОВЕРЛЕИ
// ============================================================================

LRESULT CALLBACK OverlayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND:
        return 1;
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

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
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    return CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"Overlay", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );
}

// 1. Сворачивание всех окон (Win + D)
void PIZDA(HINSTANCE hInstance) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event('D', 0, 0, 0);
    keybd_event('D', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    g_isEffectActive.store(false);
}

// 2. Визуальные помехи на экране поверх всех окон (клики проходят сквозь, 7 - 20 секунд)
void GlitchScreenOverlay(HINSTANCE hInstance, const std::wstring& overlayText, int durationMs = 12000) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

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

    // WS_EX_TOPMOST поверх всех окон, WS_EX_TRANSPARENT - клики проходят сквозь!
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"GlitchOverlay", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        g_isEffectActive.store(false);
        return;
    }

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    HDC hdcWindow = GetDC(hwnd);
    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RECT fullScreenRect = { 0, 0, width, height };

    HFONT hFont = CreateFontW(
        64, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Impact"
    );

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < durationMs) {

        if (g_cancelEffects.load()) break;

        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        FillRect(hdcWindow, &fullScreenRect, blackBrush);

        int blockCount = rand() % 6 + 4;
        for (int i = 0; i < blockCount; ++i) {
            int rx = rand() % width;
            int ry = rand() % height;
            int rw = rand() % 450 + 100;
            int rh = rand() % 120 + 20;

            BYTE r = rand() % 255 + 1;
            BYTE g = rand() % 255 + 1;
            BYTE b = rand() % 255 + 1;

            HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
            RECT rect = { rx, ry, rx + rw, ry + rh };
            FillRect(hdcWindow, &rect, brush);
            DeleteObject(brush);
        }

        HFONT hOldFont = (HFONT)SelectObject(hdcWindow, hFont);
        SetBkMode(hdcWindow, TRANSPARENT);
        SetTextColor(hdcWindow, RGB(255, 0, 0));

        RECT textRect = { 0, 40, width, 150 };
        DrawTextW(hdcWindow, overlayText.c_str(), -1, &textRect, DT_CENTER | DT_SINGLELINE | DT_NOCLIP);

        SelectObject(hdcWindow, hOldFont);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    DeleteObject(hFont);
    ReleaseDC(hwnd, hdcWindow);
    DestroyWindow(hwnd);
    g_isEffectActive.store(false);
}

// 3. Инверсия экрана «на ходу» в реальном времени (живая интерактивная работа, 7 - 20 секунд)
void InvertScreen(int durationMs = 12000) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    if (MagInitialize()) {
        MAGCOLOREFFECT invertMatrix = {
            -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
             0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f
        };

        MAGCOLOREFFECT identityMatrix = {
             1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  0.0f,  1.0f
        };

        // Живая инверсия DWM на аппаратном уровне
        MagSetFullscreenColorEffect(&invertMatrix);
        
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() < durationMs) {
            if (g_cancelEffects.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        MagSetFullscreenColorEffect(&identityMatrix);
        MagUninitialize();
    }

    g_isEffectActive.store(false);
}

void InvertScreenOverlay(HINSTANCE hInstance, int durationMs = 12000) {
    InvertScreen(durationMs);
}

// 4. Дрожание активного окна
void ShakeWindow(HWND hwnd, int durationMs = 2000) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    if (!hwnd) hwnd = GetForegroundWindow();
    if (!hwnd) {
        g_isEffectActive.store(false);
        return;
    }

    RECT rc;
    GetWindowRect(hwnd, &rc);
    int origX = rc.left;
    int origY = rc.top;

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < durationMs) {
        if (g_cancelEffects.load()) break;

        int offsetX = (rand() % 31) - 15;
        int offsetY = (rand() % 31) - 15;

        SetWindowPos(hwnd, NULL, origX + offsetX, origY + offsetY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
    }

    SetWindowPos(hwnd, NULL, origX, origY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);
    g_isEffectActive.store(false);
}

// 5. Создание гигантского квадратного курсора
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

// 5. Оверлейный и системный гигантский квадратный курсор (работает везде, включая игры и Dota 2)
void SetHugeCursorForDuration(HINSTANCE hInstance, int durationMs = 7000) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    // 1. Меняем системные курсоры Windows
    for (DWORD cursorId : SYSTEM_CURSOR_IDS) {
        HCURSOR hGiant = CreateGiantCursor(256);
        SetSystemCursor(hGiant, cursorId);
    }

    // 2. Создаем фоновый оверлей поверх всех окон (для игр и Dota 2)
    const wchar_t CLASS_NAME[] = L"CursorOverlayClass";
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
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"CursorOverlay", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd) {
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

        HDC hdcWin = GetDC(hwnd);
        HDC hdcMem = CreateCompatibleDC(hdcWin);
        HBITMAP hBmp = CreateCompatibleBitmap(hdcWin, width, height);
        HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

        HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
        HBRUSH redBrush = CreateSolidBrush(RGB(255, 0, 0));
        RECT fullScreenRect = { 0, 0, width, height };
        int cursorSize = 256;

        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() < durationMs) {

            if (g_cancelEffects.load()) break;

            MSG msg;
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }

            POINT pt;
            GetCursorPos(&pt);
            int lx = pt.x - x;
            int ly = pt.y - y;

            FillRect(hdcMem, &fullScreenRect, blackBrush);

            RECT squareRect = { lx - cursorSize / 2, ly - cursorSize / 2, lx + cursorSize / 2, ly + cursorSize / 2 };
            FillRect(hdcMem, &squareRect, redBrush);

            BitBlt(hdcWin, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        DeleteObject(redBrush);
        SelectObject(hdcMem, hOldBmp);
        DeleteObject(hBmp);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWin);
        DestroyWindow(hwnd);
    }

    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);
    g_isEffectActive.store(false);
}

// 6. Оконная процедура и запуск Флешки (клики и фокус беспрепятственно проходят сквозь!)
LRESULT CALLBACK FlashWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rect;
        GetClientRect(hwnd, &rect);
        FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
        return 1;
    }
    case WM_DESTROY:
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void Flashbang(HINSTANCE hInstance, int durationMs, std::mt19937& gen) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    const wchar_t CLASS_NAME[] = L"FlashbangOverlayClass";

    static bool isRegistered = false;
    if (!isRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = FlashWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
        RegisterClassW(&wc);
        isRegistered = true;
    }

    int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // WS_EX_TOPMOST поверх всего, WS_EX_TRANSPARENT | WS_EX_NOACTIVATE - клики всегда проходят сквозь окно!
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"Flash", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        g_isEffectActive.store(false);
        return;
    }

    std::uniform_int_distribution<size_t> sndDist(0, FLASHBANG_SOUNDS.size() - 1);
    WORD flashSoundId = FLASHBANG_SOUNDS[sndDist(gen)];
    PlaySoundW(MAKEINTRESOURCE(flashSoundId), hInstance, SND_RESOURCE | SND_ASYNC);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    UpdateWindow(hwnd);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    int holdTimeMs = 350;
    for (int elapsed = 0; elapsed < holdTimeMs; elapsed += 30) {
        if (g_cancelEffects.load()) { DestroyWindow(hwnd); g_isEffectActive.store(false); return; }
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }

    int steps = 30;
    int fadeTimeMs = durationMs - holdTimeMs;
    if (fadeTimeMs < 100) fadeTimeMs = 100;
    int stepDelay = fadeTimeMs / steps;

    for (int i = steps; i >= 0; i--) {
        if (g_cancelEffects.load()) break;
        BYTE alpha = (BYTE)((i / (float)steps) * 255);
        SetLayeredWindowAttributes(hwnd, 0, alpha, LWA_ALPHA);
        UpdateWindow(hwnd);
        std::this_thread::sleep_for(std::chrono::milliseconds(stepDelay));
    }

    DestroyWindow(hwnd);
    g_isEffectActive.store(false);
}

// 7. Увеличение экрана (Зум 2x) через Magnification API (5 - 15 секунд)
void ZoomScreen(float magFactor = 2.0f, int durationMs = 8000) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    if (MagInitialize()) {
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);
        int xOffset = (int)(screenW / 4.0f);
        int yOffset = (int)(screenH / 4.0f);

        MagSetFullscreenTransform(magFactor, xOffset, yOffset);

        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() < durationMs) {
            if (g_cancelEffects.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        MagSetFullscreenTransform(1.0f, 0, 0);
        MagUninitialize();
    }

    g_isEffectActive.store(false);
}

// 8. Проигрывание случайного длинного звука
void PlayLongSound(HINSTANCE hInstance, std::mt19937& gen) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    std::uniform_int_distribution<size_t> sndDist(0, LONG_SOUNDS.size() - 1);
    WORD soundId = LONG_SOUNDS[sndDist(gen)];

#ifdef _DEBUG
    std::wcout << L"[LONG SOUND] Proigryvaetsya dlinnyy zvuk ID: " << soundId << std::endl;
#endif
    PlaySoundW(MAKEINTRESOURCE(soundId), hInstance, SND_RESOURCE | SND_ASYNC);
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));

    g_isEffectActive.store(false);
}

// 7.5. Неосязаемые черные вертикальные и горизонтальные полосы на каждом мониторе со случайной скоростью каждую секунду
void BlackStripeOverlay(HINSTANCE hInstance, int durationMs = 10000) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    const wchar_t CLASS_NAME[] = L"BlackStripeOverlayClass";
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

    int virtX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int virtWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    // WS_EX_TOPMOST поверх всего, WS_EX_TRANSPARENT | WS_EX_NOACTIVATE — клики всегда проходят сквозь!
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"BlackStripeOverlay", WS_POPUP | WS_VISIBLE,
        virtX, virtY, virtWidth, virtHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        g_isEffectActive.store(false);
        return;
    }

    // Зеленый цвет для прозрачности (ChromaKey)
    COLORREF chromaKey = RGB(0, 255, 0);
    SetLayeredWindowAttributes(hwnd, chromaKey, 0, LWA_COLORKEY);

    HDC hdcWin = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWin);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcWin, virtWidth, virtHeight);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    HBRUSH greenBrush = CreateSolidBrush(chromaKey);
    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    HBRUSH whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    RECT fullScreenRect = { 0, 0, virtWidth, virtHeight };

    // Перечисление всех подключенных мониторов
    struct MonData {
        RECT rc;
    };
    std::vector<MonData> rawMonitors;
    EnumDisplayMonitors(NULL, NULL, [](HMONITOR hMon, HDC hdc, LPRECT lprc, LPARAM p) -> BOOL {
        MONITORINFO mi = { sizeof(MONITORINFO) };
        if (GetMonitorInfoW(hMon, &mi)) {
            auto* vec = reinterpret_cast<std::vector<MonData>*>(p);
            vec->push_back({ mi.rcMonitor });
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&rawMonitors));

    if (rawMonitors.empty()) {
        RECT fallback = { virtX, virtY, virtX + virtWidth, virtY + virtHeight };
        rawMonitors.push_back({ fallback });
    }

    auto pickRandomSpeed = []() -> float {
        int spd = (rand() % 26) + 6; // от 6 до 31 пикселей за кадр
        if (rand() % 2 == 0) spd = -spd;
        return (float)spd;
    };

    struct MonitorStripes {
        int localLeft, localTop, localRight, localBottom;
        int monWidth, monHeight;

        // Вертикальная полоса (движется влево-вправо)
        int vWidth;        // 35 - 55 px
        float vPosX;
        float vSpeedX;

        // Горизонтальная полоса (движется вверх-вниз)
        int hHeight;       // 35 - 55 px
        float hPosY;
        float hSpeedY;

        std::chrono::steady_clock::time_point lastSpeedChange;
    };

    auto start = std::chrono::steady_clock::now();
    std::vector<MonitorStripes> stripesList;

    for (const auto& m : rawMonitors) {
        MonitorStripes s;
        s.localLeft = m.rc.left - virtX;
        s.localTop = m.rc.top - virtY;
        s.localRight = m.rc.right - virtX;
        s.localBottom = m.rc.bottom - virtY;
        s.monWidth = s.localRight - s.localLeft;
        s.monHeight = s.localBottom - s.localTop;

        s.vWidth = rand() % 21 + 35; // 35 - 55 пикселей
        int maxVX = s.monWidth - s.vWidth;
        s.vPosX = (float)(rand() % (maxVX > 0 ? maxVX : 1));
        s.vSpeedX = pickRandomSpeed();

        s.hHeight = rand() % 21 + 35; // 35 - 55 пикселей
        int maxHY = s.monHeight - s.hHeight;
        s.hPosY = (float)(rand() % (maxHY > 0 ? maxHY : 1));
        s.hSpeedY = pickRandomSpeed();

        s.lastSpeedChange = start;
        stripesList.push_back(s);
    }

    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < durationMs) {

        if (g_cancelEffects.load()) break;

        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        auto now = std::chrono::steady_clock::now();

        // Заливка фона всего виртуального экрана прозрачным зеленым
        FillRect(hdcMem, &fullScreenRect, greenBrush);

        for (auto& s : stripesList) {
            // Каждую секунду выбираем новую случайную скорость для вертикальной и горизонтальной полос
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s.lastSpeedChange).count() >= 1000) {
                s.vSpeedX = pickRandomSpeed();
                s.hSpeedY = pickRandomSpeed();
                s.lastSpeedChange = now;
            }

            // Обновление положения вертикальной полосы (влево-вправо)
            s.vPosX += s.vSpeedX;
            if (s.vPosX < 0) {
                s.vPosX = 0;
                s.vSpeedX = -s.vSpeedX;
            }
            else if (s.vPosX + s.vWidth > s.monWidth) {
                s.vPosX = (float)(s.monWidth - s.vWidth);
                s.vSpeedX = -s.vSpeedX;
            }

            // Обновление положения горизонтальной полосы (вверх-вниз)
            s.hPosY += s.hSpeedY;
            if (s.hPosY < 0) {
                s.hPosY = 0;
                s.hSpeedY = -s.hSpeedY;
            }
            else if (s.hPosY + s.hHeight > s.monHeight) {
                s.hPosY = (float)(s.monHeight - s.hHeight);
                s.hSpeedY = -s.hSpeedY;
            }

            // Динамический выбор кисти (эпилептическое мерцание между черным и белым)
            HBRUSH vBrush = (rand() % 2 == 0) ? blackBrush : whiteBrush;
            HBRUSH hBrush = (rand() % 2 == 0) ? blackBrush : whiteBrush;

            // Отрисовка вертикальной полосы
            RECT vRect = {
                s.localLeft + (int)s.vPosX,
                s.localTop,
                s.localLeft + (int)s.vPosX + s.vWidth,
                s.localBottom
            };
            FillRect(hdcMem, &vRect, vBrush);

            // Отрисовка горизонтальной полосы
            RECT hRect = {
                s.localLeft,
                s.localTop + (int)s.hPosY,
                s.localRight,
                s.localTop + (int)s.hPosY + s.hHeight
            };
            FillRect(hdcMem, &hRect, hBrush);
        }

        BitBlt(hdcWin, 0, 0, virtWidth, virtHeight, hdcMem, 0, 0, SRCCOPY);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    DeleteObject(greenBrush);
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWin);
    DestroyWindow(hwnd);
    g_isEffectActive.store(false);
}

// ============================================================================
// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ ЗАГРУЗКИ BMP ИЗОБРАЖЕНИЙ
// ============================================================================

HBITMAP LoadScreamerImage(HINSTANCE hInstance, WORD resId) {
    int index = (int)(resId - IDB_PHOTO1 + 1);
    if (index >= 1 && index <= 100) {
        wchar_t bmpPath[MAX_PATH];
        swprintf_s(bmpPath, L"screamerpics\\%d.bmp", index);
        if (GetFileAttributesW(bmpPath) != INVALID_FILE_ATTRIBUTES) {
            HBITMAP hBmp = (HBITMAP)LoadImageW(NULL, bmpPath, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE | LR_CREATEDIBSECTION);
            if (hBmp) return hBmp;
        }
    }

    return (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(resId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
}

// 9. Оконная процедура и запуск Полноэкранного скримера
LRESULT CALLBACK ScreamerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HBITMAP hScreamerBitmap = NULL;

    switch (uMsg) {
    case WM_CREATE:
        hScreamerBitmap = LoadScreamerImage(GetModuleHandle(NULL), selectedImageId);
        SetTimer(hwnd, 1, 2000, NULL);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (hScreamerBitmap) {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            HDC hdcMem = CreateCompatibleDC(hdc);
            HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hScreamerBitmap);

            BITMAP bm;
            GetObject(hScreamerBitmap, sizeof(bm), &bm);

            SetStretchBltMode(hdc, HALFTONE);
            StretchBlt(hdc, 0, 0, screenWidth, screenHeight,
                       hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

            SelectObject(hdcMem, hOldBmp);
            DeleteDC(hdcMem);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER:
        PlaySoundW(NULL, NULL, 0);
        KillTimer(hwnd, 1);
        DestroyWindow(hwnd);
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PlaySoundW(NULL, NULL, 0);
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_DESTROY:
        if (hScreamerBitmap) {
            DeleteObject(hScreamerBitmap);
            hScreamerBitmap = NULL;
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ShowScreamerWindow(HINSTANCE hInstance, std::mt19937& gen) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    std::uniform_int_distribution<size_t> imgDist(0, IMAGES.size() - 1);
    std::uniform_int_distribution<size_t> sndDist(0, SCREAMER_SOUNDS.size() - 1);

    selectedImageId = IMAGES[imgDist(gen)];
    selectedSoundId = SCREAMER_SOUNDS[sndDist(gen)];

#ifdef _DEBUG
    std::wcout << L"[SCREAMER] Image ID: " << selectedImageId << L" | Sound ID: " << selectedSoundId << std::endl;
#endif

    const wchar_t CLASS_NAME[] = L"ScreamerExternalClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc = ScreamerWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassW(&wc);

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        CLASS_NAME,
        L"Screamer",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenWidth, screenHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd != NULL) {
        PlaySoundW(MAKEINTRESOURCE(selectedSoundId), hInstance, SND_RESOURCE | SND_ASYNC);

        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (g_cancelEffects.load()) { DestroyWindow(hwnd); break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnregisterClassW(CLASS_NAME, hInstance);
    g_isEffectActive.store(false);
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ СКИЛЛЧЕКА И ТРОЙНОГО НАКАЗАНИЯ
// ============================================================================

void TriggerSingleRandomEvent(HINSTANCE hInstance, std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, 9);
    int choice = dist(gen);
    switch (choice) {
    case 0:
        Flashbang(hInstance, 2000, gen);
        break;
    case 1:
        ShowScreamerWindow(hInstance, gen);
        break;
    case 2:
        PlayLongSound(hInstance, gen);
        break;
    case 3: {
        std::uniform_int_distribution<int> distGlitchDur(7000, 20000);
        GlitchScreenOverlay(hInstance, L"VAS VZLAMIVAET JOPA", distGlitchDur(gen));
        break;
    }
    case 4:
        ShakeWindow(GetForegroundWindow(), 2000);
        break;
    case 5: {
        std::uniform_int_distribution<int> distInvertDur(7000, 20000);
        InvertScreenOverlay(hInstance, distInvertDur(gen));
        break;
    }
    case 6: {
        std::uniform_int_distribution<int> distCursorDur(5000, 10000);
        SetHugeCursorForDuration(hInstance, distCursorDur(gen));
        break;
    }
    case 7:
        PIZDA(hInstance);
        break;
    case 8: {
        std::uniform_int_distribution<int> distZoomDur(5000, 15000);
        ZoomScreen(2.0f, distZoomDur(gen));
        break;
    }
    case 9: {
        std::uniform_int_distribution<int> distStripeDur(7000, 15000);
        BlackStripeOverlay(hInstance, distStripeDur(gen));
        break;
    }
    }
}

void TriggerThreeRandomEvents(HINSTANCE hInstance, std::mt19937& gen) {
    for (int i = 0; i < 3; ++i) {
        if (g_cancelEffects.load() || !g_isRunning.load()) break;
        TriggerSingleRandomEvent(hInstance, gen);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
    }
}

// 11. Скиллчек в стиле Dead by Daylight (перехват клавиши Пробел)
static HHOOK g_hSkillCheckHook = NULL;
static std::atomic<bool> g_isSkillCheckActive(false);
static std::atomic<bool> g_skillCheckSpacePressed(false);

LRESULT CALLBACK SkillCheckKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isSkillCheckActive.load()) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        if (pKey->vkCode == VK_SPACE) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                g_skillCheckSpacePressed.store(true);
            }
            // Перехватываем Пробел, чтобы он не доходил до других окон
            return 1;
        }
    }
    return CallNextHookEx(g_hSkillCheckHook, nCode, wParam, lParam);
}

void ShowSkillCheck(HINSTANCE hInstance, std::mt19937& gen) {
    bool expected = false;
    if (!g_isEffectActive.compare_exchange_strong(expected, true)) return;

    g_skillCheckSpacePressed.store(false);
    g_isSkillCheckActive.store(true);

    g_hSkillCheckHook = SetWindowsHookExW(WH_KEYBOARD_LL, SkillCheckKeyboardProc, hInstance, 0);

    const wchar_t CLASS_NAME[] = L"DbdSkillCheckClass";
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

    POINT ptCursor;
    GetCursorPos(&ptCursor);

    int winSize = 240;
    int x = ptCursor.x - winSize / 2;
    int y = ptCursor.y - winSize / 2;

    int virtX = GetSystemMetrics(SM_XVIRTUALSCREEN);
    int virtY = GetSystemMetrics(SM_YVIRTUALSCREEN);
    int virtW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    if (x < virtX) x = virtX;
    if (y < virtY) y = virtY;
    if (x + winSize > virtX + virtW) x = virtX + virtW - winSize;
    if (y + winSize > virtY + virtH) y = virtY + virtH - winSize;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"SkillCheck", WS_POPUP | WS_VISIBLE,
        x, y, winSize, winSize,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        if (g_hSkillCheckHook) { UnhookWindowsHookEx(g_hSkillCheckHook); g_hSkillCheckHook = NULL; }
        g_isSkillCheckActive.store(false);
        g_isEffectActive.store(false);
        return;
    }

    SetWindowPos(hwnd, HWND_TOPMOST, x, y, winSize, winSize, SWP_NOACTIVATE | SWP_SHOWWINDOW);

    COLORREF chromaKey = RGB(0, 255, 0);
    SetLayeredWindowAttributes(hwnd, chromaKey, 0, LWA_COLORKEY);

    std::uniform_real_distribution<float> angleDist(90.0f, 200.0f);
    float startZoneAngle = angleDist(gen);
    float zoneWidth = 70.0f;               // Увеличенная зона успеха (70 градусов)
    float endZoneAngle = startZoneAngle + zoneWidth;

    float greatZoneWidth = 20.0f;          // Зона "Отлично!" (20 градусов)
    float startGreatAngle = endZoneAngle - greatZoneWidth;

    float currentAngle = 0.0f;
    float rotationSpeed = 360.0f / 1300.0f; // ~0.277 град/мс

    HDC hdcWin = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWin);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcWin, winSize, winSize);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    HBRUSH greenBrush = CreateSolidBrush(chromaKey);
    HBRUSH darkBgBrush = CreateSolidBrush(RGB(15, 18, 24));
    HBRUSH innerRingBrush = CreateSolidBrush(RGB(24, 28, 36));
    HBRUSH zoneWhiteBrush = CreateSolidBrush(RGB(230, 230, 240));
    HBRUSH zoneRedBrush = CreateSolidBrush(RGB(235, 40, 40));
    HPEN borderPen = CreatePen(PS_SOLID, 2, RGB(80, 80, 95));
    HPEN needlePen = CreatePen(PS_SOLID, 4, RGB(255, 60, 60));

    HFONT hTextFont = CreateFontW(
        22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    Beep(1000, 80);

    bool isHitSuccess = false;
    bool isFinished = false;

    auto lastTime = std::chrono::steady_clock::now();

    while (!isFinished) {
        if (g_cancelEffects.load() || !g_isRunning.load()) {
            break;
        }

        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // Динамически перемещаем окно следом за курсором мыши в реальном времени
        POINT pt;
        GetCursorPos(&pt);
        int curX = pt.x - winSize / 2;
        int curY = pt.y - winSize / 2;

        if (curX < virtX) curX = virtX;
        if (curY < virtY) curY = virtY;
        if (curX + winSize > virtX + virtW) curX = virtX + virtW - winSize;
        if (curY + winSize > virtY + virtH) curY = virtY + virtH - winSize;

        SetWindowPos(hwnd, HWND_TOPMOST, curX, curY, winSize, winSize, SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOZORDER);

        auto now = std::chrono::steady_clock::now();
        float deltaMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        lastTime = now;

        currentAngle += rotationSpeed * deltaMs;

        if (g_skillCheckSpacePressed.load()) {
            if (currentAngle >= startZoneAngle && currentAngle <= endZoneAngle) {
                isHitSuccess = true;
            } else {
                isHitSuccess = false;
            }
            isFinished = true;
            break;
        }

        if (currentAngle >= 360.0f) {
            isHitSuccess = false;
            isFinished = true;
            break;
        }

        RECT fullRect = { 0, 0, winSize, winSize };
        FillRect(hdcMem, &fullRect, greenBrush);

        int cx = winSize / 2;
        int cy = winSize / 2;
        int outerR = 90;
        int innerR = 62;

        HPEN hOldPen = (HPEN)SelectObject(hdcMem, borderPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, darkBgBrush);
        Ellipse(hdcMem, cx - outerR, cy - outerR, cx + outerR, cy + outerR);

        auto getCoord = [cx, cy](float deg, int radius, int& outX, int& outY) {
            float rad = (deg - 90.0f) * 3.14159265f / 180.0f;
            outX = cx + (int)(radius * cosf(rad));
            outY = cy + (int)(radius * sinf(rad));
        };

        int xStart, yStart, xEnd, yEnd;
        getCoord(startZoneAngle, outerR + 5, xStart, yStart);
        getCoord(endZoneAngle, outerR + 5, xEnd, yEnd);
        SelectObject(hdcMem, zoneWhiteBrush);
        Pie(hdcMem, cx - outerR, cy - outerR, cx + outerR, cy + outerR, xStart, yStart, xEnd, yEnd);

        int xGStart, yGStart;
        getCoord(startGreatAngle, outerR + 5, xGStart, yGStart);
        SelectObject(hdcMem, zoneRedBrush);
        Pie(hdcMem, cx - outerR, cy - outerR, cx + outerR, cy + outerR, xGStart, yGStart, xEnd, yEnd);

        SelectObject(hdcMem, innerRingBrush);
        Ellipse(hdcMem, cx - innerR, cy - innerR, cx + innerR, cy + innerR);

        int needleX, needleY;
        getCoord(currentAngle, outerR + 4, needleX, needleY);
        SelectObject(hdcMem, needlePen);
        MoveToEx(hdcMem, cx, cy, NULL);
        LineTo(hdcMem, needleX, needleY);

        SelectObject(hdcMem, zoneRedBrush);
        Ellipse(hdcMem, cx - 8, cy - 8, cx + 8, cy + 8);

        HFONT hOldFont = (HFONT)SelectObject(hdcMem, hTextFont);
        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));
        RECT textRect = { cx - 50, cy - 14, cx + 50, cy + 14 };
        DrawTextW(hdcMem, L"Пробел!", -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        SelectObject(hdcMem, hOldFont);
        SelectObject(hdcMem, hOldPen);
        SelectObject(hdcMem, hOldBrush);

        BitBlt(hdcWin, 0, 0, winSize, winSize, hdcMem, 0, 0, SRCCOPY);

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    DeleteObject(hTextFont);
    DeleteObject(needlePen);
    DeleteObject(borderPen);
    DeleteObject(zoneRedBrush);
    DeleteObject(zoneWhiteBrush);
    DeleteObject(innerRingBrush);
    DeleteObject(darkBgBrush);
    DeleteObject(greenBrush);
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWin);
    DestroyWindow(hwnd);

    if (g_hSkillCheckHook) {
        UnhookWindowsHookEx(g_hSkillCheckHook);
        g_hSkillCheckHook = NULL;
    }
    g_isSkillCheckActive.store(false);
    g_isEffectActive.store(false);

    if (isHitSuccess) {
        Beep(1800, 120);
    } else {
        Beep(300, 250);
        std::thread([hInstance]() {
            std::random_device rd;
            std::mt19937 g(rd());
            TriggerThreeRandomEvents(hInstance, g);
        }).detach();
    }
}

// ============================================================================
// ПОТОК ГЕНЕРАЦИИ СЛУЧАЙНЫХ СОБЫТИЙ И ОБРАБОТКИ ОТЛАДОЧНЫХ КЛАВИШ
// ============================================================================

void EventLoopWorker(HINSTANCE hInstance) {
    std::random_device rd;
    std::mt19937 gen(rd());

    int msCounter = 0;

    while (g_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        msCounter += 50;

#ifdef _DEBUG
        // Горячие клавиши отладки
        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shiftPressed) {
            if (GetAsyncKeyState(VK_F1) & 0x8000) {
                ShakeWindow(GetForegroundWindow(), 2000);
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F2) & 0x8000) {
                std::uniform_int_distribution<int> distInvertDur(7000, 20000);
                InvertScreenOverlay(hInstance, distInvertDur(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F3) & 0x8000) {
                std::uniform_int_distribution<int> distCursorDur(5000, 10000);
                SetHugeCursorForDuration(hInstance, distCursorDur(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F4) & 0x8000) {
                std::uniform_int_distribution<int> distGlitchDur(7000, 20000);
                GlitchScreenOverlay(hInstance, L"VAS VZLAMIVAET JOPA", distGlitchDur(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F5) & 0x8000) {
                Flashbang(hInstance, 2000, gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F6) & 0x8000) {
                PIZDA(hInstance);
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F7) & 0x8000) {
                std::uniform_int_distribution<int> distInvertDur(7000, 20000);
                InvertScreen(distInvertDur(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F8) & 0x8000) {
                ShowScreamerWindow(hInstance, gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F9) & 0x8000) {
                PlayLongSound(hInstance, gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F10) & 0x8000) {
                std::uniform_int_distribution<int> distZoomDur(5000, 15000);
                ZoomScreen(2.0f, distZoomDur(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F11) & 0x8000) {
                std::uniform_int_distribution<int> distStripeDur(7000, 15000);
                BlackStripeOverlay(hInstance, distStripeDur(gen));
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F12) & 0x8000) {
                ShowSkillCheck(hInstance, gen);
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
        }
#endif

        // Проверка возникновения случайных событий раз в 1 секунду
        if (msCounter >= 1000) {
            msCounter = 0;

            if (!g_isEnabled.load() || g_isEffectActive.load()) {
                continue;
            }

            int modeIdx = (int)g_currentMode.load();
            if (modeIdx < 0 || modeIdx > 4) modeIdx = 1;

            const ModeChances& current = G_MODE_CHANCES[modeIdx];

            // 1. Проверка на флешку
            std::uniform_int_distribution<int> distFlash(1, current.flashbang);
            if (distFlash(gen) == 1) {
                Flashbang(hInstance, 2000, gen);
                continue;
            }

            // 2. Проверка на скример
            std::uniform_int_distribution<int> distScream(1, current.screamer);
            if (distScream(gen) == 1) {
                ShowScreamerWindow(hInstance, gen);
                continue;
            }

            // 3. Проверка на длинный звук
            std::uniform_int_distribution<int> distLong(1, current.longSound);
            if (distLong(gen) == 1) {
                PlayLongSound(hInstance, gen);
                continue;
            }

            // 4. Проверка на помехи (Glitch: длительность 7 - 20 секунд)
            std::uniform_int_distribution<int> distGlitch(1, current.glitch);
            if (distGlitch(gen) == 1) {
                std::uniform_int_distribution<int> distGlitchDur(7000, 20000);
                GlitchScreenOverlay(hInstance, L"VAS VZLAMIVAET JOPA", distGlitchDur(gen));
                continue;
            }

            // 5. Проверка на дрожание окна (Shake)
            std::uniform_int_distribution<int> distShake(1, current.shake);
            if (distShake(gen) == 1) {
                ShakeWindow(GetForegroundWindow(), 2000);
                continue;
            }

            // 6. Проверка на инверсию экрана (длительность 7 - 20 секунд)
            std::uniform_int_distribution<int> distInvert(1, current.invert);
            if (distInvert(gen) == 1) {
                std::uniform_int_distribution<int> distInvertDur(7000, 20000);
                InvertScreenOverlay(hInstance, distInvertDur(gen));
                continue;
            }

            // 7. Проверка на гигантский курсор (длительность 5 - 10 секунд)
            std::uniform_int_distribution<int> distCursor(1, current.giantCursor);
            if (distCursor(gen) == 1) {
                std::uniform_int_distribution<int> distCursorDur(5000, 10000);
                SetHugeCursorForDuration(hInstance, distCursorDur(gen));
                continue;
            }

            // 8. Проверка на сворачивание окон (Win+D)
            std::uniform_int_distribution<int> distPizda(1, current.pizda);
            if (distPizda(gen) == 1) {
                PIZDA(hInstance);
                continue;
            }

            // 9. Проверка на зум 2x (длительность 5 - 15 секунд)
            std::uniform_int_distribution<int> distZoom(1, current.zoom);
            if (distZoom(gen) == 1) {
                std::uniform_int_distribution<int> distZoomDur(5000, 15000);
                ZoomScreen(2.0f, distZoomDur(gen));
                continue;
            }

            // 10. Проверка на черную вертикальную полоску (длительность 7 - 15 секунд)
            std::uniform_int_distribution<int> distStripe(1, current.stripe);
            if (distStripe(gen) == 1) {
                std::uniform_int_distribution<int> distStripeDur(7000, 15000);
                BlackStripeOverlay(hInstance, distStripeDur(gen));
                continue;
            }

            // 11. Проверка на скиллчек DBD
            std::uniform_int_distribution<int> distSkill(1, current.skillCheck);
            if (distSkill(gen) == 1) {
                ShowSkillCheck(hInstance, gen);
                continue;
            }
        }
    }
}

// ============================================================================
// ГРАФИЧЕСКИЙ ИНТЕРФЕЙС (GUI НАСТРОЕК)
// ============================================================================

void UpdateStatusText() {
    if (!g_hwndStatus) return;

    int modeIdx = (int)g_currentMode.load();
    const ModeChances& current = G_MODE_CHANCES[modeIdx];

    wchar_t buf[512];
    swprintf_s(buf,
        L"%s\n"
        L"⚡ Флешка: 1/%d | Скример: 1/%d | Дл.звук: 1/%d | 🎯 Скиллчек: 1/%d\n"
        L"🌀 Помехи: 1/%d | Дрожание: 1/%d | Курсор: 1/%d | 🔍 Зум: 1/%d | █ Полоса: 1/%d",
        current.description,
        current.flashbang, current.screamer, current.longSound, current.skillCheck,
        current.glitch, current.shake, current.giantCursor, current.zoom, current.stripe
    );

    SetWindowTextW(g_hwndStatus, buf);
}

LRESULT CALLBACK GuiWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        HINSTANCE hInstance = ((LPCREATESTRUCT)lParam)->hInstance;

        g_hDarkBrush = CreateSolidBrush(RGB(24, 28, 36));
        g_hGuiFont = CreateFontW(
            16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );

        // 1. Главный заголовок
        HWND hTitle = CreateWindowW(L"STATIC", L"НАСТРОЙКИ СОБЫТИЙ",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            20, 12, 340, 22, hwnd, NULL, hInstance, NULL);

        HFONT hTitleFont = CreateFontW(
            18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );
        SendMessageW(hTitle, WM_SETFONT, (WPARAM)hTitleFont, TRUE);

        // 2. Огромный тумблер переключения (ВКЛ / ВЫКЛ)
        CreateWindowW(L"BUTTON", L"ВКЛ/ВЫКЛ",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            20, 38, 340, 52, hwnd, (HMENU)IDC_TOGGLE_BTN, hInstance, NULL);

        // 3. Подзаголовок выбора частоты
        HWND hSec = CreateWindowW(L"STATIC", L"Частота возникновения событий:",
            WS_VISIBLE | WS_CHILD,
            25, 98, 330, 18, hwnd, NULL, hInstance, NULL);
        SendMessageW(hSec, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);

        // 4. Переключатели интенсивности (Radio Buttons)
        HWND r1 = CreateWindowW(L"BUTTON", L"🟢 Низкая",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_GROUP,
            30, 118, 320, 22, hwnd, (HMENU)IDC_RADIO_LOW, hInstance, NULL);

        HWND r2 = CreateWindowW(L"BUTTON", L"🟡 Средняя",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            30, 142, 320, 22, hwnd, (HMENU)IDC_RADIO_MED, hInstance, NULL);

        HWND r3 = CreateWindowW(L"BUTTON", L"🟠 Высокая (Шанс флешки 1/50)",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            30, 166, 320, 22, hwnd, (HMENU)IDC_RADIO_HIGH, hInstance, NULL);

        HWND r4 = CreateWindowW(L"BUTTON", L"💀 СМЕРТЬ (Шанс флешки 1/25)",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            30, 190, 320, 22, hwnd, (HMENU)IDC_RADIO_DEATH, hInstance, NULL);

        HWND r5 = CreateWindowW(L"BUTTON", L"💀💀💀 СМЕРТЬ++",
            WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON,
            30, 214, 320, 22, hwnd, (HMENU)IDC_RADIO_DEATHPLUS, hInstance, NULL);

        SendMessageW(r1, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);
        SendMessageW(r2, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);
        SendMessageW(r3, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);
        SendMessageW(r4, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);
        SendMessageW(r5, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);

        CheckRadioButton(hwnd, IDC_RADIO_LOW, IDC_RADIO_DEATHPLUS, IDC_RADIO_MED);

        // 5. Описание шансов и текущего режима
        g_hwndStatus = CreateWindowW(L"STATIC", L"",
            WS_VISIBLE | WS_CHILD | SS_LEFT,
            20, 240, 340, 60, hwnd, (HMENU)IDC_STATUS_LABEL, hInstance, NULL);
        SendMessageW(g_hwndStatus, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);

        // 6. Кнопка сброса/завершения всех эффектов
        CreateWindowW(L"BUTTON", L"🛑 СБРОСИТЬ ВСЕ ЭФФЕКТЫ",
            WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_TABSTOP,
            20, 305, 340, 38, hwnd, (HMENU)IDC_BTN_CANCEL_EFFECT, hInstance, NULL);

#ifdef _DEBUG
        // 7. Кнопки отладки в GUI (только в Debug сборке)
        HWND hDbgLabel = CreateWindowW(L"STATIC", L"⚡ Тест событий (Debug):",
            WS_VISIBLE | WS_CHILD,
            20, 350, 340, 18, hwnd, NULL, hInstance, NULL);
        SendMessageW(hDbgLabel, WM_SETFONT, (WPARAM)g_hGuiFont, TRUE);

        HFONT hBtnFont = CreateFontW(
            13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
        );

        HWND b1 = CreateWindowW(L"BUTTON", L"🔊 Звук", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            20, 370, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_LONGSOUND, hInstance, NULL);
        HWND b2 = CreateWindowW(L"BUTTON", L"😱 Скример", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            133, 370, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_SCREAMER, hInstance, NULL);
        HWND b3 = CreateWindowW(L"BUTTON", L"⚡ Флешка", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            246, 370, 111, 26, hwnd, (HMENU)IDC_BTN_TEST_FLASHBANG, hInstance, NULL);

        HWND b4 = CreateWindowW(L"BUTTON", L"🌀 Помехи", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            20, 400, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_GLITCH, hInstance, NULL);
        HWND b5 = CreateWindowW(L"BUTTON", L"📳 Дрожание", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            133, 400, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_SHAKE, hInstance, NULL);
        HWND b6 = CreateWindowW(L"BUTTON", L"🔄 Инверсия", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            246, 400, 111, 26, hwnd, (HMENU)IDC_BTN_TEST_INVERT, hInstance, NULL);

        HWND b7 = CreateWindowW(L"BUTTON", L"🖱️ Курсор", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            20, 430, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_CURSOR, hInstance, NULL);
        HWND b8 = CreateWindowW(L"BUTTON", L"🖥️ Win+D", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            133, 430, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_PIZDA, hInstance, NULL);
        HWND b9 = CreateWindowW(L"BUTTON", L"🔍 Зум 2x", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            246, 430, 111, 26, hwnd, (HMENU)IDC_BTN_TEST_ZOOM, hInstance, NULL);

        HWND b10 = CreateWindowW(L"BUTTON", L"█ Полоса", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            20, 460, 163, 26, hwnd, (HMENU)IDC_BTN_TEST_STRIPE, hInstance, NULL);
        HWND b11 = CreateWindowW(L"BUTTON", L"🎯 Скиллчек", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            191, 460, 166, 26, hwnd, (HMENU)IDC_BTN_TEST_SKILLCHECK, hInstance, NULL);

        SendMessageW(b1, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b2, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b3, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b4, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b5, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b6, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b7, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b8, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b9, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b10, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b11, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
#endif

        UpdateStatusText();
        return 0;
    }

    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_TOGGLE_BTN:
            g_isEnabled.store(!g_isEnabled.load());
            InvalidateRect(GetDlgItem(hwnd, IDC_TOGGLE_BTN), NULL, TRUE);
            break;

        case IDC_RADIO_LOW:
            g_currentMode.store(MODE_LOW);
            UpdateStatusText();
            break;
        case IDC_RADIO_MED:
            g_currentMode.store(MODE_MEDIUM);
            UpdateStatusText();
            break;
        case IDC_RADIO_HIGH:
            g_currentMode.store(MODE_HIGH);
            UpdateStatusText();
            break;
        case IDC_RADIO_DEATH:
            g_currentMode.store(MODE_DEATH);
            UpdateStatusText();
            break;

        case IDC_RADIO_DEATHPLUS:
            g_currentMode.store(MODE_DEATHPLUS);
            UpdateStatusText();
            break;

        case IDC_BTN_CANCEL_EFFECT:
            std::thread(CancelAllEffects).detach();
            break;

#ifdef _DEBUG
        case IDC_BTN_TEST_LONGSOUND:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                PlayLongSound(hInst, g);
            }).detach();
            break;

        case IDC_BTN_TEST_SCREAMER:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                ShowScreamerWindow(hInst, g);
            }).detach();
            break;

        case IDC_BTN_TEST_FLASHBANG:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                Flashbang(hInst, 2000, g);
            }).detach();
            break;

        case IDC_BTN_TEST_GLITCH:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                std::uniform_int_distribution<int> distGlitchDur(7000, 20000);
                GlitchScreenOverlay(hInst, L"VAS VZLAMIVAET JOPA", distGlitchDur(g));
            }).detach();
            break;

        case IDC_BTN_TEST_SHAKE:
            std::thread([]() {
                ShakeWindow(GetForegroundWindow(), 2000);
            }).detach();
            break;

        case IDC_BTN_TEST_INVERT:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                std::uniform_int_distribution<int> distInvertDur(7000, 20000);
                InvertScreenOverlay(hInst, distInvertDur(g));
            }).detach();
            break;

        case IDC_BTN_TEST_CURSOR:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                std::uniform_int_distribution<int> distCursorDur(5000, 10000);
                SetHugeCursorForDuration(hInst, distCursorDur(g));
            }).detach();
            break;

        case IDC_BTN_TEST_PIZDA:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                PIZDA(hInst);
            }).detach();
            break;

        case IDC_BTN_TEST_ZOOM:
            std::thread([]() {
                std::random_device rd; std::mt19937 g(rd());
                std::uniform_int_distribution<int> distZoomDur(5000, 15000);
                ZoomScreen(2.0f, distZoomDur(g));
            }).detach();
            break;

        case IDC_BTN_TEST_STRIPE:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                std::uniform_int_distribution<int> distDur(7000, 15000);
                BlackStripeOverlay(hInst, distDur(g));
            }).detach();
            break;

        case IDC_BTN_TEST_SKILLCHECK:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                ShowSkillCheck(hInst, g);
            }).detach();
            break;
#endif
        }
        return 0;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;
        if (pDIS->CtlID == IDC_TOGGLE_BTN) {
            HDC hdc = pDIS->hDC;
            RECT rect = pDIS->rcItem;
            bool enabled = g_isEnabled.load();

            COLORREF bgCol = enabled ? RGB(34, 139, 34) : RGB(178, 34, 34);
            COLORREF borderCol = enabled ? RGB(60, 220, 60) : RGB(230, 50, 50);

            HBRUSH hBrush = CreateSolidBrush(bgCol);
            HPEN hPen = CreatePen(PS_SOLID, 2, borderCol);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 16, 16);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hBrush);
            DeleteObject(hPen);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));

            HFONT hBtnFont = CreateFontW(
                20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );
            HFONT hOldFont = (HFONT)SelectObject(hdc, hBtnFont);

            const wchar_t* btnText = enabled ? L"⚡ ВКЛЮЧЕНО (ON) ⚡" : L"⛔ ВЫКЛЮЧЕНО (OFF) ⛔";
            DrawTextW(hdc, btnText, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
            DeleteObject(hBtnFont);
            return TRUE;
        }
        else if (pDIS->CtlID == IDC_BTN_CANCEL_EFFECT) {
            HDC hdc = pDIS->hDC;
            RECT rect = pDIS->rcItem;

            COLORREF bgCol = RGB(160, 30, 30);
            COLORREF borderCol = RGB(220, 50, 50);

            HBRUSH hBrush = CreateSolidBrush(bgCol);
            HPEN hPen = CreatePen(PS_SOLID, 2, borderCol);
            HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            RoundRect(hdc, rect.left, rect.top, rect.right, rect.bottom, 12, 12);

            SelectObject(hdc, hOldBrush);
            SelectObject(hdc, hOldPen);
            DeleteObject(hBrush);
            DeleteObject(hPen);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));

            HFONT hBtnFont = CreateFontW(
                16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
            );
            HFONT hOldFont = (HFONT)SelectObject(hdc, hBtnFont);

            DrawTextW(hdc, L"🛑 СБРОСИТЬ ВСЕ ЭФФЕКТЫ", -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

            SelectObject(hdc, hOldFont);
            DeleteObject(hBtnFont);
            return TRUE;
        }
        break;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        HWND hwndStatic = (HWND)lParam;
        int controlId = GetDlgCtrlID(hwndStatic);

        SetBkMode(hdcStatic, TRANSPARENT);

        if (controlId == IDC_RADIO_DEATH) {
            SetTextColor(hdcStatic, RGB(255, 90, 90));
        } else if (controlId == IDC_RADIO_DEATHPLUS) {
            SetTextColor(hdcStatic, RGB(255, 40, 40));
        } else if (controlId == IDC_STATUS_LABEL) {
            SetTextColor(hdcStatic, RGB(180, 220, 255));
        } else {
            SetTextColor(hdcStatic, RGB(230, 230, 230));
        }

        return (INT_PTR)g_hDarkBrush;
    }

    case WM_CTLCOLORBTN:
        return (INT_PTR)g_hDarkBrush;

    case WM_DESTROY:
        if (g_hDarkBrush) DeleteObject(g_hDarkBrush);
        if (g_hGuiFont) DeleteObject(g_hGuiFont);
        g_isRunning.store(false);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// ============================================================================
// ТОЧКА ВХОДА (MAIN)
// ============================================================================

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Включаем Per-Monitor V2 DPI Awareness для точных физических координат мыши и окон
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        typedef BOOL(WINAPI* PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
        auto pfnSetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pfnSetProcessDpiAwarenessContext) {
            pfnSetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        } else {
            SetProcessDPIAware();
        }
    }

    // Всегда скрываем консоль (и в Debug, и в Release), чтобы не всплывало лишнее окно
    HWND hwndConsole = GetConsoleWindow();
    if (hwndConsole) ShowWindow(hwndConsole, SW_HIDE);

    // Запускаем фоновый поток обработки событий
    std::thread workerThread(EventLoopWorker, hInstance);

    // Регистрация класса окна GUI
    const wchar_t GUI_CLASS[] = L"PrankControlPanelClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = GuiWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = GUI_CLASS;
    wc.hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_ICON1));
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(RGB(24, 28, 36));

    RegisterClassW(&wc);

    int winWidth = 395;
#ifdef _DEBUG
    int winHeight = 540;
#else
    int winHeight = 395;
#endif
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwndGui = CreateWindowExW(
        0,
        GUI_CLASS,
        L"Панель Управления Событиями",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE,
        (screenW - winWidth) / 2, (screenH - winHeight) / 2, winWidth, winHeight,
        NULL, NULL, hInstance, NULL
    );

    // Цикл сообщений GUI
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Завершение работы фонового потока
    g_isRunning.store(false);
    if (workerThread.joinable()) {
        workerThread.join();
    }

    return 0;
}
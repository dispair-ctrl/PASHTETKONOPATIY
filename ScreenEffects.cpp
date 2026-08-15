#include "Effects.h"
#include <magnification.h>
#include <thread>
#include <chrono>

#pragma comment(lib, "magnification.lib")

// ----------------------------------------------------------------------------
// 1. ИНВЕРСИЯ ЭКРАНА (с поддержкой записи экрана OBS / Discord / Game Bar)
// ----------------------------------------------------------------------------

void InvertScreenOverlay(HINSTANCE hInstance, int durationMs) {
    bool magInitSuccess = false;
    if (MagInitialize()) {
        MAGCOLOREFFECT invertMatrix = {
            -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
             0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f
        };
        MagSetFullscreenColorEffect(&invertMatrix);
        magInitSuccess = true;
    }

    if (magInitSuccess) {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() < durationMs) {
            if (g_cancelEffects.load()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        MAGCOLOREFFECT identityMatrix = {
             1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  1.0f,  0.0f,
             0.0f,  0.0f,  0.0f,  0.0f,  1.0f
        };
        MagSetFullscreenColorEffect(&identityMatrix);
        MagUninitialize();
    }
}

// ----------------------------------------------------------------------------
// 2. ГЛИТЧИ / ПОМЕХИ НА ЭКРАНЕ И BSOD
// ----------------------------------------------------------------------------

void ShowBSODOverlay(HINSTANCE hInstance, int durationMs) {
    if (g_isEffectActive.exchange(true)) return;

    std::wstring bsodPath = L"C:\\Users\\dispair\\source\\repos\\Project1\\bsod.jpg";
    Gdiplus::Bitmap* pBmp = NULL;

    if (GetFileAttributesW(bsodPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        pBmp = Gdiplus::Bitmap::FromFile(bsodPath.c_str());
    }
    if (!pBmp || pBmp->GetLastStatus() != Gdiplus::Ok) {
        if (pBmp) delete pBmp;
        pBmp = NULL;

        wchar_t curDir[MAX_PATH];
        if (GetCurrentDirectoryW(MAX_PATH, curDir) > 0) {
            std::wstring path = std::wstring(curDir) + L"\\bsod.jpg";
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
                pBmp = Gdiplus::Bitmap::FromFile(path.c_str());
            }
        }
    }
    if (!pBmp || pBmp->GetLastStatus() != Gdiplus::Ok) {
        if (pBmp) delete pBmp;
        pBmp = NULL;

        for (const auto& folder : GetAllDesktopFolderPaths()) {
            std::wstring path = folder + L"\\bsod.jpg";
            if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
                pBmp = Gdiplus::Bitmap::FromFile(path.c_str());
                if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) break;
            }
        }
    }
    if (!pBmp || pBmp->GetLastStatus() != Gdiplus::Ok) {
        if (pBmp) delete pBmp;
        pBmp = NULL;
        // Загрузка из ресурсов .exe (IDR_BSOD_JPG = 504)
        HBITMAP hBmpRes = LoadImageFromResource(hInstance, 504);
        if (hBmpRes) {
            pBmp = Gdiplus::Bitmap::FromHBITMAP(hBmpRes, NULL);
            DeleteObject(hBmpRes);
        }
    }

    const wchar_t CLASS_NAME[] = L"BSODOverlayClass";
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
    int virtW = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    int virtH = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        CLASS_NAME, L"BSOD", WS_POPUP | WS_VISIBLE,
        virtX, virtY, virtW, virtH,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd) {
        HDC hdcWin = GetDC(hwnd);
        Gdiplus::Graphics graphics(hdcWin);
        graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

        if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) {
            graphics.DrawImage(pBmp, (INT)0, (INT)0, (INT)virtW, (INT)virtH);
        } else {
            HBRUSH blueBrush = CreateSolidBrush(RGB(0, 120, 215));
            RECT fullRect = { 0, 0, virtW, virtH };
            FillRect(hdcWin, &fullRect, blueBrush);
            DeleteObject(blueBrush);

            SetBkMode(hdcWin, TRANSPARENT);
            SetTextColor(hdcWin, RGB(255, 255, 255));
            HFONT hFont = CreateFontW(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT oF = (HFONT)SelectObject(hdcWin, hFont);
            RECT textR = { 100, 100, virtW - 100, virtH - 100 };
            DrawTextW(hdcWin, L":( Your PC ran into a problem and needs to restart.", -1, &textR, DT_LEFT | DT_WORDBREAK);
            SelectObject(hdcWin, oF);
            DeleteObject(hFont);
        }

        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() < durationMs) {
            if (g_cancelEffects.load()) break;

            MSG msg;
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        ReleaseDC(hwnd, hdcWin);
        DestroyWindow(hwnd);
    }

    if (pBmp) delete pBmp;
    g_isEffectActive.store(false);
}

void GlitchScreenOverlay(HINSTANCE hInstance, const std::wstring& overlayText, int durationMs) {
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
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"GlitchOverlay", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return;

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    HDC hdcWindow = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWindow);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcWindow, width, height);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RECT fullScreenRect = { 0, 0, width, height };

    HFONT hFontMain = CreateFontW(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Impact");
    HFONT hFontSub = CreateFontW(36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Consolas");

    const wchar_t* glitchTextPool[] = {
        L"SYSTEM FAILURE - CORRUPTED MEMORY",
        L"FATAL EXCEPTION AT 0x0000007B",
        L"VAS VZLAMIVAET JOPA",
        L"CRITICAL ERROR: DATA LOSS IMMINENT",
        L"HARDWARE FAULT DETECTED",
        L"HARD DISK OVERFLOW [99.9%]"
    };

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < durationMs) {

        if (g_cancelEffects.load()) break;

        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        FillRect(hdcMem, &fullScreenRect, blackBrush);

        // 1. Срез экрана (Screen Tearing / Displacement)
        HDC hdcScreen = GetDC(NULL);
        if (hdcScreen) {
            int sliceCount = rand() % 15 + 10;
            for (int i = 0; i < sliceCount; ++i) {
                int sliceY = rand() % (height - 30);
                int sliceH = rand() % 40 + 5;
                int shiftX = (rand() % 120) - 60;

                BitBlt(hdcMem, shiftX, sliceY, width, sliceH, hdcScreen, x, y + sliceY, SRCCOPY);
            }
            ReleaseDC(NULL, hdcScreen);
        }

        // 2. Яркие цветовые блоки и шумы
        int blockCount = rand() % 35 + 20;
        for (int i = 0; i < blockCount; ++i) {
            int rx = rand() % width;
            int ry = rand() % height;
            int rw = rand() % 500 + 80;
            int rh = rand() % 80 + 10;

            BYTE r = (rand() % 2 == 0) ? 255 : (rand() % 255);
            BYTE g = (rand() % 2 == 0) ? 0 : (rand() % 255);
            BYTE b = (rand() % 2 == 0) ? 255 : (rand() % 255);

            HBRUSH brush = CreateSolidBrush(RGB(r, g, b));
            RECT rect = { rx, ry, rx + rw, ry + rh };
            FillRect(hdcMem, &rect, brush);
            DeleteObject(brush);
        }

        // 3. Глитч Текст и Артефакты (Red/Cyan Offset)
        SetBkMode(hdcMem, TRANSPARENT);
        int textOffset = (rand() % 20) - 10;

        HFONT hOldF = (HFONT)SelectObject(hdcMem, hFontMain);

        SetTextColor(hdcMem, RGB(0, 255, 255));
        RECT r1 = { textOffset - 4, 60, width + textOffset - 4, 180 };
        DrawTextW(hdcMem, overlayText.c_str(), -1, &r1, DT_CENTER | DT_SINGLELINE | DT_NOCLIP);

        SetTextColor(hdcMem, RGB(255, 0, 0));
        RECT r2 = { textOffset + 4, 60, width + textOffset + 4, 180 };
        DrawTextW(hdcMem, overlayText.c_str(), -1, &r2, DT_CENTER | DT_SINGLELINE | DT_NOCLIP);

        SelectObject(hdcMem, hFontSub);
        const wchar_t* subT = glitchTextPool[rand() % 6];
        SetTextColor(hdcMem, RGB(255, 255, 0));
        RECT r3 = { 0, height - 120 + (rand() % 10 - 5), width, height - 40 };
        DrawTextW(hdcMem, subT, -1, &r3, DT_CENTER | DT_SINGLELINE | DT_NOCLIP);

        SelectObject(hdcMem, hOldF);

        BitBlt(hdcWindow, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
    }

    DeleteObject(hFontMain);
    DeleteObject(hFontSub);
    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWindow);
    DestroyWindow(hwnd);

    // 30% шанс вызова BSOD по завершению глитча!
    if (!g_cancelEffects.load()) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::uniform_int_distribution<int> dist30(0, 99);
        if (dist30(g) < 30) {
            ShowBSODOverlay(hInstance, 5000);
        }
    }
}

// ----------------------------------------------------------------------------
// 3. ЧЕРНЫЕ ПОЛОСЫ НА ЭКРАНЕ
// ----------------------------------------------------------------------------

void BlackStripeOverlay(HINSTANCE hInstance, int durationMs) {
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

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"BlackStripeOverlay", WS_POPUP | WS_VISIBLE,
        virtX, virtY, virtWidth, virtHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return;

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

    struct MonData { RECT rc; };
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
        int spd = (rand() % 26) + 6;
        if (rand() % 2 == 0) spd = -spd;
        return (float)spd;
    };

    struct MonitorStripes {
        int localLeft, localTop, localRight, localBottom;
        int monWidth, monHeight;
        int vWidth;
        float vPosX;
        float vSpeedX;
        int hHeight;
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

        s.vWidth = rand() % 21 + 35;
        int maxVX = s.monWidth - s.vWidth;
        s.vPosX = (float)(rand() % (maxVX > 0 ? maxVX : 1));
        s.vSpeedX = pickRandomSpeed();

        s.hHeight = rand() % 21 + 35;
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
        FillRect(hdcMem, &fullScreenRect, greenBrush);

        for (auto& s : stripesList) {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(now - s.lastSpeedChange).count() >= 1000) {
                s.vSpeedX = pickRandomSpeed();
                s.hSpeedY = pickRandomSpeed();
                s.lastSpeedChange = now;
            }

            s.vPosX += s.vSpeedX;
            if (s.vPosX < 0) {
                s.vPosX = 0;
                s.vSpeedX = -s.vSpeedX;
            } else if (s.vPosX + s.vWidth > s.monWidth) {
                s.vPosX = (float)(s.monWidth - s.vWidth);
                s.vSpeedX = -s.vSpeedX;
            }

            s.hPosY += s.hSpeedY;
            if (s.hPosY < 0) {
                s.hPosY = 0;
                s.hSpeedY = -s.hSpeedY;
            } else if (s.hPosY + s.hHeight > s.monHeight) {
                s.hPosY = (float)(s.monHeight - s.hHeight);
                s.hSpeedY = -s.hSpeedY;
            }

            HBRUSH vBrush = (rand() % 2 == 0) ? blackBrush : whiteBrush;
            HBRUSH hBrush = (rand() % 2 == 0) ? blackBrush : whiteBrush;

            RECT vRect = {
                s.localLeft + (int)s.vPosX,
                s.localTop,
                s.localLeft + (int)s.vPosX + s.vWidth,
                s.localBottom
            };
            FillRect(hdcMem, &vRect, vBrush);

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
}

// ----------------------------------------------------------------------------
// 4. ЗУМ ЭКРАНА (2x)
// ----------------------------------------------------------------------------

void ZoomScreen(float magFactor, int durationMs) {
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
}

// ----------------------------------------------------------------------------
// 5. ФЛЕШКА (ВСПЫШКА)
// ----------------------------------------------------------------------------

static LRESULT CALLBACK FlashWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"Flash", WS_POPUP | WS_VISIBLE,
        x, y, width, height,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) return;

    if (!FLASHBANG_SOUNDS.empty()) {
        std::uniform_int_distribution<size_t> sndDist(0, FLASHBANG_SOUNDS.size() - 1);
        WORD flashSoundId = FLASHBANG_SOUNDS[sndDist(gen)];
        PlaySoundW(MAKEINTRESOURCE(flashSoundId), hInstance, SND_RESOURCE | SND_ASYNC);
    }

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    UpdateWindow(hwnd);
    RedrawWindow(hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);

    int holdTimeMs = 350;
    for (int elapsed = 0; elapsed < holdTimeMs; elapsed += 30) {
        if (g_cancelEffects.load()) { DestroyWindow(hwnd); return; }
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
}

// ----------------------------------------------------------------------------
// 6. СКРИМЕР ОКНО
// ----------------------------------------------------------------------------

static LRESULT CALLBACK ScreamerWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static HBITMAP hScreamerBitmap = NULL;

    switch (uMsg) {
    case WM_CREATE:
        if (selectedImageId != 0) {
            hScreamerBitmap = LoadScreamerImage(GetModuleHandle(NULL), selectedImageId);
        }
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
    if (!IMAGES.empty()) {
        std::uniform_int_distribution<size_t> imgDist(0, IMAGES.size() - 1);
        selectedImageId = IMAGES[imgDist(gen)];
    } else {
        selectedImageId = 0;
    }

    if (!SCREAMER_SOUNDS.empty()) {
        std::uniform_int_distribution<size_t> sndDist(0, SCREAMER_SOUNDS.size() - 1);
        selectedSoundId = SCREAMER_SOUNDS[sndDist(gen)];
    } else {
        selectedSoundId = 0;
    }

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
        if (selectedSoundId != 0) {
            PlaySoundW(MAKEINTRESOURCE(selectedSoundId), hInstance, SND_RESOURCE | SND_ASYNC);
        }


        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (g_cancelEffects.load()) { DestroyWindow(hwnd); break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    UnregisterClassW(CLASS_NAME, hInstance);
}

// ----------------------------------------------------------------------------
// 7. ВЫЗОВ СЛУЧАЙНОГО ЭКРАННОГО ЭФФЕКТА
// ----------------------------------------------------------------------------

void TriggerRandomScreenEffect(HINSTANCE hInstance, std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, 6);
    int choice = dist(gen);
    switch (choice) {
    case 0: {
        std::uniform_int_distribution<int> distInvertDur(7000, 20000);
        InvertScreenOverlay(hInstance, distInvertDur(gen));
        break;
    }
    case 1: {
        std::uniform_int_distribution<int> distGlitchDur(7000, 20000);
        GlitchScreenOverlay(hInstance, L"VAS VZLAMIVAET JOPA", distGlitchDur(gen));
        break;
    }
    case 2: {
        std::uniform_int_distribution<int> distStripeDur(7000, 15000);
        BlackStripeOverlay(hInstance, distStripeDur(gen));
        break;
    }
    case 3: {
        std::uniform_int_distribution<int> distZoomDur(5000, 15000);
        ZoomScreen(2.0f, distZoomDur(gen));
        break;
    }
    case 4:
        Flashbang(hInstance, 2000, gen);
        break;
    case 5:
        ShowScreamerWindow(hInstance, gen);
        break;
    case 6:
        LowQualityScreenEffect(10000);
        break;
    }
}

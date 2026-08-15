#include "SkillCheckEffect.h"
#include <thread>
#include <chrono>
#include <cmath>

static HHOOK g_hSkillCheckHook = NULL;
static std::atomic<bool> g_isSkillCheckActive(false);
static std::atomic<bool> g_skillCheckSpacePressed(false);

static LRESULT CALLBACK SkillCheckKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && g_isSkillCheckActive.load()) {
        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        if (pKey->vkCode == VK_SPACE) {
            if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
                g_skillCheckSpacePressed.store(true);
            }
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
    float zoneWidth = 70.0f;
    float endZoneAngle = startZoneAngle + zoneWidth;

    float greatZoneWidth = 20.0f;
    float startGreatAngle = endZoneAngle - greatZoneWidth;

    float currentAngle = 0.0f;
    float rotationSpeed = 360.0f / 1300.0f;

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
        // Запускаем 3 случайных эффекта сразу из всех трех разделов одновременно!
        std::thread([hInstance]() {
            std::random_device rd;
            std::mt19937 g(rd());
            TriggerThreeCategoryPunishment(hInstance, g);
        }).detach();
    }
}

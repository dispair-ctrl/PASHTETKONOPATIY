#include "GUI.h"
#include <thread>

void UpdateStatusText() {
    if (!g_hwndStatus) return;

    int modeIdx = (int)g_currentMode.load();
    const ModeChances& current = G_MODE_CHANCES[modeIdx];

    wchar_t buf[512];
    swprintf_s(buf,
        L"%s\n"
        L"⚡ Флешка: 1/%d | Скример: 1/%d | Дл.звук: 1/%d | 🎯 Скиллчек: 1/%d\n"
        L"🌀 Помехи: 1/%d | Дрожание: 1/%d | Курсор: 1/%d | 🔍 Зум: 1/%d | █ Полоса: 1/%d | 🤖 Шкатулка: 1/%d | 📺 Реклама: 1/%d",
        current.description,
        current.flashbang, current.screamer, current.longSound, current.skillCheck,
        current.glitch, current.shake, current.giantCursor, current.zoom, current.stripe,
        current.marionette, current.ads
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
            20, 460, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_STRIPE, hInstance, NULL);
        HWND b11 = CreateWindowW(L"BUTTON", L"🎯 Скиллчек", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            133, 460, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_SKILLCHECK, hInstance, NULL);
        HWND b12 = CreateWindowW(L"BUTTON", L"🤖 Шкатулка", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            246, 460, 111, 26, hwnd, (HMENU)IDC_BTN_TEST_MARIONETTE, hInstance, NULL);

        HWND b13 = CreateWindowW(L"BUTTON", L"📺 Реклама", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            20, 490, 337, 26, hwnd, (HMENU)IDC_BTN_TEST_ADS, hInstance, NULL);

        HWND b14 = CreateWindowW(L"BUTTON", L"🖥️ 720p", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            20, 520, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_LOWQUAL, hInstance, NULL);
        HWND b15 = CreateWindowW(L"BUTTON", L"⚡ Мышка", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            133, 520, 108, 26, hwnd, (HMENU)IDC_BTN_TEST_EPILEMOUSE, hInstance, NULL);
        HWND b16 = CreateWindowW(L"BUTTON", L"🧩 Капча", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
            246, 520, 111, 26, hwnd, (HMENU)IDC_BTN_TEST_CAPTCHA, hInstance, NULL);

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
        SendMessageW(b12, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b13, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b14, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b15, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
        SendMessageW(b16, WM_SETFONT, (WPARAM)hBtnFont, TRUE);
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

        case IDC_BTN_TEST_MARIONETTE:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                ShowMarionetteEffect(hInst, g);
            }).detach();
            break;

        case IDC_BTN_TEST_ADS:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                ShowAdsPopup(hInst, g);
            }).detach();
            break;

        case IDC_BTN_TEST_LOWQUAL:
            std::thread([]() {
                LowQualityScreenEffect(10000);
            }).detach();
            break;

        case IDC_BTN_TEST_EPILEMOUSE:
            std::thread([]() {
                EpilepticMouseEffect(2500);
            }).detach();
            break;

        case IDC_BTN_TEST_CAPTCHA:
            std::thread([]() {
                HINSTANCE hInst = GetModuleHandle(NULL);
                std::random_device rd; std::mt19937 g(rd());
                ShowKeyboardCaptcha(hInst, g);
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

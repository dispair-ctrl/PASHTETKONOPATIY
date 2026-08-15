#include "AdsEffect.h"
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <unknwn.h>
#include <gdiplus.h>
#include <shellapi.h>

struct AdWindowData {
    std::wstring imgPath;
    Gdiplus::Bitmap* pBitmap;
    int winW;
    int winH;
    std::chrono::steady_clock::time_point startTime;

    bool hasFakeBtn;
    bool showFakeBtn;
    int fakeCorner;
    RECT fakeBtnRect;

    bool showRealBtn;
    int realCorner;
    RECT realBtnRect;

    HINSTANCE hInstance;
    std::mt19937* pGen;
};

// ----------------------------------------------------------------------------
// 1. Полноэкранный просмотр fakeclosephoto (3 секунды)
// ----------------------------------------------------------------------------

static std::wstring FindFakeClosePhotoPath() {
    std::vector<std::wstring> candidates = {
        L"C:\\Users\\dispair\\source\\repos\\Project1\\fakeclosephoto.png",
        L"C:\\Users\\dispair\\source\\repos\\Project1\\fakeclosephoto.jpg",
        L"C:\\Users\\dispair\\source\\repos\\Project1\\fakeclosephoto.jpeg",
        L"C:\\Users\\dispair\\source\\repos\\Project1\\fakeclosephoto.bmp"
    };

    // 1. Текущая рабочая директория
    wchar_t curDir[MAX_PATH];
    if (GetCurrentDirectoryW(MAX_PATH, curDir) > 0) {
        std::wstring base = curDir;
        candidates.push_back(base + L"\\fakeclosephoto.png");
        candidates.push_back(base + L"\\fakeclosephoto.jpg");
        candidates.push_back(base + L"\\fakeclosephoto.jpeg");
        candidates.push_back(base + L"\\fakeclosephoto.bmp");
        candidates.push_back(base + L"\\fakeclosephoto.webp");
    }

    // 2. Директория исполняемого файла .exe
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) > 0) {
        std::wstring exeDir = exePath;
        size_t pos = exeDir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            std::wstring base = exeDir.substr(0, pos);
            candidates.push_back(base + L"\\fakeclosephoto.png");
            candidates.push_back(base + L"\\fakeclosephoto.jpg");
            candidates.push_back(base + L"\\fakeclosephoto.jpeg");
            candidates.push_back(base + L"\\fakeclosephoto.bmp");
            candidates.push_back(base + L"\\fakeclosephoto.webp");
        }
    }

    // 3. Папки рабочего стола
    for (const auto& folder : GetAllDesktopFolderPaths()) {
        candidates.push_back(folder + L"\\fakeclosephoto.png");
        candidates.push_back(folder + L"\\fakeclosephoto.jpg");
        candidates.push_back(folder + L"\\fakeclosephoto.jpeg");
        candidates.push_back(folder + L"\\fakeclosephoto.bmp");
        candidates.push_back(folder + L"\\fakeclosephoto.webp");
    }

    for (const auto& path : candidates) {
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return path;
        }
    }
    return L"";
}

static LRESULT CALLBACK FakePhotoProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static Gdiplus::Bitmap* pBmp = NULL;

    switch (uMsg) {
    case WM_CREATE: {
        std::wstring photoPath = FindFakeClosePhotoPath();
        if (!photoPath.empty()) {
            pBmp = Gdiplus::Bitmap::FromFile(photoPath.c_str());
        }

        if (!pBmp || pBmp->GetLastStatus() != Gdiplus::Ok) {
            // Резервная загрузка из ресурсов .exe (вшитое фото)
            HBITMAP hBmpRes = LoadImageFromResource(GetModuleHandle(NULL), IDR_FAKECLOSEPHOTO_PNG);
            if (hBmpRes) {
                pBmp = Gdiplus::Bitmap::FromHBITMAP(hBmpRes, NULL);
                DeleteObject(hBmpRes);
            }
        }
        SetTimer(hwnd, 1, 3000, NULL);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        int screenW = GetSystemMetrics(SM_CXSCREEN);
        int screenH = GetSystemMetrics(SM_CYSCREEN);

        if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) {
            Gdiplus::Graphics g(hdc);
            g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g.DrawImage(pBmp, 0, 0, screenW, screenH);
        } else {
            HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
            RECT fullRect = { 0, 0, screenW, screenH };
            FillRect(hdc, &fullRect, blackBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 0, 0));
            HFONT font = CreateFontW(72, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Impact");
            HFONT oldF = (HFONT)SelectObject(hdc, font);
            DrawTextW(hdc, L"ФЕЙКОВЫЙ КРЕСТИК!", -1, &fullRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, oldF);
            DeleteObject(font);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_TIMER:
    case WM_KEYDOWN:
        KillTimer(hwnd, 1);
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        if (pBmp) { delete pBmp; pBmp = NULL; }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static void ShowFakeClosePhotoOverlay(HINSTANCE hInstance) {
    const wchar_t CLASS_NAME[] = L"FakeClosePhotoOverlayClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = FakePhotoProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        CLASS_NAME, L"FakeClosePhoto", WS_POPUP | WS_VISIBLE,
        0, 0, screenW, screenH,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd) {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    UnregisterClassW(CLASS_NAME, hInstance);
}

// ----------------------------------------------------------------------------
// 2. Диалоговое окно подтверждения "Вы точно хотите закрыть?" (со скроллом)
// ----------------------------------------------------------------------------

struct ConfirmDialogData {
    std::chrono::steady_clock::time_point startTime;
    bool showYesBtn;
    bool resultConfirmed;
    int scrollY;
    int maxScrollY;
    RECT noBtnRect;
    RECT yesBtnRect;
};

static void UpdateConfirmScrollbar(HWND hwnd, ConfirmDialogData* data) {
    SCROLLINFO si = { sizeof(SCROLLINFO) };
    si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
    si.nMin = 0;
    si.nMax = 520; // Полная высота контента
    RECT rc;
    GetClientRect(hwnd, &rc);
    si.nPage = rc.bottom;
    si.nPos = data->scrollY;
    SetScrollInfo(hwnd, SB_VERT, &si, TRUE);
    data->maxScrollY = (std::max)(0, si.nMax - (int)si.nPage);
}

static LRESULT CALLBACK ConfirmDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    ConfirmDialogData* data = (ConfirmDialogData*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        data = (ConfirmDialogData*)pcs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)data);
        data->scrollY = 0;
        data->maxScrollY = 300;
        UpdateConfirmScrollbar(hwnd, data);
        SetTimer(hwnd, 1, 50, NULL);
        return 0;
    }
    case WM_TIMER: {
        if (!data) return 0;
        auto now = std::chrono::steady_clock::now();
        float elapsedMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - data->startTime).count();
        if (elapsedMs >= 2000.0f && !data->showYesBtn) {
            data->showYesBtn = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_VSCROLL: {
        if (!data) return 0;
        int oldY = data->scrollY;
        switch (LOWORD(wParam)) {
        case SB_LINEUP: data->scrollY -= 20; break;
        case SB_LINEDOWN: data->scrollY += 20; break;
        case SB_PAGEUP: data->scrollY -= 100; break;
        case SB_PAGEDOWN: data->scrollY += 100; break;
        case SB_THUMBTRACK: data->scrollY = HIWORD(wParam); break;
        }
        data->scrollY = (std::max)(0, (std::min)(data->scrollY, data->maxScrollY));
        if (data->scrollY != oldY) {
            SetScrollPos(hwnd, SB_VERT, data->scrollY, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_MOUSEWHEEL: {
        if (!data) return 0;
        short delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int oldY = data->scrollY;
        data->scrollY -= (delta / WHEEL_DELTA) * 30;
        data->scrollY = (std::max)(0, (std::min)(data->scrollY, data->maxScrollY));
        if (data->scrollY != oldY) {
            SetScrollPos(hwnd, SB_VERT, data->scrollY, TRUE);
            InvalidateRect(hwnd, NULL, FALSE);
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        // Фон диалога
        HBRUSH bgBrush = CreateSolidBrush(RGB(30, 34, 45));
        FillRect(hdc, &clientRect, bgBrush);
        DeleteObject(bgBrush);

        int sy = data ? data->scrollY : 0;

        // Длинный текст / соглашение (смещается по scrollY)
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(220, 225, 240));
        HFONT titleFont = CreateFontW(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        HFONT oldF = (HFONT)SelectObject(hdc, titleFont);
        RECT titleRect = { 15, 15 - sy, clientRect.right - 15, 55 - sy };
        DrawTextW(hdc, L"ВЫ ТОЧНО ХОТИТЕ ЗАКРЫТЬ?", -1, &titleRect, DT_CENTER | DT_SINGLELINE);
        SelectObject(hdc, oldF);
        DeleteObject(titleFont);

        HFONT subFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        oldF = (HFONT)SelectObject(hdc, subFont);
        RECT subRect = { 15, 55 - sy, clientRect.right - 15, 150 - sy };
        DrawTextW(hdc, L"Подтверждая закрытие этого окна, вы соглашаетесь с условиями пользовательского соглашения. Прокрутите вниз для выбора ответа.", -1, &subRect, DT_WORDBREAK | DT_CENTER);
        SelectObject(hdc, oldF);
        DeleteObject(subFont);

        // ОГРОМНАЯ КНОПКА "НЕТ" (расположена ниже по списку Y = 160)
        data->noBtnRect = { 20, 160 - sy, clientRect.right - 20, 250 - sy };
        HBRUSH noBrush = CreateSolidBrush(RGB(220, 40, 40));
        FillRect(hdc, &data->noBtnRect, noBrush);
        DeleteObject(noBrush);

        SetTextColor(hdc, RGB(255, 255, 255));
        HFONT noFont = CreateFontW(32, 0, 0, 0, FW_HEAVY, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Impact");
        oldF = (HFONT)SelectObject(hdc, noFont);
        DrawTextW(hdc, L"НЕТ !!! (ОСТАТЬСЯ)", -1, &data->noBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, oldF);
        DeleteObject(noFont);

        // Дополнительный текст дисклеймера перед кнопкой ДА
        oldF = (HFONT)SelectObject(hdc, subFont);
        RECT discRect = { 15, 270 - sy, clientRect.right - 15, 410 - sy };
        DrawTextW(hdc, L"Внимание: Действие 'Да' закроет текущий баннер. Вы уверены, что хотите отказаться от уникального спецпредложения?\n\nЛистайте ниже...", -1, &discRect, DT_WORDBREAK | DT_CENTER);
        SelectObject(hdc, oldF);
        DeleteObject(subFont);

        // МАЛЕНЬКАЯ КНОПКА "ДА" (появляется только через 2 секунды в самом низу Y = 430)
        if (data && data->showYesBtn) {
            data->yesBtnRect = { clientRect.right - 110, 430 - sy, clientRect.right - 20, 465 - sy };
            HBRUSH yesBrush = CreateSolidBrush(RGB(50, 55, 70));
            FillRect(hdc, &data->yesBtnRect, yesBrush);
            DeleteObject(yesBrush);

            SetTextColor(hdc, RGB(160, 170, 190));
            HFONT yesFont = CreateFontW(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            oldF = (HFONT)SelectObject(hdc, yesFont);
            DrawTextW(hdc, L"да, закрыть", -1, &data->yesBtnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            SelectObject(hdc, oldF);
            DeleteObject(yesFont);
        } else {
            data->yesBtnRect = { 0, 0, 0, 0 };
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (!data) return 0;
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };

        if (PtInRect(&data->noBtnRect, pt)) {
            data->resultConfirmed = false;
            DestroyWindow(hwnd);
            return 0;
        }

        if (data->showYesBtn && PtInRect(&data->yesBtnRect, pt)) {
            data->resultConfirmed = true;
            DestroyWindow(hwnd);
            return 0;
        }
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static bool ShowConfirmCloseDialog(HWND hParent, HINSTANCE hInstance) {
    const wchar_t CLASS_NAME[] = L"ConfirmCloseDialogClass";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = ConfirmDialogProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RegisterClassW(&wc);

    int winW = 360;
    int winH = 220;

    RECT parentRect;
    GetWindowRect(hParent, &parentRect);
    int posX = parentRect.left + (parentRect.right - parentRect.left - winW) / 2;
    int posY = parentRect.top + (parentRect.bottom - parentRect.top - winH) / 2;

    ConfirmDialogData data = {};
    data.startTime = std::chrono::steady_clock::now();
    data.showYesBtn = false;
    data.resultConfirmed = false;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"Подтверждение", WS_POPUP | WS_VISIBLE | WS_VSCROLL,
        posX, posY, winW, winH,
        hParent, NULL, hInstance, &data
    );

    if (hwnd) {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    UnregisterClassW(CLASS_NAME, hInstance);
    return data.resultConfirmed;
}

// ----------------------------------------------------------------------------
// 3. Переход на случайный поиск в Рамблере при клике на баннер
// ----------------------------------------------------------------------------

static void OpenRandomRamblerSearch(std::mt19937* pGen) {
    std::vector<std::wstring> searchUrls = {
        L"https://nova.rambler.ru/search?query=%D0%BB%D0%B8%D0%B3%D0%B0+%D0%BB%D0%B5%D0%B3%D0%B5%D0%BD%D0%B4",                      // лига легенд
        L"https://nova.rambler.ru/search?query=%D0%BC%D0%B0%D0%BB%D1%8C%D1%87%D0%B8%D0%BA%D0%B8+%D1%86%D0%B5%D0%BB%D1%83%D1%8E%D1%82%D1%81%D1%8F", // мальчики целуются
        L"https://nova.rambler.ru/search?query=%D0%BF%D0%BE%D0%BB%D0%BE%D0%B2%D0%BE%D0%B9+%D0%BE%D1%80%D0%B3%D0%B0%D0%BD+4+%D1%81%D0%B0%D0%BD%D1%82%D0%B8%D0%BC%D0%B5%D1%82%D1%80%D0%B0" // половой орган 4 сантиметра
    };

    size_t idx = 0;
    if (pGen) {
        std::uniform_int_distribution<size_t> dist(0, searchUrls.size() - 1);
        idx = dist(*pGen);
    }

    ShellExecuteW(NULL, L"open", searchUrls[idx].c_str(), NULL, NULL, SW_SHOWNORMAL);
}

// ----------------------------------------------------------------------------
// 4. Основной рекламный баннер
// ----------------------------------------------------------------------------

static RECT CalculateCornerRect(int cornerIndex, int winW, int winH, int btnSize = 16, int margin = 4) {
    switch (cornerIndex) {
    case 0: // Верхний правый
        return { winW - btnSize - margin, margin, winW - margin, margin + btnSize };
    case 1: // Верхний левый
        return { margin, margin, margin + btnSize, margin + btnSize };
    case 2: // Нижний правый
        return { winW - btnSize - margin, winH - btnSize - margin, winW - margin, winH - margin };
    case 3: // Нижний левый
        return { margin, winH - btnSize - margin, margin + btnSize, winH - margin };
    default:
        return { winW - btnSize - margin, margin, winW - margin, margin + btnSize };
    }
}

static void DrawCloseBtnGraphic(HDC hdc, const RECT& rect) {
    HBRUSH redBrush = CreateSolidBrush(RGB(220, 40, 40));
    HPEN whitePen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));

    FillRect(hdc, &rect, redBrush);
    HPEN hOldP = (HPEN)SelectObject(hdc, whitePen);

    int p = 3;
    MoveToEx(hdc, rect.left + p, rect.top + p, NULL);
    LineTo(hdc, rect.right - p, rect.bottom - p);
    MoveToEx(hdc, rect.right - p, rect.top + p, NULL);
    LineTo(hdc, rect.left + p, rect.bottom - p);

    SelectObject(hdc, hOldP);
    DeleteObject(whitePen);
    DeleteObject(redBrush);
}

static LRESULT CALLBACK AdsWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    AdWindowData* data = (AdWindowData*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        data = (AdWindowData*)pcs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)data);
        SetTimer(hwnd, 1, 50, NULL);
        return 0;
    }

    case WM_TIMER: {
        if (!data) return 0;
        auto now = std::chrono::steady_clock::now();
        float elapsedMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - data->startTime).count();

        // Появление настоящего крестика через 3 секунды
        if (elapsedMs >= 3000.0f && !data->showRealBtn) {
            data->showRealBtn = true;
            InvalidateRect(hwnd, NULL, FALSE);
        }

        if (g_cancelEffects.load()) {
            DestroyWindow(hwnd);
        }
        return 0;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        if (data) {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            int winW = clientRect.right;
            int winH = clientRect.bottom;

            if (data->pBitmap && data->pBitmap->GetLastStatus() == Gdiplus::Ok) {
                Gdiplus::Graphics graphics(hdc);
                graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
                graphics.DrawImage(data->pBitmap, 0, 0, winW, winH);
            } else {
                HBRUSH bgBrush = CreateSolidBrush(RGB(255, 240, 200));
                FillRect(hdc, &clientRect, bgBrush);
                DeleteObject(bgBrush);

                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(200, 20, 20));
                DrawTextW(hdc, L"🔥 ГОРЯЧАЯ РЕКЛАМА! 🔥", -1, &clientRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }

            // Отрисовка ФЕЙКОВОГО крестика (появляется сразу при открытии, если выпал 30% шанс)
            if (data->showFakeBtn) {
                data->fakeBtnRect = CalculateCornerRect(data->fakeCorner, winW, winH);
                DrawCloseBtnGraphic(hdc, data->fakeBtnRect);
            }

            // Отрисовка НАСТОЯЩЕГО крестика (появляется через 3 секунды)
            if (data->showRealBtn) {
                data->realBtnRect = CalculateCornerRect(data->realCorner, winW, winH);
                DrawCloseBtnGraphic(hdc, data->realBtnRect);
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_LBUTTONDOWN: {
        if (!data) return 0;
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };

        // 1. Клик по ФЕЙКОВОМУ крестику -> просмотр fakeclosephoto на 3 сек
        if (data->showFakeBtn && PtInRect(&data->fakeBtnRect, pt)) {
            ShowFakeClosePhotoOverlay(data->hInstance);
            return 0;
        }

        // 2. Клик по НАСТОЯЩЕМУ крестику -> 30% шанс диалога подтверждения
        if (data->showRealBtn && PtInRect(&data->realBtnRect, pt)) {
            bool needConfirm = false;
            if (data->pGen) {
                std::uniform_int_distribution<int> chanceDist(0, 99);
                if (chanceDist(*(data->pGen)) < 30) {
                    needConfirm = true;
                }
            }

            if (needConfirm) {
                bool confirmed = ShowConfirmCloseDialog(hwnd, data->hInstance);
                if (confirmed) {
                    KillTimer(hwnd, 1);
                    DestroyWindow(hwnd);
                }
            } else {
                KillTimer(hwnd, 1);
                DestroyWindow(hwnd);
            }
            return 0;
        }

        // 3. Клик на сам баннер рекламы (мимо крестиков) -> открытие случайного поиска в Рамблере
        OpenRandomRamblerSearch(data->pGen);
        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

static std::vector<std::wstring> GetReklamaImages() {
    std::vector<std::wstring> images;
    auto desktopFolders = GetAllDesktopFolderPaths();

    for (const auto& folder : desktopFolders) {
        std::wstring reklamaDir = folder + L"\\reklama";
        std::wstring searchPattern = reklamaDir + L"\\*.*";
        WIN32_FIND_DATAW findData;
        HANDLE hFind = FindFirstFileW(searchPattern.c_str(), &findData);

        if (hFind != INVALID_HANDLE_VALUE) {
            do {
                if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    std::wstring fileName = findData.cFileName;
                    std::wstring ext = L"";
                    size_t dotPos = fileName.find_last_of(L".");
                    if (dotPos != std::wstring::npos) {
                        ext = fileName.substr(dotPos);
                        for (auto& c : ext) c = towlower(c);
                    }

                    if (ext == L".png" || ext == L".jpg" || ext == L".jpeg" || ext == L".bmp" || ext == L".webp") {
                        images.push_back(reklamaDir + L"\\" + fileName);
                    }
                }
            } while (FindNextFileW(hFind, &findData));
            FindClose(hFind);
        }
    }
    return images;
}

void ShowAdsPopup(HINSTANCE hInstance, std::mt19937& gen) {
    std::vector<std::wstring> images = GetReklamaImages();

    Gdiplus::Bitmap* pBitmap = NULL;
    int winW = 400;
    int winH = 300;
    std::wstring selectedPath = L"";

    if (!images.empty()) {
        std::uniform_int_distribution<size_t> dist(0, images.size() - 1);
        selectedPath = images[dist(gen)];
        pBitmap = Gdiplus::Bitmap::FromFile(selectedPath.c_str());

        if (pBitmap && pBitmap->GetLastStatus() == Gdiplus::Ok) {
            int origW = (int)pBitmap->GetWidth();
            int origH = (int)pBitmap->GetHeight();

            int maxW = 550;
            int maxH = 450;

            float scale = 1.0f;
            if (origW > maxW || origH > maxH) {
                scale = (std::min)((float)maxW / origW, (float)maxH / origH);
            }
            winW = (int)(origW * scale);
            winH = (int)(origH * scale);
            if (winW < 100) winW = 100;
            if (winH < 100) winH = 100;
        } else {
            if (pBitmap) { delete pBitmap; pBitmap = NULL; }
        }
    }

    const wchar_t CLASS_NAME[] = L"AdsPopupClass";
    static bool isRegistered = false;

    if (!isRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = AdsWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassW(&wc);
        isRegistered = true;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);

    std::uniform_int_distribution<int> distX(50, (std::max)(50, screenW - winW - 50));
    std::uniform_int_distribution<int> distY(50, (std::max)(50, screenH - winH - 50));

    int posX = distX(gen);
    int posY = distY(gen);

    std::uniform_int_distribution<int> chanceDist(0, 99);
    std::uniform_int_distribution<int> cornerDist(0, 3);

    AdWindowData data = {};
    data.imgPath = selectedPath;
    data.pBitmap = pBitmap;
    data.winW = winW;
    data.winH = winH;
    data.startTime = std::chrono::steady_clock::now();
    data.hInstance = hInstance;
    data.pGen = &gen;

    // 1. Выбор угла для НАСТОЯЩЕГО крестика (появится через 3 сек)
    data.realCorner = cornerDist(gen);
    data.showRealBtn = false;

    // 2. Фейковый крестик (30% шанс появления сразу при старте)
    data.hasFakeBtn = (chanceDist(gen) < 30);
    data.showFakeBtn = data.hasFakeBtn;
    if (data.hasFakeBtn) {
        data.fakeCorner = (data.realCorner + 1 + (gen() % 3)) % 4;
    } else {
        data.fakeCorner = -1;
    }

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"Реклама", WS_POPUP | WS_VISIBLE,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, &data
    );

    if (hwnd) {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            if (g_cancelEffects.load()) { DestroyWindow(hwnd); break; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    if (pBitmap) {
        delete pBitmap;
    }
}

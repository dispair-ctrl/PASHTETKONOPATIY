#include "Effects.h"
#include <magnification.h>
#include <thread>
#include <chrono>
#include <vector>
#include <algorithm>
#include <unknwn.h>
#include <gdiplus.h>

#pragma comment(lib, "magnification.lib")

void ShowAdsPopup(HINSTANCE hInstance, std::mt19937& gen);

// ----------------------------------------------------------------------------
// 1. ОТМЕНА ВСЕХ ЭФФЕКТОВ
// ----------------------------------------------------------------------------

void CancelAllEffects() {
    g_cancelEffects.store(true);

    // 1. Останавливаем все звуки
    PlaySoundW(NULL, NULL, 0);

    // 2. Восстанавливаем оригинальные системные курсоры Windows
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

    // 3. Восстанавливаем разрешение экрана (720p -> оригинальное)
    ChangeDisplaySettingsW(NULL, 0);

    // 4. Отменяем инверсию цвета и зум Magnification API
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

// ----------------------------------------------------------------------------
// 2. ДРОЖАНИЕ ОКНА (SHAKE)
// ----------------------------------------------------------------------------

void ShakeWindow(HWND hwnd, int durationMs) {
    if (!hwnd) hwnd = GetForegroundWindow();
    if (!hwnd) return;

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
}

// ----------------------------------------------------------------------------
// 3. ГИГАНТСКИЙ КУРСОР
// ----------------------------------------------------------------------------

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

void SetHugeCursorForDuration(HINSTANCE hInstance, int durationMs) {
    for (size_t i = 0; i < NUM_SYSTEM_CURSORS; ++i) {
        HCURSOR hGiant = CreateGiantCursor(256);
        SetSystemCursor(hGiant, SYSTEM_CURSOR_IDS[i]);
    }

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
                DispatchMessage(&msg);
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
}

// ----------------------------------------------------------------------------
// 4. СВОРАЧИВАНИЕ ОКНА (WIN+D)
// ----------------------------------------------------------------------------

void PIZDA(HINSTANCE hInstance) {
    keybd_event(VK_LWIN, 0, 0, 0);
    keybd_event('D', 0, 0, 0);
    keybd_event('D', 0, KEYEVENTF_KEYUP, 0);
    keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

static HCURSOR CreateBlankCursor() {
    int w = GetSystemMetrics(SM_CXCURSOR);
    int h = GetSystemMetrics(SM_CYCURSOR);
    BYTE maskAND[128];
    BYTE maskXOR[128];
    memset(maskAND, 0xFF, sizeof(maskAND));
    memset(maskXOR, 0x00, sizeof(maskXOR));
    return CreateCursor(GetModuleHandle(NULL), 0, 0, w, h, maskAND, maskXOR);
}

void LowQualityScreenEffect(int durationMs) {
    if (g_isEffectActive.exchange(true)) return;

    // Сохраняем копию стандартного курсора стрелки перед скрытием
    HCURSOR hSavedArrow = (HCURSOR)CopyImage(LoadCursor(NULL, IDC_ARROW), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);

    // Скрываем стандартные системные курсоры Windows
    for (size_t i = 0; i < NUM_SYSTEM_CURSORS; ++i) {
        HCURSOR hBlank = CreateBlankCursor();
        SetSystemCursor(hBlank, SYSTEM_CURSOR_IDS[i]);
    }

    HINSTANCE hInstance = GetModuleHandle(NULL);
    const wchar_t CLASS_NAME[] = L"PixelateOverlayClass";
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
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE,
        CLASS_NAME, L"PixelateOverlay", WS_POPUP | WS_VISIBLE,
        virtX, virtY, virtW, virtH,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd) {
        BOOL affinitySet = SetWindowDisplayAffinity(hwnd, WDA_EXCLUDEFROMCAPTURE);
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);

        HDC hdcWin = GetDC(hwnd);
        int lowW = (std::max)(160, virtW / 6);
        int lowH = (std::max)(120, virtH / 6);

        auto start = std::chrono::steady_clock::now();
        while (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() < durationMs) {

            if (g_cancelEffects.load()) break;

            MSG msg;
            while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            if (!affinitySet) {
                ShowWindow(hwnd, SW_HIDE);
            }

            HDC hdcScreen = GetDC(NULL);
            if (hdcScreen) {
                HDC hdcMemLow = CreateCompatibleDC(hdcScreen);
                HBITMAP hBmpLow = CreateCompatibleBitmap(hdcScreen, lowW, lowH);
                HBITMAP hOldLow = (HBITMAP)SelectObject(hdcMemLow, hBmpLow);

                SetStretchBltMode(hdcMemLow, COLORONCOLOR);
                StretchBlt(hdcMemLow, 0, 0, lowW, lowH, hdcScreen, virtX, virtY, virtW, virtH, SRCCOPY);

                // Отрисовка точного пикселизированного курсора
                POINT pt;
                GetCursorPos(&pt);

                ICONINFO ii = { 0 };
                if (hSavedArrow && GetIconInfo(hSavedArrow, &ii)) {
                    int xHot = ii.xHotspot * lowW / virtW;
                    int yHot = ii.yHotspot * lowH / virtH;

                    if (ii.hbmMask) DeleteObject(ii.hbmMask);
                    if (ii.hbmColor) DeleteObject(ii.hbmColor);

                    int lx = (pt.x - virtX) * lowW / virtW;
                    int ly = (pt.y - virtY) * lowH / virtH;

                    int iconW = 32 * lowW / virtW;
                    int iconH = 32 * lowH / virtH;
                    if (iconW < 16) iconW = 16;
                    if (iconH < 16) iconH = 16;

                    DrawIconEx(hdcMemLow, lx - xHot, ly - yHot, hSavedArrow, iconW, iconH, 0, NULL, DI_NORMAL);
                }

                if (!affinitySet) {
                    ShowWindow(hwnd, SW_SHOWNA);
                }

                SetStretchBltMode(hdcWin, COLORONCOLOR);
                StretchBlt(hdcWin, 0, 0, virtW, virtH, hdcMemLow, 0, 0, lowW, lowH, SRCCOPY);

                SelectObject(hdcMemLow, hOldLow);
                DeleteObject(hBmpLow);
                DeleteDC(hdcMemLow);
                ReleaseDC(NULL, hdcScreen);
            } else if (!affinitySet) {
                ShowWindow(hwnd, SW_SHOWNA);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(33));
        }

        ReleaseDC(hwnd, hdcWin);
        DestroyWindow(hwnd);
    }

    if (hSavedArrow) DestroyCursor(hSavedArrow);

    // Восстанавливаем оригинальные системные курсоры Windows
    SystemParametersInfo(SPI_SETCURSORS, 0, NULL, 0);

    g_isEffectActive.store(false);
}

// ----------------------------------------------------------------------------
// 6. МЫШКА-ЭПИЛЕПТИК
// ----------------------------------------------------------------------------

void EpilepticMouseEffect(int durationMs) {
    if (g_isEffectActive.exchange(true)) return;

    POINT startPt;
    GetCursorPos(&startPt);

    auto start = std::chrono::steady_clock::now();
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start).count() < durationMs) {
        if (g_cancelEffects.load()) break;

        POINT curPt;
        GetCursorPos(&curPt);

        int offsetX = (rand() % 161) - 80;
        int offsetY = (rand() % 161) - 80;

        SetCursorPos(curPt.x + offsetX, curPt.y + offsetY);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    g_isEffectActive.store(false);
}

// ----------------------------------------------------------------------------
// 7. КАПЧА ПРИ ВВОДЕ С КЛАВИАТУРЫ
// ----------------------------------------------------------------------------

struct CaptchaGridCell {
    std::wstring category;
    Gdiplus::Bitmap* pBitmap;
    RECT cellRect;
};

struct CaptchaWindowData {
    std::wstring targetCategoryName;
    std::wstring targetCategoryTitle;
    CaptchaGridCell cells[4];
    int correctCellIndex;
    HINSTANCE hInstance;
    std::mt19937* pGen;
    bool answerClicked;
};

static Gdiplus::Bitmap* LoadKapchaImage(HINSTANCE hInstance, const std::wstring& filename, WORD resId) {
    std::vector<std::wstring> candidates;
    wchar_t curDir[MAX_PATH];
    if (GetCurrentDirectoryW(MAX_PATH, curDir) > 0) {
        candidates.push_back(std::wstring(curDir) + L"\\kapcha\\" + filename);
    }
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) > 0) {
        std::wstring exeDir = exePath;
        size_t pos = exeDir.find_last_of(L"\\/");
        if (pos != std::wstring::npos) {
            candidates.push_back(exeDir.substr(0, pos) + L"\\kapcha\\" + filename);
        }
    }
    for (const auto& folder : GetAllDesktopFolderPaths()) {
        candidates.push_back(folder + L"\\kapcha\\" + filename);
    }

    for (const auto& path : candidates) {
        if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES) {
            Gdiplus::Bitmap* pBmp = Gdiplus::Bitmap::FromFile(path.c_str());
            if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) return pBmp;
            if (pBmp) delete pBmp;
        }
    }

    // Резервная загрузка из ресурсов .exe
    if (resId != 0) {
        HBITMAP hBmpRes = LoadImageFromResource(hInstance, resId);
        if (hBmpRes) {
            Gdiplus::Bitmap* pBmp = Gdiplus::Bitmap::FromHBITMAP(hBmpRes, NULL);
            DeleteObject(hBmpRes);
            if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) return pBmp;
            if (pBmp) delete pBmp;
        }
    }
    return NULL;
}

static LRESULT CALLBACK CaptchaWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    CaptchaWindowData* data = (CaptchaWindowData*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CREATE: {
        CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
        data = (CaptchaWindowData*)pcs->lpCreateParams;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)data);
        SetTimer(hwnd, 1, 50, NULL);
        return 0;
    }
    case WM_TIMER: {
        if (g_cancelEffects.load()) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        HBRUSH bgBrush = CreateSolidBrush(RGB(24, 28, 36));
        FillRect(hdc, &clientRect, bgBrush);
        DeleteObject(bgBrush);

        if (data) {
            // Заголовок Капчи
            RECT headerRect = { 10, 10, clientRect.right - 10, 55 };
            HBRUSH headerBrush = CreateSolidBrush(RGB(66, 133, 244));
            FillRect(hdc, &headerRect, headerBrush);
            DeleteObject(headerBrush);

            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 255));
            HFONT hFont1 = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            HFONT oldF = (HFONT)SelectObject(hdc, hFont1);
            RECT t1Rect = { 15, 12, clientRect.right - 15, 30 };
            DrawTextW(hdc, L"Выберите все изображения, где есть:", -1, &t1Rect, DT_LEFT | DT_SINGLELINE);
            SelectObject(hdc, oldF);
            DeleteObject(hFont1);

            HFONT hFont2 = CreateFontW(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            oldF = (HFONT)SelectObject(hdc, hFont2);
            RECT t2Rect = { 15, 30, clientRect.right - 15, 52 };
            DrawTextW(hdc, data->targetCategoryTitle.c_str(), -1, &t2Rect, DT_LEFT | DT_SINGLELINE);
            SelectObject(hdc, oldF);
            DeleteObject(hFont2);

            // Отрисовка сетки 2x2
            Gdiplus::Graphics graphics(hdc);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);

            for (int i = 0; i < 4; ++i) {
                RECT& r = data->cells[i].cellRect;
                if (data->cells[i].pBitmap && data->cells[i].pBitmap->GetLastStatus() == Gdiplus::Ok) {
                    graphics.DrawImage(data->cells[i].pBitmap, (INT)r.left, (INT)r.top, (INT)(r.right - r.left), (INT)(r.bottom - r.top));
                } else {
                    HBRUSH cellBg = CreateSolidBrush(RGB(50, 55, 70));
                    FillRect(hdc, &r, cellBg);
                    DeleteObject(cellBg);
                }

                // Рамка вокруг ячейки
                HPEN pPen = CreatePen(PS_SOLID, 2, RGB(200, 200, 200));
                HPEN oP = (HPEN)SelectObject(hdc, pPen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, r.left, r.top, r.right, r.bottom);
                SelectObject(hdc, oP);
                DeleteObject(pPen);
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (!data || data->answerClicked) return 0;
        POINT pt = { LOWORD(lParam), HIWORD(lParam) };

        for (int i = 0; i < 4; ++i) {
            if (PtInRect(&data->cells[i].cellRect, pt)) {
                data->answerClicked = true;

                bool isCorrect = (i == data->correctCellIndex);
                bool triggerPunishment = false;

                if (!isCorrect) {
                    // ПРОЁБ: 100% запуск 3 эффектов!
                    triggerPunishment = true;
                } else {
                    // ПРАВИЛЬНО: 20% шанс запуска 3 эффектов
                    if (data->pGen) {
                        std::uniform_int_distribution<int> dist20(0, 99);
                        if (dist20(*(data->pGen)) < 20) {
                            triggerPunishment = true;
                        }
                    }
                }

                HINSTANCE hInst = data->hInstance;
                std::mt19937 genCopy = data->pGen ? *(data->pGen) : std::mt19937();

                DestroyWindow(hwnd);

                if (triggerPunishment) {
                    std::thread([hInst, genCopy]() mutable {
                        TriggerThreeCategoryPunishment(hInst, genCopy);
                    }).detach();
                }
                return 0;
            }
        }
        return 0;
    }
    case WM_DESTROY:
        for (int i = 0; i < 4; ++i) {
            if (data && data->cells[i].pBitmap) {
                delete data->cells[i].pBitmap;
                data->cells[i].pBitmap = NULL;
            }
        }
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void ShowKeyboardCaptcha(HINSTANCE hInstance, std::mt19937& gen) {
    if (g_isEffectActive.exchange(true)) return;

    struct CategoryDef {
        std::wstring name;
        std::wstring title;
        std::vector<std::wstring> files;
        std::vector<WORD> resIds;
    };

    std::vector<CategoryDef> categories = {
        { L"автобус", L"АВТОБУС", { L"автобус1.png", L"автобус2.png", L"автобус3.png", L"автобус4.png" }, { 5000, 5001, 5002, 5003 } },
        { L"гидрант", L"ПОЖАРНЫЙ ГИДРАНТ", { L"гидрант1.png", L"гидрант2.png", L"гидрант3.png", L"гидрант4.png" }, { 5004, 5005, 5006, 5007 } },
        { L"девушка", L"ДЕВУШКА", { L"девушка1.png", L"девушка2.png", L"девушка3.png", L"девушка4.png" }, { 5008, 5009, 5010, 5011 } },
        { L"пешеходныйпереход", L"ПЕШЕХОДНЫЙ ПЕРЕХОД", { L"пешеходныйпереход1.png", L"пешеходныйпереход2.png", L"пешеходныйпереход3.png", L"пешеходныйпереход4.png" }, { 5012, 5013, 5014, 5015 } }
    };

    std::uniform_int_distribution<size_t> catDist(0, categories.size() - 1);
    size_t targetCatIdx = catDist(gen);
    const auto& targetCat = categories[targetCatIdx];

    std::uniform_int_distribution<size_t> fileDist(0, 3);
    size_t targetFileIdx = fileDist(gen);

    CaptchaWindowData data = {};
    data.targetCategoryName = targetCat.name;
    data.targetCategoryTitle = targetCat.title;
    data.hInstance = hInstance;
    data.pGen = &gen;
    data.answerClicked = false;

    // Выбираем правильную ячейку (0..3)
    std::uniform_int_distribution<int> cellDist(0, 3);
    data.correctCellIndex = cellDist(gen);

    // Подготавливаем оставшиеся категории для остальных 3 ячеек
    std::vector<size_t> otherCatIndices;
    for (size_t i = 0; i < categories.size(); ++i) {
        if (i != targetCatIdx) otherCatIndices.push_back(i);
    }
    std::shuffle(otherCatIndices.begin(), otherCatIndices.end(), gen);

    int cellPositions[4][4] = {
        { 15, 65, 210, 260 },
        { 220, 65, 415, 260 },
        { 15, 270, 210, 465 },
        { 220, 270, 415, 465 }
    };

    size_t otherCounter = 0;
    for (int i = 0; i < 4; ++i) {
        data.cells[i].cellRect = { cellPositions[i][0], cellPositions[i][1], cellPositions[i][2], cellPositions[i][3] };
        if (i == data.correctCellIndex) {
            data.cells[i].category = targetCat.name;
            data.cells[i].pBitmap = LoadKapchaImage(hInstance, targetCat.files[targetFileIdx], targetCat.resIds[targetFileIdx]);
        } else {
            size_t oCatIdx = otherCatIndices[otherCounter % otherCatIndices.size()];
            otherCounter++;
            const auto& oCat = categories[oCatIdx];
            size_t oFileIdx = fileDist(gen);
            data.cells[i].category = oCat.name;
            data.cells[i].pBitmap = LoadKapchaImage(hInstance, oCat.files[oFileIdx], oCat.resIds[oFileIdx]);
        }
    }

    const wchar_t CLASS_NAME[] = L"KeyboardCaptchaClass";
    static bool isRegistered = false;
    if (!isRegistered) {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = CaptchaWindowProc;
        wc.hInstance = hInstance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassW(&wc);
        isRegistered = true;
    }

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int winW = 435;
    int winH = 480;
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"Подтвердите, что вы не робот", WS_POPUP | WS_VISIBLE,
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

    g_isEffectActive.store(false);
}

// ----------------------------------------------------------------------------
// 8. ВЫЗОВ СЛУЧАЙНОГО ДРУГОГО/СИСТЕМНОГО ЭФФЕКТА
// ----------------------------------------------------------------------------

void TriggerRandomOtherEffect(HINSTANCE hInstance, std::mt19937& gen) {
    std::uniform_int_distribution<int> dist(0, 5);
    int choice = dist(gen);
    switch (choice) {
    case 0:
        ShakeWindow(GetForegroundWindow(), 2000);
        break;
    case 1: {
        std::uniform_int_distribution<int> distCursorDur(5000, 10000);
        SetHugeCursorForDuration(hInstance, distCursorDur(gen));
        break;
    }
    case 2:
        PIZDA(hInstance);
        break;
    case 3:
        ShowAdsPopup(hInstance, gen);
        break;
    case 4:
        LowQualityScreenEffect(10000);
        break;
    case 5:
        EpilepticMouseEffect(2500);
        break;
    }
}

// ----------------------------------------------------------------------------
// 9. ТРОЙНОЕ НАКАЗАНИЕ: 3 ЭФФЕКТА СРАЗУ ИЗ ВСЕХ 3-Х РАЗДЕЛОВ (ОДНОВРЕМЕННО)
// ----------------------------------------------------------------------------

void TriggerThreeCategoryPunishment(HINSTANCE hInstance, std::mt19937& gen) {
    std::thread tSound([hInstance]() {
        std::random_device rd; std::mt19937 g(rd());
        TriggerRandomSoundEffect(hInstance, g);
    });

    std::thread tScreen([hInstance]() {
        std::random_device rd; std::mt19937 g(rd());
        TriggerRandomScreenEffect(hInstance, g);
    });

    std::thread tOther([hInstance]() {
        std::random_device rd; std::mt19937 g(rd());
        TriggerRandomOtherEffect(hInstance, g);
    });

    tSound.detach();
    tScreen.detach();
    tOther.detach();
}

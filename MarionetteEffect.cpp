#include "MarionetteEffect.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <unknwn.h>
#include <gdiplus.h>

static Gdiplus::Bitmap* LoadGdiplusBitmap(bool pressed) {
    auto desktopFolders = GetAllDesktopFolderPaths();
    std::vector<std::wstring> fileNames;

    if (pressed) {
        fileNames = {
            L"knopka_najata2.psd.png",
            L"knopka_najata2.png",
            L"knopka_najata.psd.png",
            L"knopka_najata.png"
        };
    } else {
        fileNames = {
            L"knopka2.psd.png",
            L"knopka2.png",
            L"knopka.psd.png",
            L"knopka.png"
        };
    }

    for (const auto& folder : desktopFolders) {
        for (const auto& fileName : fileNames) {
            std::wstring fullPath = folder + L"\\" + fileName;
            if (GetFileAttributesW(fullPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                Gdiplus::Bitmap* pBmp = Gdiplus::Bitmap::FromFile(fullPath.c_str());
                if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) {
                    return pBmp;
                }
                if (pBmp) delete pBmp;
            }
        }
    }

    // Резервная загрузка из ресурсов EXE
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WORD resId = pressed ? IDR_KNOPKA_NAJATA2_PNG : IDR_KNOPKA2_PNG;
    HRSRC hRes = FindResourceW(hInstance, MAKEINTRESOURCEW(resId), RT_RCDATA);
    if (hRes) {
        DWORD size = SizeofResource(hInstance, hRes);
        HGLOBAL hGlob = LoadResource(hInstance, hRes);
        if (hGlob) {
            void* pData = LockResource(hGlob);
            if (pData) {
                HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, size);
                if (hBuffer) {
                    void* pBuffer = GlobalLock(hBuffer);
                    if (pBuffer) {
                        CopyMemory(pBuffer, pData, size);
                        GlobalUnlock(hBuffer);
                        IStream* pStream = NULL;
                        if (SUCCEEDED(CreateStreamOnHGlobal(hBuffer, TRUE, &pStream))) {
                            Gdiplus::Bitmap* pBmp = Gdiplus::Bitmap::FromStream(pStream);
                            pStream->Release();
                            if (pBmp && pBmp->GetLastStatus() == Gdiplus::Ok) {
                                return pBmp;
                            }
                            if (pBmp) delete pBmp;
                        }
                    }
                    GlobalFree(hBuffer);
                }
            }
        }
    }

    return NULL;
}

void ShowMarionetteEffect(HINSTANCE hInstance, std::mt19937& gen) {
    Gdiplus::Bitmap* pBmpNormal = LoadGdiplusBitmap(false);
    Gdiplus::Bitmap* pBmpPressed = LoadGdiplusBitmap(true);

    const wchar_t CLASS_NAME[] = L"FnafMarionetteClass";
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

    int btnWidth = 100;
    int btnHeight = 100;
    int barWidth = 180;
    int barHeight = 26;
    int padding = 15;

    int winWidth = barWidth + padding + btnWidth;
    int winHeight = (std::max)(btnHeight, barHeight + 40);

    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int winX = screenW - winWidth - 30;
    int winY = 30;

    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW,
        CLASS_NAME, L"MarionetteMusicBox", WS_POPUP | WS_VISIBLE,
        winX, winY, winWidth, winHeight,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        if (pBmpNormal) delete pBmpNormal;
        if (pBmpPressed) delete pBmpPressed;
        return;
    }

    COLORREF chromaKey = RGB(0, 255, 0);
    SetLayeredWindowAttributes(hwnd, chromaKey, 0, LWA_COLORKEY);

    HDC hdcWin = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWin);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcWin, winWidth, winHeight);
    HBITMAP hOldBmp = (HBITMAP)SelectObject(hdcMem, hBmp);

    HBRUSH greenBrush = CreateSolidBrush(chromaKey);
    HBRUSH barBgBrush = CreateSolidBrush(RGB(30, 30, 40));
    HBRUSH barFillBrush = CreateSolidBrush(RGB(50, 205, 50));
    HBRUSH barWarningBrush = CreateSolidBrush(RGB(220, 50, 50));
    HPEN barBorderPen = CreatePen(PS_SOLID, 2, RGB(200, 200, 200));

    HFONT hFont = CreateFontW(
        14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI"
    );

    float fillPercentage = 100.0f;
    float timeAt100PercentMs = 0.0f;
    auto startTime = std::chrono::steady_clock::now();
    auto lastTime = startTime;

    RECT btnRect = { barWidth + padding, (winHeight - btnHeight) / 2, barWidth + padding + btnWidth, (winHeight - btnHeight) / 2 + btnHeight };

    bool isFailed = false;

    while (g_isRunning.load()) {
        if (g_cancelEffects.load()) break;

        auto now = std::chrono::steady_clock::now();
        float totalElapsedMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
        if (totalElapsedMs >= 60000.0f) {
            break;
        }

        float deltaMs = (float)std::chrono::duration_cast<std::chrono::milliseconds>(now - lastTime).count();
        lastTime = now;

        MSG msg;
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        POINT ptMouse;
        GetCursorPos(&ptMouse);
        bool lbtnDown = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;

        bool isPressedOnButton = false;
        if (lbtnDown) {
            POINT ptClient = ptMouse;
            ScreenToClient(hwnd, &ptClient);
            if (PtInRect(&btnRect, ptClient)) {
                isPressedOnButton = true;
            }
        }

        if (isPressedOnButton) {
            fillPercentage += (100.0f / 1.5f) * (deltaMs / 1000.0f);
            if (fillPercentage >= 100.0f) {
                fillPercentage = 100.0f;
                timeAt100PercentMs += deltaMs;
            } else {
                timeAt100PercentMs = 0.0f;
            }
        } else {
            fillPercentage -= 10.0f * (deltaMs / 1000.0f);
            if (fillPercentage < 0.0f) fillPercentage = 0.0f;
            timeAt100PercentMs = 0.0f;
        }

        if (fillPercentage <= 0.0f || timeAt100PercentMs >= 3000.0f) {
            isFailed = true;
            break;
        }

        RECT winRect = { 0, 0, winWidth, winHeight };
        FillRect(hdcMem, &winRect, greenBrush);

        int barX = 10;
        int barY = (winHeight - barHeight) / 2;
        RECT barOuterRect = { barX, barY, barX + barWidth, barY + barHeight };

        HPEN hOldPen = (HPEN)SelectObject(hdcMem, barBorderPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdcMem, barBgBrush);
        Rectangle(hdcMem, barOuterRect.left, barOuterRect.top, barOuterRect.right, barOuterRect.bottom);

        int filledPx = (int)((barWidth - 4) * (fillPercentage / 100.0f));
        if (filledPx > 0) {
            RECT fillRect = { barX + 2, barY + 2, barX + 2 + filledPx, barY + barHeight - 2 };
            HBRUSH currentFillBrush = (fillPercentage < 25.0f || timeAt100PercentMs > 2000.0f) ? barWarningBrush : barFillBrush;
            FillRect(hdcMem, &fillRect, currentFillBrush);
        }

        SetBkMode(hdcMem, TRANSPARENT);
        SetTextColor(hdcMem, RGB(255, 255, 255));
        HFONT hOldFont = (HFONT)SelectObject(hdcMem, hFont);
        wchar_t percentStr[32];
        swprintf_s(percentStr, L"ШКАТУЛКА: %d%%", (int)fillPercentage);
        DrawTextW(hdcMem, percentStr, -1, &barOuterRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        Gdiplus::Bitmap* pCurrentBmp = isPressedOnButton ? (pBmpPressed ? pBmpPressed : pBmpNormal) : pBmpNormal;

        if (pCurrentBmp) {
            Gdiplus::Graphics graphics(hdcMem);
            graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            graphics.DrawImage(pCurrentBmp, btnRect.left, btnRect.top, btnWidth, btnHeight);
        } else {
            HBRUSH btnBrush = CreateSolidBrush(isPressedOnButton ? RGB(180, 40, 40) : RGB(40, 140, 220));
            FillRect(hdcMem, &btnRect, btnBrush);
            DeleteObject(btnBrush);

            DrawTextW(hdcMem, isPressedOnButton ? L"ЖМЕШЬ!" : L"ЗАЖМИ!", -1, &btnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        }

        SelectObject(hdcMem, hOldFont);
        SelectObject(hdcMem, hOldPen);
        SelectObject(hdcMem, hOldBrush);

        BitBlt(hdcWin, 0, 0, winWidth, winHeight, hdcMem, 0, 0, SRCCOPY);
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    DeleteObject(hFont);
    DeleteObject(barBorderPen);
    DeleteObject(barWarningBrush);
    DeleteObject(barFillBrush);
    DeleteObject(barBgBrush);
    DeleteObject(greenBrush);

    SelectObject(hdcMem, hOldBmp);
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWin);
    DestroyWindow(hwnd);

    if (pBmpNormal) delete pBmpNormal;
    if (pBmpPressed) delete pBmpPressed;

    if (isFailed) {
        std::thread([hInstance]() {
            std::random_device rd; std::mt19937 g(rd());
            TriggerThreeCategoryPunishment(hInstance, g);
        }).detach();
    }
}

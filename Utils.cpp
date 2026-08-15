#include "Utils.h"
#include <shlobj.h>
#include <iostream>

static ULONG_PTR g_gdiplusToken = 0;

void InitGDIPlus() {
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, NULL);
}

void ShutdownGDIPlus() {
    if (g_gdiplusToken) {
        Gdiplus::GdiplusShutdown(g_gdiplusToken);
        g_gdiplusToken = 0;
    }
}

std::vector<std::wstring> GetAllDesktopFolderPaths() {
    std::vector<std::wstring> folders;

    // 1. Прямой путь через %USERPROFILE%\Desktop
    wchar_t userProfile[MAX_PATH];
    if (GetEnvironmentVariableW(L"USERPROFILE", userProfile, MAX_PATH) > 0) {
        std::wstring desktop = std::wstring(userProfile) + L"\\Desktop";
        if (GetFileAttributesW(desktop.c_str()) != INVALID_FILE_ATTRIBUTES) {
            folders.push_back(desktop);
        }
    }

    // 2. Путь через SHGetFolderPathW (CSIDL_DESKTOPDIRECTORY)
    wchar_t csidlPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOPDIRECTORY, NULL, 0, csidlPath))) {
        std::wstring desktop = csidlPath;
        if (std::find(folders.begin(), folders.end(), desktop) == folders.end()) {
            folders.push_back(desktop);
        }
    }

    // 3. Путь к общему рабочему столу
    wchar_t commonPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_COMMON_DESKTOPDIRECTORY, NULL, 0, commonPath))) {
        std::wstring desktop = commonPath;
        if (std::find(folders.begin(), folders.end(), desktop) == folders.end()) {
            folders.push_back(desktop);
        }
    }

    if (folders.empty()) {
        folders.push_back(L"C:\\Users\\Public\\Desktop");
    }

    return folders;
}

std::wstring GetDesktopFolderPath() {
    auto folders = GetAllDesktopFolderPaths();
    return folders.front();
}

HBITMAP LoadImageToHBITMAP(const std::wstring& filePath) {
    if (GetFileAttributesW(filePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        return NULL;
    }

    Gdiplus::Bitmap bitmap(filePath.c_str());
    if (bitmap.GetLastStatus() != Gdiplus::Ok) {
        return NULL;
    }

    HBITMAP hBmp = NULL;
    Gdiplus::Color bgColor(0, 0, 0, 0);
    bitmap.GetHBITMAP(bgColor, &hBmp);
    return hBmp;
}

HBITMAP LoadImageFromResource(HINSTANCE hInstance, WORD resId) {
    HRSRC hRes = FindResourceW(hInstance, MAKEINTRESOURCEW(resId), RT_RCDATA);
    if (!hRes) return NULL;
    DWORD size = SizeofResource(hInstance, hRes);
    HGLOBAL hGlob = LoadResource(hInstance, hRes);
    if (!hGlob) return NULL;
    void* pData = LockResource(hGlob);
    if (!pData) return NULL;

    HGLOBAL hBuffer = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hBuffer) return NULL;
    void* pBuffer = GlobalLock(hBuffer);
    if (pBuffer) {
        CopyMemory(pBuffer, pData, size);
        GlobalUnlock(hBuffer);

        IStream* pStream = NULL;
        if (SUCCEEDED(CreateStreamOnHGlobal(hBuffer, TRUE, &pStream))) {
            Gdiplus::Bitmap bitmap(pStream);
            pStream->Release();

            if (bitmap.GetLastStatus() == Gdiplus::Ok) {
                HBITMAP hBmp = NULL;
                Gdiplus::Color bgColor(0, 0, 0, 0);
                bitmap.GetHBITMAP(bgColor, &hBmp);
                return hBmp;
            }
        }
    }
    GlobalFree(hBuffer);
    return NULL;
}

HBITMAP LoadScreamerImage(HINSTANCE hInstance, WORD resId) {
    HBITMAP hBmp = LoadImageFromResource(hInstance, resId);
    if (hBmp) return hBmp;

    return (HBITMAP)LoadImageW(hInstance, MAKEINTRESOURCEW(resId), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);
}


LRESULT CALLBACK OverlayProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_NCHITTEST:
        return HTTRANSPARENT;
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

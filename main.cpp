#include "Globals.h"
#include "Utils.h"
#include "GUI.h"
#include "EventLoop.h"
#include <thread>

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);

    // Инициализация GDI+
    InitGDIPlus();

    // Инициализация ресурсов (динамический поиск зашитых картинок и звуков)
    InitDynamicResources(hInstance);


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

    // Скрываем консоль (и в Debug, и в Release)
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
    int winHeight = 615;
#else
    int winHeight = 405;
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

    // Завершение работы GDI+
    ShutdownGDIPlus();

    return 0;
}

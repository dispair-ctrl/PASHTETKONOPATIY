#include "EventLoop.h"
#include <thread>
#include <chrono>

static HHOOK g_hKeyHook = NULL;
static HINSTANCE g_hHookInstance = NULL;

static LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        if (g_isEnabled.load() && !g_isEffectActive.load()) {
            int modeIdx = (int)g_currentMode.load();
            if (modeIdx >= 0 && modeIdx <= 4) {
                const ModeChances& current = G_MODE_CHANCES[modeIdx];
                static std::random_device rd;
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<int> distCaptcha(1, current.captcha);

                if (distCaptcha(gen) == 1) {
                    std::thread([hInst = g_hHookInstance]() {
                        std::random_device rd; std::mt19937 g(rd());
                        ShowKeyboardCaptcha(hInst, g);
                    }).detach();
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyHook, nCode, wParam, lParam);
}

void EventLoopWorker(HINSTANCE hInstance) {
    g_hHookInstance = hInstance;
    g_hKeyHook = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);

    std::random_device rd;
    std::mt19937 gen(rd());

    int msCounter = 0;

    while (g_isRunning.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        msCounter += 50;

        // Вызов сообщений Windows для работы клавиатурного хука
        MSG hookMsg;
        while (PeekMessageW(&hookMsg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&hookMsg);
            DispatchMessage(&hookMsg);
        }

#ifdef _DEBUG
        // Горячие клавиши отладки (Shift + F1..F12)
        bool shiftPressed = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        if (shiftPressed) {
            if (GetAsyncKeyState(VK_F1) & 0x8000) {
                std::thread([hInstance]() { ShakeWindow(GetForegroundWindow(), 2000); }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F2) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distInvertDur(7000, 20000);
                    InvertScreenOverlay(hInstance, distInvertDur(g));
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F3) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distCursorDur(5000, 10000);
                    SetHugeCursorForDuration(hInstance, distCursorDur(g));
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F4) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distGlitchDur(7000, 20000);
                    GlitchScreenOverlay(hInstance, L"VAS VZLAMIVAET JOPA", distGlitchDur(g));
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F5) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    Flashbang(hInstance, 2000, g);
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F6) & 0x8000) {
                std::thread([hInstance]() { PIZDA(hInstance); }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F8) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    ShowScreamerWindow(hInstance, g);
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F9) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    PlayLongSound(hInstance, g);
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F10) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distZoomDur(5000, 15000);
                    ZoomScreen(2.0f, distZoomDur(g));
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F11) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distStripeDur(7000, 15000);
                    BlackStripeOverlay(hInstance, distStripeDur(g));
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
            if (GetAsyncKeyState(VK_F12) & 0x8000) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    ShowSkillCheck(hInstance, g);
                }).detach();
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
                msCounter = 0; continue;
            }
        }
#endif

        // Проверка возникновения случайных событий раз в 1 секунду
        if (msCounter >= 1000) {
            msCounter = 0;

            if (!g_isEnabled.load()) {
                continue;
            }

            int modeIdx = (int)g_currentMode.load();
            if (modeIdx < 0 || modeIdx > 4) modeIdx = 1;

            const ModeChances& current = G_MODE_CHANCES[modeIdx];

            // 1. Проверка на флешку
            std::uniform_int_distribution<int> distFlash(1, current.flashbang);
            if (distFlash(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    Flashbang(hInstance, 2000, g);
                }).detach();
            }

            // 2. Проверка на скример
            std::uniform_int_distribution<int> distScream(1, current.screamer);
            if (distScream(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    ShowScreamerWindow(hInstance, g);
                }).detach();
            }

            // 3. Проверка на длинный звук
            std::uniform_int_distribution<int> distLong(1, current.longSound);
            if (distLong(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    PlayLongSound(hInstance, g);
                }).detach();
            }

            // 4. Проверка на помехи (Glitch)
            std::uniform_int_distribution<int> distGlitch(1, current.glitch);
            if (distGlitch(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distGlitchDur(7000, 20000);
                    GlitchScreenOverlay(hInstance, L"VAS VZLAMIVAET JOPA", distGlitchDur(g));
                }).detach();
            }

            // 5. Проверка на дрожание окна (Shake)
            std::uniform_int_distribution<int> distShake(1, current.shake);
            if (distShake(gen) == 1) {
                std::thread([]() {
                    ShakeWindow(GetForegroundWindow(), 2000);
                }).detach();
            }

            // 6. Проверка на инверсию экрана
            std::uniform_int_distribution<int> distInvert(1, current.invert);
            if (distInvert(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distInvertDur(7000, 20000);
                    InvertScreenOverlay(hInstance, distInvertDur(g));
                }).detach();
            }

            // 7. Проверка на гигантский курсор
            std::uniform_int_distribution<int> distCursor(1, current.giantCursor);
            if (distCursor(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distCursorDur(5000, 10000);
                    SetHugeCursorForDuration(hInstance, distCursorDur(g));
                }).detach();
            }

            // 8. Проверка на сворачивание окон (Win+D)
            std::uniform_int_distribution<int> distPizda(1, current.pizda);
            if (distPizda(gen) == 1) {
                std::thread([hInstance]() {
                    PIZDA(hInstance);
                }).detach();
            }

            // 9. Проверка на зум 2x
            std::uniform_int_distribution<int> distZoom(1, current.zoom);
            if (distZoom(gen) == 1) {
                std::thread([]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distZoomDur(5000, 15000);
                    ZoomScreen(2.0f, distZoomDur(g));
                }).detach();
            }

            // 10. Проверка на черную полоску
            std::uniform_int_distribution<int> distStripe(1, current.stripe);
            if (distStripe(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    std::uniform_int_distribution<int> distStripeDur(7000, 15000);
                    BlackStripeOverlay(hInstance, distStripeDur(g));
                }).detach();
            }

            // 11. Проверка на скиллчек DBD
            std::uniform_int_distribution<int> distSkill(1, current.skillCheck);
            if (distSkill(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    ShowSkillCheck(hInstance, g);
                }).detach();
            }

            // 12. Проверка на Марионетку FNAF
            std::uniform_int_distribution<int> distMario(1, current.marionette);
            if (distMario(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    ShowMarionetteEffect(hInstance, g);
                }).detach();
            }

            // 13. Проверка на рекламу
            std::uniform_int_distribution<int> distAds(1, current.ads);
            if (distAds(gen) == 1) {
                std::thread([hInstance]() {
                    std::random_device rd; std::mt19937 g(rd());
                    ShowAdsPopup(hInstance, g);
                }).detach();
            }

            // 14. Проверка на понижение качества (1280x720)
            std::uniform_int_distribution<int> distLowQual(1, current.lowQuality);
            if (distLowQual(gen) == 1) {
                std::thread([]() {
                    LowQualityScreenEffect(10000);
                }).detach();
            }

            // 15. Проверка на мышку-эпилептика
            std::uniform_int_distribution<int> distEpileMouse(1, current.epilepsyMouse);
            if (distEpileMouse(gen) == 1) {
                std::thread([]() {
                    EpilepticMouseEffect(2500);
                }).detach();
            }
        }
    }

    if (g_hKeyHook) {
        UnhookWindowsHookEx(g_hKeyHook);
        g_hKeyHook = NULL;
    }
}

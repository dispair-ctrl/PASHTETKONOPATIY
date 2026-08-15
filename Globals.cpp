#include "Globals.h"

// Глобальные состояния
std::atomic<bool> g_isEnabled(true);
std::atomic<FrequencyMode> g_currentMode(MODE_MEDIUM);
std::atomic<bool> g_isRunning(true);
std::atomic<bool> g_cancelEffects(false);
std::atomic<bool> g_isEffectActive(false);

WORD selectedImageId = 0;
WORD selectedSoundId = 0;

HWND g_hwndStatus = NULL;
HBRUSH g_hDarkBrush = NULL;
HFONT g_hGuiFont = NULL;

const ModeChances G_MODE_CHANCES[] = {
    // 0: Низкая
    { 200, 500, 400, 350, 300, 400, 350, 600, 350, 350, 450, 300, 250, 400, 300, 800, L"Режим: НИЗКАЯ частота событий" },
    // 1: Средняя
    { 100, 250, 200, 180, 150, 200, 180, 300, 180, 180, 250, 180, 150, 250, 180, 450, L"Режим: СРЕДНЯЯ частота событий" },
    // 2: Высокая
    {  50, 120, 100,  90,  75, 100,  90, 150,  90,  90, 120,  90,  80, 120,  90, 200, L"Режим: ВЫСОКАЯ частота (Флешка 1/50)" },
    // 3: Смерть
    {  25,  40,  50,  30,  25,  35,  30,  50,  30,  30,  45,  35,  30,  50,  40,  80, L"Режим: 💀 СМЕРТЬ! (Флешка 1/25)" },
    // 4: Смерть++
    {  15,  25,  30,  17,  15,  26,  17,  30,  17,  17,  25,  20,  18,  30,  20,  40, L"Режим: 💀💀💀 СМЕРТЬ++!" }
};

std::vector<WORD> IMAGES;
std::vector<WORD> SCREAMER_SOUNDS;
std::vector<WORD> FLASHBANG_SOUNDS;
std::vector<WORD> LONG_SOUNDS;

static BOOL CALLBACK EnumResourcesCallback(HMODULE hModule, LPCWSTR lpType, LPWSTR lpName, LONG_PTR lParam) {
    if (IS_INTRESOURCE(lpName)) {
        WORD resId = (WORD)(ULONG_PTR)lpName;
        if (resId >= 1000 && resId <= 1999) {
            IMAGES.push_back(resId);
        } else if (resId >= 2000 && resId <= 2999) {
            SCREAMER_SOUNDS.push_back(resId);
        } else if (resId >= 3000 && resId <= 3999) {
            FLASHBANG_SOUNDS.push_back(resId);
        } else if (resId >= 4000 && resId <= 4999) {
            LONG_SOUNDS.push_back(resId);
        }
    }
    return TRUE;
}

void InitDynamicResources(HINSTANCE hInstance) {
    IMAGES.clear();
    SCREAMER_SOUNDS.clear();
    FLASHBANG_SOUNDS.clear();
    LONG_SOUNDS.clear();

    EnumResourceNamesW(hInstance, RT_RCDATA, EnumResourcesCallback, 0);
    EnumResourceNamesW(hInstance, L"WAVE", EnumResourcesCallback, 0);
}


const DWORD SYSTEM_CURSOR_IDS[] = {
    OCR_NORMAL, OCR_IBEAM, OCR_WAIT, OCR_CROSS, OCR_UP,
    OCR_SIZENWSE, OCR_SIZENESW, OCR_SIZEWE, OCR_SIZENS,
    OCR_SIZEALL, OCR_NO, OCR_HAND, OCR_APPSTARTING
};

const size_t NUM_SYSTEM_CURSORS = sizeof(SYSTEM_CURSOR_IDS) / sizeof(SYSTEM_CURSOR_IDS[0]);

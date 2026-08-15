#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef OEMRESOURCE
#define OEMRESOURCE
#endif

#include <windows.h>
#include <mmsystem.h>
#include <atomic>
#include <vector>
#include <string>
#include <random>

#include "resource.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "magnification.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

// ----------------------------------------------------------------------------
// Режимы и Интенсивность Событий
// ----------------------------------------------------------------------------

enum FrequencyMode {
    MODE_LOW = 0,
    MODE_MEDIUM = 1,
    MODE_HIGH = 2,
    MODE_DEATH = 3,
    MODE_DEATHPLUS = 4
};

struct ModeChances {
    int flashbang;   // Шанс 1 из N в секунду
    int screamer;    // Шанс 1 из N в секунду
    int longSound;   // Шанс 1 из N в секунду
    int glitch;      // Шанс 1 из N в секунду
    int shake;       // Шанс 1 из N в секунду
    int invert;      // Шанс 1 из N в секунду
    int giantCursor; // Шанс 1 из N в секунду
    int pizda;       // Шанс 1 из N в секунду
    int zoom;        // Шанс 1 из N в секунду
    int stripe;      // Шанс 1 из N в секунду
    int skillCheck;  // Шанс 1 из N в секунду
    int marionette;  // Шанс 1 из N в секунду (Марионетка FNAF)
    int ads;         // Шанс 1 из N в секунду (Реклама)
    int lowQuality;  // Шанс 1 из N в секунду (Понижение качества 720p)
    int epilepsyMouse; // Шанс 1 из N в секунду (Мышка-эпилептик)
    int captcha;     // Шанс 1 из N при нажатии клавиши (Капча)
    const wchar_t* description;
};

extern const ModeChances G_MODE_CHANCES[];

// Глобальные состояния
extern std::atomic<bool> g_isEnabled;
extern std::atomic<FrequencyMode> g_currentMode;
extern std::atomic<bool> g_isRunning;
extern std::atomic<bool> g_cancelEffects;
extern std::atomic<bool> g_isEffectActive;

extern WORD selectedImageId;
extern WORD selectedSoundId;

extern HWND g_hwndStatus;
extern HBRUSH g_hDarkBrush;
extern HFONT g_hGuiFont;

// Идентификаторы элементов управления GUI
#define IDC_TOGGLE_BTN        1001
#define IDC_RADIO_LOW         1002
#define IDC_RADIO_MED         1003
#define IDC_RADIO_HIGH        1004
#define IDC_RADIO_DEATH       1005
#define IDC_RADIO_DEATHPLUS   1008
#define IDC_STATUS_LABEL      1006
#define IDC_BTN_CANCEL_EFFECT 1007

#ifdef _DEBUG
#define IDC_BTN_TEST_LONGSOUND 2001
#define IDC_BTN_TEST_SCREAMER  2002
#define IDC_BTN_TEST_FLASHBANG 2003
#define IDC_BTN_TEST_GLITCH    2004
#define IDC_BTN_TEST_SHAKE     2005
#define IDC_BTN_TEST_INVERT    2006
#define IDC_BTN_TEST_CURSOR    2007
#define IDC_BTN_TEST_PIZDA     2008
#define IDC_BTN_TEST_ZOOM      2009
#define IDC_BTN_TEST_STRIPE    2010
#define IDC_BTN_TEST_SKILLCHECK 2011
#define IDC_BTN_TEST_MARIONETTE 2012
#define IDC_BTN_TEST_ADS        2013
#define IDC_BTN_TEST_LOWQUAL    2014
#define IDC_BTN_TEST_EPILEMOUSE 2015
#define IDC_BTN_TEST_CAPTCHA    2016
#endif

// Списки ресурсов (заполняются динамически при старте)
extern std::vector<WORD> IMAGES;
extern std::vector<WORD> SCREAMER_SOUNDS;
extern std::vector<WORD> FLASHBANG_SOUNDS;
extern std::vector<WORD> LONG_SOUNDS;
extern const DWORD SYSTEM_CURSOR_IDS[];
extern const size_t NUM_SYSTEM_CURSORS;

void InitDynamicResources(HINSTANCE hInstance);

